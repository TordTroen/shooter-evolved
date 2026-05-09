# C++ Code Standard

This document defines the coding conventions for this project. All new code and edits by AI sessions must follow these rules.

---

## Language Version

- **C++20** minimum. Prefer modern idioms over legacy workarounds.
- Compiler: MSVC via CMake. Treat warnings as errors.

---

## Naming

| Construct | Convention | Example |
|---|---|---|
| Class / struct / enum class | `PascalCase` | `PhysicsWorld`, `CharacterController` |
| Function / method | `camelCase` | `update()`, `isOnGround()` |
| Member variable | `m_` + `camelCase` | `m_character`, `m_tempAllocator` |
| Static / global constant | `k` + `PascalCase` | `kMoveSpeed`, `kCapsuleRadius` |
| Local variable | `camelCase` | `deltaTime`, `wishVelocity` |
| Namespace | `PascalCase` or short all-lower | `Layers`, `JPH` |
| Enum value | `PascalCase` | `EGroundState::OnGround` |
| Macro (avoid; use constexpr) | `ALL_CAPS` | `JOLT_USE_STD_VECTOR` |

- Do **not** use Hungarian notation (no `bFlag`, `iCount`, `pPtr`) except the `m_` member prefix.
- Prefer descriptive names. Single-letter names are only acceptable for loop indices and trivial lambdas.
- Acronyms follow case rules: `glslId` not `GLSLId`, `m_vao` not `m_VAO` (unless the identifier is all-caps by external convention, e.g. SDL macros).

---

## Files

- **Header extension**: `.h`
- **Source extension**: `.cpp`
- One class per file; filename matches the class name exactly: `PhysicsWorld.h` / `PhysicsWorld.cpp`.
- All headers use `#pragma once` — never `#ifndef` guards.

### Include Order (within a `.cpp`)

1. Matching header (own `.h`)
2. Other project headers
3. Third-party library headers (Jolt, SDL, GLAD, GLM, …)
4. Standard library headers

Each group separated by a blank line. Within a group, alphabetical order is preferred but not required.

```cpp
#include "CharacterController.h"
#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
```

- Prefer **forward declarations** over includes in headers whenever the full type is not needed.

---

## Classes

```cpp
class Foo
{
public:
    Foo();
    ~Foo();

    // Delete copy; implement move only when useful.
    Foo(const Foo&) = delete;
    Foo& operator=(const Foo&) = delete;

    void doThing(float dt);

    [[nodiscard]] int value() const { return m_value; }

private:
    int m_value = 0;
};
```

- **Section order**: `public` → `protected` → `private`.
- Explicitly `= delete` copy constructor and copy-assignment for classes that own resources.
- Mark getters `[[nodiscard]]`.
- Prefer initialiser lists over assignment in constructors. Member initialisation order must match declaration order.
- Trivial getters may be defined inline in the header. Non-trivial bodies go in the `.cpp`.

---

## Constants & Magic Numbers

- Use `constexpr` instead of `#define` for all constants.
- Name every magic number. No bare numeric literals in logic — only in constant definitions.

```cpp
// Good
static constexpr float kCapsuleRadius = 0.3f;
static constexpr int   kMaxEntities   = 1024;

// Bad
float r = 0.3f;
```

---

## Types

- Use `int32_t`, `uint32_t`, `uint64_t`, etc. from `<cstdint>` when the size matters.
- Use `std::size_t` for sizes and counts.
- Prefer `float` over `double` for game math unless precision is explicitly required.
- Never use `unsigned int` when you mean "non-negative index" — use a signed type to avoid wrapping bugs, or use a named typedef.

---

## Ownership & Memory

- **No raw `new`/`delete`** in application code. Use `std::unique_ptr` for sole ownership, `std::shared_ptr` only when shared ownership is genuinely required.
- Pass by raw pointer or reference to express *non-owning* access.
- Prefer stack allocation. Heap-allocate only when lifetime or size demands it.
- `std::vector` is the default container. Only reach for others (`std::unordered_map`, `std::deque`, …) when you have a specific reason.

