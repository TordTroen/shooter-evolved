# main.cpp Refactoring Plan

This document is a self-contained brief for an AI coding agent. The goal is to split `main.cpp` into focused modules under `src/`, in line with the project's `CLAUDE.md` guidelines.

## Context the agent needs

- Project root: `C:\projects\c++\shooter\project`
- Build: run `build.bat` (do **not** invoke `cmake` / `msbuild` / `cl` directly).
- Test: run `test.bat`.
- Source lives in `src/`. Tests live in `tests/` (Catch2).
- Engine stack: SDL3 + OpenGL (glad) + GLM + Jolt Physics + ImGui + a glTF loader.
- Rendering uses one shared `Shader` (`shaders/basic.vert` / `shaders/basic.frag`) with uniforms `view`, `projection`, `model`, `uBaseColor`, `uHasBaseColor`, `uEmissive`, `uColor`.
- `main.cpp` currently sits at the project root (477 lines).
- `CLAUDE.md` rules to keep in mind:
  - Prefer self-contained classes/namespaces; minimize cross-module coupling.
  - Keep `main.cpp` to wiring; push real logic into modules under `src/`.
  - Structure logic so it can be unit-tested — pure functions, DI, small testable units.

## Build/test gates after every step

After each extraction:
1. Run `build.bat`. Fix any build break before continuing.
2. Run `test.bat`. Existing tests must stay green.
3. Smoke-test the binary: window opens, ground/boxes render, WASD + mouse work, left click fires, muzzle flash + hitmarker appear, FPS overlay visible.

Behaviour must be **identical** to the current `main.cpp` after each step — these are pure refactors. No new features, no changed visuals, no changed tunables.

## Current `main.cpp` map (line ranges in the existing file)

| Lines     | Concern                                  |
|-----------|------------------------------------------|
| 32–35     | `Debug::Settings::kEnableHitPositionMarker` (debug toggle) |
| 37–63     | `setProjectRootAsWorkingDir()` (walks up from exe looking for `shaders/`) |
| 66–106    | `kPlaneVerts`, `kBoxVerts` (procedural geometry) |
| 110–140   | `makeMuzzleFlashPixels()` (procedural RGBA texture) |
| 142–149   | `kPlaneIdx`, `kBoxIdx` |
| 151–164   | `main()` startup: window, GL state, ImGui |
| 165–195   | Asset/mesh setup (shader, meshes, default white texture, gltf gun, muzzle flash texture, scene, viewmodel) |
| 196–212   | Character, camera, projection, timers |
| 214–260   | Main loop event pump (quit, escape, resize+projection-rebuild, mouse button, mouse motion) |
| 262–307   | Fire handling (raycast, damage, impulse, debug marker) |
| 309–325   | Scene tick, character update, camera sync, shader hot-reload, GL clear, shader setup |
| 327–331   | Actor render loop |
| 336–373   | Viewmodel rendering (camera-basis transform, glTF axis fix) |
| 375–419   | Muzzle-flash billboard (additive blend, depth mask, timer decrement) |
| 422–470   | HUD: FPS overlay, crosshair, hitmarker (ImGui draw-list) |
| 471–476   | imguiLayer.endFrame, swap |

## Extractions (in suggested order)

Do these as **separate** changes — one extraction per step, build + smoke test between each. Don't bundle.

---

### Step 1 — Primitives (rendering geometry data)

**Files to create**
- `src/rendering/Primitives.h`
- `src/rendering/Primitives.cpp`

**Move from main.cpp**
- `kPlaneVerts` (lines 66–71)
- `kPlaneIdx`  (line 72)
- `kBoxVerts`  (lines 75–106)
- `kBoxIdx`    (lines 142–149)

**Proposed API**
```cpp
namespace Primitives
{
    std::span<const Vertex>   planeVertices();
    std::span<const uint32_t> planeIndices();
    std::span<const Vertex>   boxVertices();
    std::span<const uint32_t> boxIndices();
}
```
The constants can live in the `.cpp` as `static constexpr std::array<...>`. Header just declares the accessors. `Vertex` is in `src/rendering/Vertex.h`.

**Call site update**
`main.cpp` constructs meshes via:
```cpp
Mesh planeMesh(Primitives::planeVertices(), Primitives::planeIndices());
Mesh boxMesh  (Primitives::boxVertices(),   Primitives::boxIndices());
```

**Test (add to `tests/`)**
`tests/test_primitives.cpp`:
- Plane has 4 vertices, 6 indices.
- Box has 24 vertices, 36 indices.
- Every index is in range `[0, vertCount)`.
- All normals are unit length (within 1e-5).

