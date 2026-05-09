# OpenGL FPS Demo — Implementation Plan

## Tech Stack

| Library | Role |
|---|---|
| C++20 | Language (`std::span`, designated initialisers, `<numbers>`) |
| OpenGL | Rendering API |
| SDL3 | Windowing, input, GL context creation |
| GLAD | OpenGL function loading via `SDL_GL_GetProcAddress` |
| GLM | Math — vectors, matrices, transforms |
| stb_image | Texture loading |
| Jolt Physics | Rigid body physics, collision detection, character controller |

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
- `std::span` — pass mesh vertex/index data to `Mesh` without copying into a `std::vector`
- Designated initialisers — cleaner construction of config structs (e.g. `WindowConfig{ .title = "Demo", .width = 1280 }`)
- `<numbers>` — `std::numbers::pi` instead of a `#define`
- `[[nodiscard]]` on `getViewMatrix()`, `getProjectionMatrix()` etc. to catch accidentally discarded matrices

---

## Suggested File Layout

```
project/
├── main.cpp                        # Entry point, SDL init, main loop
│
├── src/
│   ├── core/
│   │   ├── Window.h/.cpp           # SDL3 window + GL context lifetime
│   │   └── Time.h/.cpp             # Delta time, SDL_GetTicks wrapper
│   │
│   ├── rendering/
│   │   ├── Shader.h/.cpp           # Compile, link, set uniforms
│   │   ├── Mesh.h/.cpp             # VAO/VBO/EBO upload + draw call
│   │   └── Texture.h/.cpp          # stb_image load, glTexImage2D upload
│   │
│   ├── physics/
│   │   ├── PhysicsWorld.h/.cpp     # Jolt system init, layers, broadphase, step
│   │   └── CharacterController.h/.cpp  # Player capsule, ground check, move/jump
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

---

## Phase 1 — Project Setup & Window

**Goal:** Get a window open with a working OpenGL context.

- Initialize SDL3 with `SDL_Init(SDL_INIT_VIDEO)`
- Set GL attributes (version, core profile, depth size) before window creation
- Create window with `SDL_WINDOW_OPENGL` flag
- Create context with `SDL_GL_CreateContext`
- Bootstrap GLAD using `SDL_GL_GetProcAddress` as the loader
- Set up a basic event loop with `SDL_PollEvent` and `SDL_GL_SwapWindow`

---

## Phase 2 — Shader & Geometry Pipeline

**Goal:** Get a triangle/mesh on screen with the MVP matrix chain working.

### Shader class
- Load vertex and fragment source from files
- Compile both stages, link into a program
- Helper methods to set `int`, `float`, `vec3`, `mat4` uniforms

### Mesh class
- Store VAO, VBO, EBO handles
- Upload `position + normal + UV` vertex layout on construction
- Single `draw()` method calling `glDrawElements`

### Uniforms
- Upload `model`, `view`, `projection` as separate `mat4` uniforms each frame
- Compute the full MVP on the CPU: `projection * view * model`

---

## Phase 3 — First-Person Camera

**Goal:** Mouse look + WASD movement with correct delta-time scaling.

### Camera struct
- Fields: `pos`, `front`, `up`, `worldUp`, `yaw`, `pitch`
- `getViewMatrix()` returns `glm::lookAt(pos, pos + front, up)`
- `updateVectors()` recomputes `front` from yaw/pitch, re-orthogonalises `right` and `up`

### Mouse look
- Call `SDL_SetRelativeMouseMode(SDL_TRUE)` at startup to lock & hide cursor
- On `SDL_EVENT_MOUSE_MOTION`, read `event.motion.xrel` / `yrel`
- Scale by sensitivity, add to yaw/pitch, clamp pitch to ±89°
- Call `updateVectors()` after each update

### WASD movement
- Each frame, read keyboard state with `SDL_GetKeyboardState`
- Move along `front` (W/S) and `right` (A/D)
- Scale all movement by `deltaTime` (seconds since last frame via `SDL_GetTicks`)

### Projection matrix
- `glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 1000.0f)`
- Recompute on window resize

---

## Phase 4 — Scene Geometry

**Goal:** Render the plane and two boxes with correct depth.

- Enable depth testing: `glEnable(GL_DEPTH_TEST)`
- Clear both color and depth buffers each frame: `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`

### Mesh data
- **Plane:** single quad (two triangles), scaled large via the model matrix (e.g. `glm::scale(..., {20, 1, 20})`)
- **Box:** 6 faces × 2 triangles, normals pointing outward per face

### Rendering the scene each frame
```
// Bind shader, upload view + projection once
shader.use();
shader.setMat4("view", camera.getViewMatrix());
shader.setMat4("projection", projectionMatrix);

// Plane
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, {0, 0, 0});
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

