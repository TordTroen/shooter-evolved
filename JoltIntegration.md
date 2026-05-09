# Jolt Physics Integration Plan

## Context

You are adding the Jolt Physics library to an existing C++20 OpenGL FPS demo project. The project already builds with CMake and uses SDL3, GLAD, GLM, and stb_image. After this task, the project should compile and link against Jolt 5.5.0, with a minimal `PhysicsWorld` class that initializes Jolt and tears it down cleanly. **No gameplay physics yet** — that is a follow-up task. The success criterion for this task is: the program starts, prints "Jolt initialized", shuts down Jolt, and exits with code 0.

## Project location

`C:\projects\c++\shooter\project` (Windows). The project root contains `main.cpp`, a `src/` tree, and a `CMakeLists.txt`. Read `GameOutline.md` in the project root for the broader architecture before starting — your additions should fit the existing folder layout (a `src/physics/` directory is already planned for in the outline).

## Constraints

- Do not modify rendering, camera, or input code. This task is build-system + Jolt lifecycle only.
- Do not introduce any other dependencies.
- C++20 must remain enabled. Jolt is internally C++17 but must compile cleanly inside a C++20 target.
- Pin Jolt to release tag `v5.5.0`. Do not track `master`.

---

## Step 1 — Update CMakeLists.txt

Add Jolt via `FetchContent`. Jolt's `CMakeLists.txt` is in the `Build/` subdirectory of its repository, so `SOURCE_SUBDIR "Build"` is required — without it, configuration fails with a confusing "no CMakeLists found" error.

Insert this block **before** the `add_executable(...)` line:

```cmake
include(FetchContent)

# Jolt feature flags — must be set BEFORE FetchContent_MakeAvailable
# so they propagate into Jolt's own build. These same defines must be
# visible to game code that includes Jolt headers (handled below via
# add_compile_definitions, which applies project-wide).
add_compile_definitions(
    JPH_NO_FORCE_INLINE       # Avoid __forceinline issues if LTO is enabled
    JPH_DEBUG_RENDERER        # Enable debug shape rendering hooks
    JPH_PROFILE_ENABLED       # Enable internal profiler
)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.5.0
    SOURCE_SUBDIR  "Build"
)
FetchContent_MakeAvailable(JoltPhysics)
```

Then update the executable target so it can find Jolt headers and link the static library. Modify the existing `target_include_directories` and `target_link_libraries` calls (or add them if missing):

```cmake
target_include_directories(fps_demo PRIVATE
    src
    ${JoltPhysics_SOURCE_DIR}/..   # Jolt headers are under <root>/Jolt/...
)

target_link_libraries(fps_demo PRIVATE
    Jolt
    # leave existing SDL3, OpenGL, GLM links intact
)
```

**Why `${JoltPhysics_SOURCE_DIR}/..`:** Jolt's headers are included as `#include <Jolt/Jolt.h>`. `JoltPhysics_SOURCE_DIR` points at the `Build/` subfolder (because of `SOURCE_SUBDIR`), so the parent directory is what contains the `Jolt/` header tree.

---

## Step 2 — Create the PhysicsWorld class

Create two new files. The folder `src/physics/` may not exist yet; create it.

### `src/physics/PhysicsWorld.h`

```cpp
#pragma once

#include <memory>

namespace JPH {
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
}

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void update(float deltaTime);

    [[nodiscard]] JPH::PhysicsSystem& system() { return *m_system; }

private:
    std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> m_jobSystem;
    std::unique_ptr<JPH::PhysicsSystem> m_system;
};
```

### `src/physics/PhysicsWorld.cpp`

`Jolt/Jolt.h` MUST be the first Jolt include — it sets up platform macros that the rest of Jolt's headers depend on. Including any other Jolt header first will produce cryptic compile errors.