```cpp
// Ownership: unique_ptr
std::unique_ptr<JPH::PhysicsSystem> m_system;

// Non-owning reference: raw reference or pointer
void update(PhysicsWorld& world);
```

---

## Functions

- Keep functions short. If a function doesn't fit on screen, it probably does too much.
- Return values are preferred over output parameters. Use structured bindings or small structs when multiple values are needed.
- `const`-correct everything: member functions that don't mutate state must be `const`.
- Mark functions `[[nodiscard]]` when ignoring the return value is almost always a bug.
- Prefer `static` free functions over methods for logic that doesn't need `this`.

---

## Casts

- **Never use C-style casts** `(int)x`. Always use named casts.
- `static_cast<T>()` for safe conversions.
- `reinterpret_cast<T>()` only when interfacing with C APIs (OpenGL, etc.).
- `const_cast` only to interface with legacy C APIs — never to discard `const` in your own code.

---

## `auto`

- Use `auto` when the type is:
  - Obvious from the right-hand side (`auto it = vec.begin();`)
  - A lambda or complex iterator/template type
- Avoid `auto` when the type is not immediately apparent from context. Explicit types aid readability.

---

## Control Flow

- Always use braces `{}` on `if`/`else`/`for`/`while`, even for single-statement bodies.
- Prefer early returns to deep nesting.
- Use range-for over index-for when the index is not needed.

```cpp
// Good
for (const auto& entity : entities)
{
    ...
}

// Use index only when needed
for (int i = 0; i < count; ++i)
{
    ...
}
```

---

## Comments

- **Write comments only for the *why*, never the *what*.** Well-named identifiers document themselves.
- Comments explain hidden constraints, subtle invariants, or workarounds for external bugs.
- Keep comments short — one line max in most cases.
- Do not leave TODO/FIXME comments in committed code unless they reference a tracked issue.

```cpp
// Offset the shape so the character position represents feet, not capsule centre.
JPH::RotatedTranslatedShapeSettings(...)
```

---

## Formatting

- **Indent**: 4 spaces. No tabs.
- **Line length**: soft limit 120 characters.
- **Brace style**: Allman — opening brace always on its own line.
- **Pointer/reference declarator**: attached to the type, not the variable name.

```cpp
void foo(const PhysicsWorld& world, float* outResult);
```

- Align related declarations vertically when it improves readability (e.g., column-aligned member variables or initialisers), but don't force it.
- One statement per line.
- Leave one blank line between methods.

---

## Namespaces

- Use namespaces to group related free functions and types (e.g., `Layers`, `Physics`).
- Do **not** `using namespace` in headers. In `.cpp` files it is acceptable for narrow, well-known namespaces (`using namespace glm;`) but avoid it for large namespaces like `std`.
- Do not place `using namespace` at file scope in headers — ever.

---

## Error Handling

- Prefer returning explicit error states (`bool`, enum, `std::optional`) over exceptions for recoverable errors.
- Use `assert()` (or Jolt's `JPH_ASSERT`) for invariants that must hold in correct code — not for user-facing validation.
- Log errors with `std::cerr` prefixed with `[ClassName]` for easy filtering.
- Do not silently swallow errors. Either handle, log, or propagate.

---

## Third-Party Libraries (this project)

| Library | Purpose | Namespace / Prefix |
|---|---|---|
| Jolt Physics 5.5 | Physics simulation | `JPH::` |
| SDL 3 | Window, input, OS | `SDL_` prefix (C API) |
| GLAD | OpenGL loader | `gl` prefix (C API) |
| GLM | Math (vectors, matrices) | `glm::` |
| stb | Image / font utilities | `stb_` prefix (C API) |

- Always include `<Jolt/Jolt.h>` before any other Jolt header.
- Prefer GLM types (`glm::vec3`, `glm::mat4`) for game-side math; convert to Jolt types at the physics boundary.
