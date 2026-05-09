# OpenGL FPS Demo — Implementation Plan

## Tech Stack

| Library | Role |
|---|---|
| C++20 | Language (`std::span`, designated initialisers, `<numbers>`) |
| OpenGL 4.5 Core | Rendering API (DSA, debug output, immutable storage) |
| SDL3 | Windowing, input, GL context creation |
| GLAD | OpenGL function loading via `SDL_GL_GetProcAddress` |
| GLM | Math — vectors, matrices, transforms |
| stb_image | Texture loading |

Target a **GL 4.5 core profile** context throughout. This unlocks Direct State Access (DSA), `KHR_debug`, and immutable texture storage, all of which simplify the code and catch bugs earlier.

---

## Build Configuration

Enable C++20 in your build system of choice:

**CMake** (recommended)
```cmake
cmake_minimum_required(VERSION 3.20)
project(fps_demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "main.cpp" "src/**/*.cpp")
add_executable(fps_demo ${SOURCES})

target_include_directories(fps_demo PRIVATE src)
# Link SDL3, OpenGL, GLM etc. here
```

**C++20 features worth using in this project**
- `std::span` — `Mesh` takes `std::span<const Vertex>` and `std::span<const uint32_t>` so it accepts `std::array`, C arrays, and `std::vector` interchangeably with no copies. Hard-coded plane/box geometry should be `constexpr std::array`.
- Designated initialisers — cleaner construction of config structs (e.g. `WindowConfig{ .title = "Demo", .width = 1280 }`)
- `<numbers>` — `std::numbers::pi` instead of a `#define`
- `[[nodiscard]]` on `getViewMatrix()`, `getProjectionMatrix()` etc. to catch accidentally discarded matrices

---

## Working in JetBrains Rider

This project is intended to be developed in **Rider 2026.1 or later**. Rider only gained CMake-based C++ support in 2026.1 (currently shipping as Beta), and JetBrains positions it specifically at game development workflows — non-Unreal C++ projects in earlier Rider versions were not supported. If you're on an older Rider, either upgrade or use CLion instead (CLion has had mature CMake support for years and uses the same ReSharper C++ engine, so the editing experience is essentially identical).

### Opening the project

1. **File → Open** and select the project root (`C:\projects\c++\shooter\project`) — the folder containing `CMakeLists.txt`, *not* the `CMakeLists.txt` file itself.
2. Rider detects the CMake project and prompts to load it. Accept; Rider runs the CMake configure step and populates the project view.
3. The **CMake** tool window (View → Tool Windows → CMake) shows configure output — watch this on first load to confirm SDL3, GLM, etc. were found.

### Toolchain & profile

- Settings → **Build, Execution, Deployment → Toolchains**. On Windows the practical options are **Visual Studio** (uses MSVC, recommended for this project) or **MinGW**. Pick Visual Studio unless you have a specific reason not to — SDL3 prebuilt binaries ship MSVC-compatible `.lib` files, and the MSVC debugger integrates more cleanly.
- Settings → **Build, Execution, Deployment → CMake**. Rider creates a `Debug` profile by default. Add a `Release` profile too (same dialog, **+** button, set Build type to `Release`) so you can switch from the run configuration dropdown.
- Generator: leave at the default (Ninja with the VS toolchain) unless you specifically need Visual Studio solution files.

### Dependencies

The cleanest option for SDL3, GLM, and stb_image is **vcpkg in manifest mode**, which Rider picks up automatically:

1. Create a `vcpkg.json` in the project root listing `sdl3`, `glm`, and `stb`.
2. Set the CMake toolchain file in Settings → Build, Execution, Deployment → CMake → CMake options:
   ```
   -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
   ```
3. In `CMakeLists.txt`, use `find_package(SDL3 CONFIG REQUIRED)` etc. and link against the imported targets (`SDL3::SDL3`, `glm::glm`).

