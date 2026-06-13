---
name: build-game
description: Build the shooter game using build.bat. Use when the user asks to build, compile, or rebuild the game/project, or wants to check that the code compiles. Supports debug (default) and release builds.
---

# Build the game

Build this C++ shooter project by running its `build.bat` wrapper. Never invoke `cmake`, `msbuild`, or `cl` directly — `build.bat` sets up the MSVC environment (`vcvars64.bat`) and configures CMake on first run.

## How to build

Run the build script from the project root:

```
build.bat
```

For a release build, pass the `release` argument:

```
build.bat release
```

Use the Bash tool (the script works through it) or PowerShell. Debug is the default and is what you want for normal development and quick compile checks.

## What it does

- Debug builds go to `cmake-build-debug\`, release builds use a Debug-type config unless `release` is passed (then `BUILD_TYPE=Release`).
- On first run it configures CMake; subsequent runs only rebuild.
- The resulting executable is `cmake-build-debug\fps_demo.exe`.

## Reporting results

- On success, the script prints `[build] Done.` and the executable path — confirm the build succeeded and mention the exe path.
- On failure, the script exits non-zero and prints which stage failed (`CMake configure failed` or `Build failed`). Surface the actual compiler errors from the output so they can be fixed; don't just say "build failed."