**CMake**
Add the new `.cpp` to whatever `add_library` / source list the existing `src/` files use. Mirror neighbouring entries. Add the new test file to `tests/CMakeLists.txt`.

---

### Step 2 — Procedural textures

**Files to create**
- `src/rendering/ProceduralTextures.h`
- `src/rendering/ProceduralTextures.cpp`

**Move from main.cpp**
- `makeMuzzleFlashPixels(int size)` (lines 110–140) — keep it pure (returns `std::vector<uint8_t>`).

**Proposed API**
```cpp
namespace ProceduralTextures
{
    std::vector<uint8_t> muzzleFlashRGBA(int size);
}
```

**Call site update**
```cpp
constexpr int kMuzzleFlashTexSize = 128;
const auto muzzleFlashPixels = ProceduralTextures::muzzleFlashRGBA(kMuzzleFlashTexSize);
Texture muzzleFlashTexture(muzzleFlashPixels.data(), kMuzzleFlashTexSize, kMuzzleFlashTexSize, 4);
```

**Test**
`tests/test_procedural_textures.cpp`:
- For size=32: output size is `32*32*4` bytes.
- Centre pixel alpha is at/near 255 (brightest core).
- A corner pixel alpha is 0 (radial falloff reaches the edge).

---

### Step 3 — Project paths

**Files to create**
- `src/core/Paths.h`
- `src/core/Paths.cpp`

**Move from main.cpp**
- `setProjectRootAsWorkingDir()` (lines 37–63).

**Proposed API**
```cpp
namespace Paths
{
    // Walks up from the exe dir until it finds a directory containing
    // the anchor (default "shaders"). Sets that as cwd. Returns true on success.
    bool setWorkingDirToProjectRoot(std::string_view anchor = "shaders");
}
```
Use `SDL_GetBasePath()` exactly as today. Keep the `std::cerr` log lines.

**Call site update**
```cpp
Paths::setWorkingDirToProjectRoot();
```

**Test**
`tests/test_paths.cpp`:
- Calling with anchor `""` returns false (or trivially true at /) — confirm chosen behaviour and lock it in.
- (Optional) create a temp dir tree, change cwd into a child, call the function, assert cwd moves up to the anchor. Skip if filesystem manipulation in tests is awkward.

---

### Step 4 — HUD

**Files to create**
- `src/ui/Hud.h`
- `src/ui/Hud.cpp`

**Move from main.cpp**
- FPS overlay (lines 423–438)
- Crosshair lines (lines 440–453)
- Hitmarker block (lines 455–469) — **includes its own timer state**.

**Proposed API**
```cpp
class Hud
{
public:
    void triggerHitmarker();           // call when a shot damages something
    void draw(float deltaTime);        // call between imguiLayer.beginFrame() / endFrame()

private:
    float m_hitmarkerTimer = 0.0f;
    static constexpr float kHitmarkerDuration = 0.18f;
};
```
Move all magic numbers (`kCrosshairSize`, `kCrosshairGap`, `kCrosshairThickness`, `kCrosshairColor`, `kHitmarkerInner`, `kHitmarkerOuter`, `kHitmarkerThickness`, the `IM_COL32` colour) inside `Hud.cpp` as file-scope constants.

**Call site update**
```cpp
Hud hud;
// in the fire-handler, when target was damaged:
hud.triggerHitmarker();
// in the render loop, inside the imguiLayer begin/end:
hud.draw(deltaTime);
```
Remove `hitmarkerTimer` and `kHitmarkerDuration` from `main.cpp`.

**Test**
`tests/test_hud.cpp` — can't render ImGui in tests, but logic is testable:
- After `triggerHitmarker()`, `Hud` should report "active". Expose a `bool hitmarkerActive() const` if needed for the test (otherwise skip).
- Decay test: requires exposing the timer. Optional; if the public API would only exist for the test, skip it.

---

### Step 5 — Muzzle flash effect

**Files to create**
- `src/rendering/MuzzleFlashEffect.h`
- `src/rendering/MuzzleFlashEffect.cpp`

**Move from main.cpp**
- The block at lines 375–419 (billboard transform, GL blend/depth state, timer decrement).
- `kMuzzleFlashDuration` (line 210).
- Constants `kFlashForwardOffset`, `kFlashSize`.

