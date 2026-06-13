# Project Guidelines

## Build & Test

- Build the project with `build.bat` (do not invoke `cmake` / `msbuild` / `cl` directly).
- Run tests with `test.bat`.

## Project Layout

- Source code lives in the `src/` folder.
- Tests live in the `tests/` folder.

## Code Guidelines

- Prefer self-contained classes and namespaces — keep responsibilities encapsulated and minimize cross-module coupling.
- Avoid putting too much logic in `main.cpp`; it should mostly wire things together. Push real logic into dedicated classes/modules under `src/`.
- Structure logic so it can be easily unit tested — favor pure functions, dependency injection, and small testable units over hidden globals or side effects.
- Prefer `const` variables wherever possible — default to immutability and only drop `const` when mutation is actually required.
- Always use curly braces for control flow statements, even single-line `if` / `else` / `for` / `while` bodies. No brace-less one-liners.

## Naming Conventions

Apply these to **new code**. Existing code may not yet match — leave it alone unless you're already touching it for another reason.

- Functions and methods: `snake_case` (e.g. `apply_pending_state`, `check_hot_reload`).
- Classes, structs, enums, and other types: `PascalCase` (e.g. `Shader`, `GameConfig`, `NetRole`).
- Member variables: `m_camelCase` prefix (e.g. `m_planeMesh`, `m_activeState`) — matches existing codebase style.
- Local variables and parameters: `snake_case` (e.g. `vert_path`, `model_index`).
- Constants and `constexpr` values: `UPPER_SNAKE_CASE` (e.g. `MAX_PLAYERS`, `TICK_RATE`).
- Enum values (prefer `enum class`): `PascalCase` (e.g. `NetRole::ClientHost`).
- Namespaces: lowercase / `snake_case` (e.g. `physics`, `net`).
- Macros: `UPPER_SNAKE_CASE` — and avoid macros unless there is no reasonable alternative.
- Template parameters: `PascalCase` (e.g. `template<typename Element>`).
- File names: `PascalCase` matching the primary class (e.g. `Shader.h` / `Shader.cpp`).
- Boolean variables and members: prefix with `is_`, `has_`, or `should_` (e.g. `is_grounded`, `has_loaded`). Members follow the `m_` rule: `m_isGrounded`.
- Getters: drop the `get_` prefix (e.g. `position()`, not `get_position()`).
