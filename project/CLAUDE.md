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