## Phase 5 — Physics (Jolt)

**Goal:** Player collides with the plane and the two boxes — no clipping, no falling through the floor.

Jolt is a multi-threaded rigid body physics library written in modern C++. It's used in *Horizon Forbidden West* and is permissively licensed (MIT). For this demo we only need three things: a physics world, three static collision shapes for the level, and a kinematic character controller for the player.

### Initialization (once, at startup)
- Call `JPH::RegisterDefaultAllocator()` and `JPH::Factory::sInstance = new JPH::Factory()`
- Call `JPH::RegisterTypes()` to register all collision shape types
- Create a `TempAllocatorImpl` (e.g. 10 MB) and a `JobSystemThreadPool`
- Create the `PhysicsSystem` and call `Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, broadPhaseLayerInterface, objectVsBroadPhaseLayerFilter, objectVsObjectLayerFilter)`
- Set gravity: `physicsSystem.SetGravity({0, -9.81f, 0})`

### Collision layers
Define two object layers and matching broadphase layers:
- `Layers::NON_MOVING` — the plane and boxes (static geometry)
- `Layers::MOVING` — the player capsule
- Filter rule: `MOVING` collides with `NON_MOVING`; `NON_MOVING` doesn't collide with itself

### Static level bodies
Create one body per renderable piece of geometry, matching the visuals in Phase 4:
- **Plane:** `BoxShape({10, 0.5f, 10})` (or a thin box) at `{0, -0.5f, 0}` with motion type `Static`
- **Box 1:** `BoxShape({0.5f, 0.5f, 0.5f})` at `{-3, 0.5f, -5}` static
- **Box 2:** `BoxShape({0.5f, 1.0f, 0.5f})` at `{3, 1.0f, -8}` static (matches the rendered scale `{1, 2, 1}`)
- For each: build `BodyCreationSettings`, call `bodyInterface.CreateAndAddBody(..., EActivation::DontActivate)`

### Player character controller
Use `JPH::CharacterVirtual` — purpose-built for FPS-style movement (no tunneling, smooth slope handling, ground detection):
- Shape: `CapsuleShape(halfHeight = 0.7f, radius = 0.3f)`
- Position the capsule so its centre is at the camera height; on each frame, set `camera.pos` to the character's position plus a small head offset
- WASD now sets a *desired horizontal velocity* on the character instead of moving the camera directly
- Replace direct `camera.pos += ...` with `character->SetLinearVelocity(...)` then `character->ExtendedUpdate(deltaTime, gravity, updateSettings, ...)`

### Stepping the simulation
Once per frame, after input but before rendering:
```
const int collisionSteps = 1;
physicsSystem.Update(deltaTime, collisionSteps, tempAllocator, jobSystem);
character->ExtendedUpdate(deltaTime, physicsSystem.GetGravity(), updateSettings, ...);
```
Use a fixed timestep (e.g. clamp `deltaTime` to `1/60`) for stable collision response.

### Debug checklist
- Spawn the player above the plane (`y = 2`) and confirm they fall and stop on it
- Walk into a box — movement should stop, no jitter, no clipping
- Walk off an edge — character should fall under gravity
- Verify world units match between rendering and physics (1 unit = 1 metre everywhere)

---

## Phase 6 — Textures (Optional Polish)

**Goal:** Replace flat colors with image textures using stb_image.

### Loading & uploading
- `stbi_set_flip_vertically_on_load(true)` before loading
- `stbi_load(path, &w, &h, &channels, 0)` returns raw pixel data
- Generate texture with `glGenTextures`, bind, call `glTexImage2D`
- Generate mipmaps with `glGenerateMipmap(GL_TEXTURE_2D)`
- Set wrap/filter params: `GL_REPEAT`, `GL_LINEAR_MIPMAP_LINEAR`

### Shader changes
- Add `vec2 aTexCoord` to vertex attributes and pass through as `vTexCoord`
- Add `uniform sampler2D uTexture` in fragment shader
- Sample with `texture(uTexture, vTexCoord)`

### Binding at draw time
- `glActiveTexture(GL_TEXTURE0)` → `glBindTexture(...)` before each draw call
- Set the sampler uniform to slot 0: `shader.setInt("uTexture", 0)`

---

## Key Integration Notes

- **The MVP chain is the first thing to get right.** Verify with a flat-color shader before adding textures or lighting.
- **Delta time** must account for the first frame (clamp to a max, e.g. 0.05s) to avoid a large jump on startup.
- **Mouse warping:** SDL may fire a large motion event when the window first gains focus — skip the first frame's mouse input to avoid a camera snap.
- **Aspect ratio** should be recomputed whenever a `SDL_EVENT_WINDOW_RESIZED` event is received, and `glViewport` updated to match.