**Proposed API**
```cpp
class MuzzleFlashEffect
{
public:
    MuzzleFlashEffect(const Mesh& planeMesh, const Texture& flashTexture);

    void trigger();
    // muzzlePos is the world-space position of the flash (barrel tip).
    // billboardRight/Up = the camera basis vectors used for billboarding.
    // -camFront should be passed for the plane normal.
    void render(Shader& shader,
                const glm::vec3& muzzlePos,
                const glm::vec3& camRight,
                const glm::vec3& camUp,
                const glm::vec3& negCamFront,
                float deltaTime);

private:
    const Mesh&    m_planeMesh;
    const Texture& m_texture;
    float          m_timer = 0.0f;
    static constexpr float kDuration = 0.2f;
};
```
The class owns its timer. `render` is a no-op when `m_timer <= 0`. Keep GL state push/pop scoped exactly as today (enable blend, additive func, depth mask off → draw → restore).

**Call site update**
- Fire handler calls `muzzleFlash.trigger();`.
- Inside the viewmodel render block, after drawing the gun, call `muzzleFlash.render(...)` with the computed `gunPos`-like flash position and camera basis vectors. The flash position computation (with `kFlashForwardOffset`) can move *inside* `render` if you pass in the camera origin + offsets, **but** to keep the diff small leave the position computation in main for this step.

Remove `muzzleFlashTimer`, `kMuzzleFlashDuration` from `main.cpp`.

---

### Step 6 — Camera basis accessors

**Files to modify**
- `src/scene/Camera.h`
- `src/scene/Camera.cpp`

**Change**
`Camera` already maintains `m_right` and `m_up` internally (see `Camera.h:20-21`). Currently `main.cpp:343-346` recomputes them from `front()`. Expose them:
```cpp
[[nodiscard]] glm::vec3 right() const { return m_right; }
[[nodiscard]] glm::vec3 up()    const { return m_up; }
```
Verify `updateVectors()` keeps `m_right` and `m_up` orthonormal (it does today). Replace the recomputation in `main.cpp` with `camera.right()` / `camera.up()`.

This is a one-line-each change but unlocks Step 7's API to be cleaner.

---

### Step 7 — Viewmodel renderer

**Files to create**
- `src/rendering/ViewmodelRenderer.h`
- `src/rendering/ViewmodelRenderer.cpp`

**Move from main.cpp**
- The block at lines 336–373 (camera-basis transform construction + the glTF axis-fix matrix).
- Constants `kRightOffset`, `kDownOffset`, `kForwardOffset`, `kGunScale`.

**Proposed API**
```cpp
class ViewmodelRenderer
{
public:
    // axisFixDegrees: yaw applied around Y to align the model's local
    // forward axis with camera-forward. BasicGun.glb wants -90.
    ViewmodelRenderer(MeshRenderer& meshRenderer,
                      float rightOffset   =  0.25f,
                      float downOffset    = -0.20f,
                      float forwardOffset =  0.45f,
                      float scale         =  0.25f,
                      float axisFixDegrees= -90.0f);

    void render(Shader& shader, const Camera& camera) const;

    // Exposed so MuzzleFlashEffect can position the flash from the same offsets.
    [[nodiscard]] glm::vec3 muzzleWorldPos(const Camera& camera,
                                           float forwardOffset) const;

private:
    MeshRenderer& m_meshRenderer;
    float m_rightOffset, m_downOffset, m_forwardOffset, m_scale, m_axisFixDegrees;
};
```
Inside `render`, do the depth clear (`glClear(GL_DEPTH_BUFFER_BIT)`) before drawing — that behaviour is part of the viewmodel.

**Call site update**
```cpp
if (gunModel.mesh)
{
    MeshRenderer gunMR(gunModel.mesh.get(), glm::vec3(1.0f));
    gunMR.texture = gunModel.baseColorTexture.get();
    ViewmodelRenderer viewmodel(gunMR);
    // ... in loop:
    viewmodel.render(shader, camera);
    muzzleFlash.render(shader,
                       viewmodel.muzzleWorldPos(camera, /*forwardOffset=*/0.65f),
                       camera.right(), camera.up(), -camera.front(),
                       deltaTime);
}
```
Or keep `gunViewmodel` as a `std::unique_ptr<ViewmodelRenderer>` if you prefer the existing optional-presence pattern.

---

### Step 8 — Weapon / hitscan

**Files to create**
- `src/player/Weapon.h`
- `src/player/Weapon.cpp`

**Move from main.cpp**
- Fire handler logic (lines 265–307).
- Constants `kShotDamage`, `kShotImpulse`.
- `Debug::Settings::kEnableHitPositionMarker` (or pass it in / drop it — see below).