```cpp
#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <thread>

// --- Layer definitions ---------------------------------------------------
// Two object layers: static world geometry vs. moving bodies (player, etc).
namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING     = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr unsigned int NUM_LAYERS = 2;
}

// Maps object layers -> broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
    }
    unsigned int GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return m_objectToBroadPhase[layer];
    }
private:
    JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::NUM_LAYERS];
};

// Object layer vs. broadphase layer filter — which object layers can collide
// with which broadphase layers.
class ObjectVsBroadPhaseLayerFilterImpl final
    : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer layer1,
                       JPH::BroadPhaseLayer layer2) const override {
        switch (layer1) {
        case Layers::NON_MOVING: return layer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:     return true;
        default:                 return false;
        }
    }
};

// Object layer vs. object layer filter.
class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        switch (a) {
        case Layers::NON_MOVING: return b == Layers::MOVING;
        case Layers::MOVING:     return true;
        default:                 return false;
        }
    }
};

// Module-level singletons for filters. Jolt holds non-owning pointers to
// these for the lifetime of the PhysicsSystem, so they must outlive it.
static BPLayerInterfaceImpl              g_broadPhaseLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl g_objectVsBroadPhaseLayerFilter;
static ObjectLayerPairFilterImpl         g_objectLayerPairFilter;

// Optional but useful while bringing things up.
static void TraceImpl(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    std::cout << "[Jolt] " << buffer << '\n';
}

PhysicsWorld::PhysicsWorld() {
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Reserve one thread for the OS / main thread.
    const int threadCount = std::max(1,
        static_cast<int>(std::thread::hardware_concurrency()) - 1);
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threadCount);

    // Capacity numbers below are typical for a small demo — tune later.
    constexpr unsigned int cMaxBodies              = 1024;
    constexpr unsigned int cNumBodyMutexes         = 0;     // 0 = use default
    constexpr unsigned int cMaxBodyPairs           = 1024;
    constexpr unsigned int cMaxContactConstraints  = 1024;

    m_system = std::make_unique<JPH::PhysicsSystem>();
    m_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
                   cMaxContactConstraints,
                   g_broadPhaseLayerInterface,
                   g_objectVsBroadPhaseLayerFilter,
                   g_objectLayerPairFilter);

    m_system->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    std::cout << "Jolt initialized\n";
}

PhysicsWorld::~PhysicsWorld() {
    m_system.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsWorld::update(float deltaTime) {
    constexpr int collisionSteps = 1;
    m_system->Update(deltaTime, collisionSteps,
                     m_tempAllocator.get(), m_jobSystem.get());
}
```

---

## Step 3 — Wire it into main.cpp

Add a single inclusion and a single instance in `main.cpp`. Do **not** call `update()` yet — just confirm construction and destruction work. Place the instance early in `main()`, before the SDL/GL setup is fine.

```cpp
#include "physics/PhysicsWorld.h"

// ... inside main(), near the top:
PhysicsWorld physics;
// existing SDL init, window creation, main loop, etc.
```

---

## Step 4 — Build and verify

From the project root:

```
cmake -B build -S .
cmake --build build --config Release
```

The first configure will take several minutes — CMake clones Jolt and configures it. Subsequent builds are fast.

**Expected console output on launch:**
```
Jolt initialized
```
followed by whatever the existing program prints. The program should exit cleanly (no leaks reported, no crash on shutdown).

---

## Verification checklist

Before considering this task done, confirm all of the following:

1. `cmake -B build -S .` completes without errors.
2. `cmake --build build --config Release` completes without errors or warnings about Jolt headers.
3. Running the resulting executable prints `Jolt initialized` and exits with code 0.
4. The existing rendering/window/input code still works exactly as before (open the window, see whatever was there previously).
5. `git status` shows changes only to: `CMakeLists.txt`, `main.cpp`, and the new files `src/physics/PhysicsWorld.h` and `src/physics/PhysicsWorld.cpp`.

---

## Common failure modes and fixes

- **"Could not find a CMakeLists.txt" during FetchContent:** missing `SOURCE_SUBDIR "Build"` in `FetchContent_Declare`.
- **Linker errors mentioning `JPH::...` symbols:** `Jolt` not in `target_link_libraries`, or the `add_compile_definitions` block is below `FetchContent_MakeAvailable` (it must be above).
- **Compile errors from Jolt headers:** `#include <Jolt/Jolt.h>` is not the first Jolt include in the translation unit. Reorder includes.
- **Crash on shutdown:** the static filter objects (`g_broadPhaseLayerInterface` etc.) were declared inside a function instead of at namespace scope, so they were destroyed before `PhysicsSystem`. Keep them at file scope as shown.
- **Crash with a "JPH_ASSERT" message at startup:** mismatch between defines used to build Jolt and defines visible to game code. All `JPH_*` defines must be set via `add_compile_definitions` at the top of `CMakeLists.txt` (project-wide) so both Jolt and the game see the same configuration.

---

## Out of scope for this task

The following will be done in a separate task — do **not** implement them here:

- Creating any rigid bodies (plane, boxes).
- Character controller / `CharacterVirtual`.
- Replacing the camera's WASD movement with physics-driven movement.
- Debug rendering of collision shapes.
- Calling `physics.update(deltaTime)` in the main loop.

Stop after the program prints `Jolt initialized` and shuts down cleanly.