GLAD is generated rather than fetched from a package manager — go to the [GLAD web generator](https://glad.dav1d.de/), pick GL 4.5 Core, download, and drop the `glad/` directory into `third_party/` with its own small `CMakeLists.txt` that exposes a `glad` static library target.

### Run configuration

Rider auto-creates a run configuration named after the executable target (`fps_demo`). Two things worth setting on it:

- **Working directory:** set to `$ProjectFileDir$`. The shader and asset paths in the code are relative (`shaders/basic.vert`, `assets/textures/...`), so the working directory must be the project root or `fopen` will fail.
- **Environment variables:** if vcpkg drops SDL3's DLL somewhere not on `PATH`, either copy it next to the executable as a CMake post-build step, or add its directory to `PATH` here.

### Debugging tips specific to this project

- The MSVC debugger in Rider supports **Natvis**. GLM ships Natvis files but they're not always picked up automatically — if `glm::vec3` and `glm::mat4` show as raw float arrays in the watch window, point Rider at GLM's `util/glm.natvis` via Settings → Build, Execution, Deployment → Debugger → Data Views → C/C++ → Natvis.
- Set a breakpoint inside the `glDebugMessageCallback` function. When GL emits an error, the debugger stops there with a full call stack pointing back to the offending GL call — much faster than reading stderr after the fact.
- Shader compile errors print to stderr from the `Shader` class. Rider's Run window captures stderr by default; no extra setup needed.

### Files Rider creates

Rider writes a `.idea/` directory at the project root for workspace state (run configs, recent files, etc.) and a `cmake-build-debug/` (and `cmake-build-release/`) directory for build artifacts. Add both to `.gitignore`:

```
.idea/
cmake-build-*/
```

---

## Suggested File Layout

```
project/
├── main.cpp                        # Entry point, SDL init, main loop, delta time
│
├── src/
│   ├── core/
│   │   └── Window.h/.cpp           # SDL3 window + GL context lifetime
│   │
│   ├── rendering/
│   │   ├── Shader.h/.cpp           # Compile, link, set uniforms, optional hot-reload
│   │   ├── Mesh.h/.cpp             # VAO/VBO/EBO upload + draw call (DSA)
│   │   ├── Texture.h/.cpp          # stb_image load, glTextureStorage2D upload
│   │   └── GLDebug.h/.cpp          # glDebugMessageCallback wiring
│   │
│   └── scene/
│       ├── Camera.h/.cpp           # View matrix, mouse look, WASD
│       └── SceneObject.h/.cpp      # Transform + mesh reference per object
│
├── shaders/
│   ├── basic.vert
│   └── basic.frag
│
└── assets/
    └── textures/                   # PNG/JPG files loaded by Texture.h
```

Delta time is simple enough (a `Uint64` last-tick value plus a subtraction) that it lives directly in `main.cpp`'s loop rather than getting its own translation unit.

---

## Phase 1 — Project Setup & Window

**Goal:** Get a window open with a working OpenGL 4.5 debug context.

- Initialize SDL3 with `SDL_Init(SDL_INIT_VIDEO)`
- Set GL attributes **before** window creation:
  - `SDL_GL_CONTEXT_MAJOR_VERSION = 4`, `SDL_GL_CONTEXT_MINOR_VERSION = 5`
  - `SDL_GL_CONTEXT_PROFILE_MASK = SDL_GL_CONTEXT_PROFILE_CORE`
  - `SDL_GL_CONTEXT_FLAGS = SDL_GL_CONTEXT_DEBUG_FLAG` (debug builds at minimum)
  - `SDL_GL_DEPTH_SIZE = 24`
- Create window with `SDL_WINDOW_OPENGL` flag
- Create context with `SDL_GL_CreateContext`
- Bootstrap GLAD using `SDL_GL_GetProcAddress` as the loader
- **Wire up `glDebugMessageCallback`** immediately after GLAD loads. Log severity, type, and message to stderr. This is the single highest-ROI thing in the whole project — you'll catch invalid enums, deprecated calls, performance warnings, and shader linker issues at the moment they happen instead of staring at a black screen.
- Set up a basic event loop with `SDL_PollEvent` and `SDL_GL_SwapWindow`

---

## Phase 2 — Shader & Geometry Pipeline

**Goal:** Get a triangle/mesh on screen with the MVP matrix chain working.

### Shader class
- Load vertex and fragment source from files
- Compile both stages, link into a program
- Helper methods to set `int`, `float`, `vec3`, `mat4` uniforms
- *Optional, big iteration win:* check the shader file's mtime each frame and recompile + relink on change. Lighting and effects work becomes dramatically faster.

### Mesh class
- Constructor takes `std::span<const Vertex>` and `std::span<const uint32_t>`
- Use DSA: `glCreateVertexArrays`, `glCreateBuffers`, then `glNamedBufferStorage` (immutable storage) and `glVertexArrayVertexBuffer` / `glVertexArrayElementBuffer` / `glVertexArrayAttribFormat` / `glVertexArrayAttribBinding` / `glEnableVertexArrayAttrib`. No bind-to-configure dance.
- Vertex layout: `position + normal + UV`
- Single `draw()` method: `glBindVertexArray(vao); glDrawElements(...)`

### Uniforms
- Upload `model`, `view`, `projection` as three separate `mat4` uniforms. `view` and `projection` are uploaded once per frame; `model` is the only one that changes per object. The vertex shader does the multiplication.

---

## Phase 3 — First-Person Camera

**Goal:** Mouse look + WASD movement with correct delta-time scaling.

### Camera struct
- Fields: `pos`, `front`, `up`, `worldUp`, `yaw`, `pitch`
- **Yaw and pitch are stored in degrees.** Convert to radians only inside `updateVectors()` when feeding `std::cos` / `std::sin`. Mixing units here is a classic source of cameras that lock up at strange angles.
- `getViewMatrix()` returns `glm::lookAt(pos, pos + front, up)`
- `updateVectors()` recomputes `front` from yaw/pitch, re-orthogonalises `right` and `up`

### Mouse look
- At startup, call `SDL_SetWindowRelativeMouseMode(window, true)` to lock and hide the cursor. (This is the SDL3 API — per-window, plain `bool`. The SDL2 form `SDL_SetRelativeMouseMode(SDL_TRUE)` will not compile against SDL3 headers.)
- On `SDL_EVENT_MOUSE_MOTION`, read `event.motion.xrel` / `yrel`
- Scale by sensitivity, add to yaw/pitch, clamp pitch to `±89.0f` degrees (clamping happens in degrees, conversion to radians happens later — see above)
- Call `updateVectors()` after each update

### WASD movement
- Each frame, read keyboard state with `SDL_GetKeyboardState`
- Move along `front` (W/S) and `right` (A/D)
- Scale all movement by `deltaTime`, computed from `SDL_GetTicksNS()` (or `SDL_GetPerformanceCounter()` for the same precision via a different API). Avoid `SDL_GetTicks` for delta — millisecond resolution is too coarse at high frame rates.

### Projection matrix
- `glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 1000.0f)`
- Recompute on window resize

---

## Phase 4 — Scene Geometry

**Goal:** Render the plane and two boxes with correct depth.

- Enable depth testing: `glEnable(GL_DEPTH_TEST)`
- Clear both color and depth buffers each frame: `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`

### Mesh data
- **Plane:** a unit quad authored directly in the **XZ plane** (y = 0 for all four vertices), normals pointing up. Scale only X and Z via the model matrix — e.g. `glm::scale(..., {20, 1, 20})`. The Y component of the scale is a no-op because the source mesh has zero Y extent; it's left as `1` for symmetry, not because it does anything.
- **Box:** 6 faces × 2 triangles, normals pointing outward per face. Hard-code as a `constexpr std::array<Vertex, 24>` (4 unique vertices per face for correct per-face normals/UVs) plus a 36-element index array.

### Rendering the scene each frame
```
// Bind shader, upload view + projection once
shader.use();
shader.setMat4("view", camera.getViewMatrix());
shader.setMat4("projection", projectionMatrix);

// Plane
glm::mat4 model = glm::mat4(1.0f);
model = glm::scale(model, {20, 1, 20});
shader.setMat4("model", model);
planeMesh.draw();

// Box 1
model = glm::translate(glm::mat4(1.0f), {-3, 0.5f, -5});
shader.setMat4("model", model);
boxMesh.draw();

// Box 2
model = glm::translate(glm::mat4(1.0f), {3, 1.0f, -8});
model = glm::scale(model, {1, 2, 1});
shader.setMat4("model", model);
boxMesh.draw();
```

---

## Phase 5 — Textures (Optional Polish)

**Goal:** Replace flat colors with image textures using stb_image.

### Loading & uploading (DSA + immutable storage)
- `stbi_set_flip_vertically_on_load(true)` before loading
- `stbi_load(path, &w, &h, &channels, 0)` returns raw pixel data
- `glCreateTextures(GL_TEXTURE_2D, 1, &id)`
- `glTextureStorage2D(id, mipLevels, GL_RGBA8, w, h)` — allocates immutable storage. Driver can apply optimizations it can't with `glTexImage2D`'s mutable storage.
- `glTextureSubImage2D(id, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, pixels)` — uploads the data
- `glGenerateTextureMipmap(id)`
- Set wrap/filter params with `glTextureParameteri`: `GL_REPEAT`, `GL_LINEAR_MIPMAP_LINEAR`

### Shader changes
- Add `vec2 aTexCoord` to vertex attributes and pass through as `vTexCoord`
- Add `uniform sampler2D uTexture` in fragment shader
- Sample with `texture(uTexture, vTexCoord)`

### Binding at draw time
- `glBindTextureUnit(0, textureId)` (DSA — replaces the `glActiveTexture` + `glBindTexture` pair)
- Set the sampler uniform to slot 0: `shader.setInt("uTexture", 0)` (once, after linking)

---

## Key Integration Notes

- **The MVP chain is the first thing to get right.** Verify with a flat-color shader before adding textures or lighting.
- **GL debug output before anything else.** If `glDebugMessageCallback` isn't wired up, debugging the rest of this list is a guessing game.
- **Delta time** must account for the first frame (clamp to a max, e.g. 0.05s) to avoid a large jump on startup.
- **Mouse warping:** SDL may fire a large motion event when the window first gains focus — skip the first frame's mouse input to avoid a camera snap.
- **Aspect ratio** should be recomputed whenever a `SDL_EVENT_WINDOW_RESIZED` event is received, and `glViewport` updated to match.