**Proposed API**
```cpp
struct FireResult
{
    bool      hit      = false;
    bool      damaged  = false;          // hit AND target was damageable
    glm::vec3 position = glm::vec3(0.0f);// world hit position (if hit)
};

class Weapon
{
public:
    explicit Weapon(int damage = 25, float impulse = 50.0f);

    FireResult fire(Scene& scene,
                    const glm::vec3& origin,
                    const glm::vec3& direction);

private:
    int   m_damage;
    float m_impulse;
};
```
Move the `std::cout << "[Shot] hit at ..."` and `[Damage]` logging into `Weapon::fire` unchanged.

For the optional debug hit-position marker: the cleanest move is to drop the `Debug::Settings::kEnableHitPositionMarker` constant entirely (it's `false`). If you want to preserve it, accept a `Mesh* debugMarkerMesh = nullptr` parameter to `fire` and spawn the marker actor inside `Weapon` when non-null.

**Call site update**
```cpp
Weapon weapon;
// in the event handler:
if (shouldFire)
{
    shouldFire = false;
    muzzleFlash.trigger();
    const glm::vec3 eyePos = character.position()
        + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f);
    const FireResult result = weapon.fire(scene, eyePos, camera.front());
    if (result.damaged) hud.triggerHitmarker();
}
```

**Test**
`tests/test_weapon.cpp` — this depends on whether `Scene` / `PhysicsWorld` can be constructed in a test. If construction is heavyweight (Jolt init etc.), skip the unit test for now and rely on the smoke test. Note this in a comment.

---

### Step 9 — DemoScene cleanup (optional, low priority)

`DemoScene::setup()` (`src/scene/DemoScene.cpp:20-111`) repeats `Actor + MeshRenderer + PhysicsBody` construction five times with only data differences. Add a private helper:

```cpp
Actor& DemoScene::spawnBox(glm::vec3 pos,
                           glm::vec3 scale,
                           glm::vec3 color,
                           int health,
                           JPH::EMotionType motion,
                           float mass);
```
…and rewrite `setup()` as five calls. The ground plane uses a different mesh and a deliberate physics/render offset, so leave it as a separate block.

This is purely a readability win — skip it if time-constrained.

---

### Step 10 — Drop `TestClass` (optional)

`src/TestClass.h` / `src/TestClass.cpp` and `tests/test_test_class.cpp` are placeholder scaffolding. Once at least one real extracted module has a passing test (Steps 1 or 2), delete:
- `src/TestClass.h`
- `src/TestClass.cpp`
- `tests/test_test_class.cpp`
- Their references in any CMake source lists.

Verify `test.bat` still runs and passes the new tests.

## Things NOT to change

- Public API of `Window`, `Mesh`, `Shader`, `Texture`, `Actor`, `Scene`, `PhysicsWorld`, `CharacterController`, `ImGuiLayer`, `ModelLoader`, `MeshRenderer`, `PhysicsBody`. They look healthy.
- Shader file paths or uniform names.
- Asset paths (`assets/models/BasicGun.glb`, `shaders/basic.*`).
- Any tunable values during refactor (offsets, damage, durations). Move them, don't change them.
- The order of operations in the frame loop: poll events → fire if requested → `scene.tick` → `character.update` → camera sync → shader hot-reload → clear → world render → viewmodel render → muzzle flash → ImGui → swap. Preserve this exactly.

## Expected end state

`main.cpp` reduced to roughly 80–120 lines, doing only:
1. `Paths::setWorkingDirToProjectRoot()`.
2. Construct `Window`, set relative mouse mode, wire GL debug, set GL state.
3. Construct `ImGuiLayer`, `Shader`, meshes (via `Primitives`), default white texture, gun model + viewmodel, muzzle flash texture + effect, scene, character, camera, projection, weapon, hud.
4. Loop: poll events → fire if requested → tick scene → update character → sync camera → hot-reload → clear → render actors → render viewmodel → render muzzle flash → `imguiLayer.beginFrame`/`hud.draw`/`endFrame` → swap.

Roughly one statement per concern, with the heavy lifting delegated to the extracted classes.

## Final checks before declaring done

- [ ] `build.bat` succeeds with no new warnings.
- [ ] `test.bat` passes; new tests for `Primitives` and `ProceduralTextures` are present and green.
- [ ] Manual smoke test: ground/boxes render, WASD+mouse look works, click fires, muzzle flash + hitmarker visible, FPS overlay top-left, ESC quits, resize doesn't break projection.
- [ ] `main.cpp` is under 150 lines.
- [ ] No new responsibilities introduced — pure refactor.
