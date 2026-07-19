# Linux / WSL build setup for `fps_server`

One-time setup steps to get `./build.sh` working on Linux (including WSL running
against the Windows checkout at `/mnt/c/Projects/C++/ShooterEvolved`). Only the
headless `fps_server` target builds on Linux — `fps_demo` stays Windows-only.

## 1. Install build tools

```sh
sudo apt update
sudo apt install -y build-essential ninja-build zip unzip pkg-config
```

- `build-essential` — provides `gcc`/`g++` (CMAKE_C_COMPILER / CMAKE_CXX_COMPILER).
- `ninja-build` — provides the `ninja` binary (`build.sh` configures CMake with `-G Ninja`).
- `zip`, `unzip`, `pkg-config` — required by vcpkg to fetch/build manifest dependencies.
- `git`, `curl`, `tar` are typically already present on WSL/Ubuntu; verify with
  `which git curl tar` if a later step fails.

## 2. Set up vcpkg

`CMakeLists.txt` looks for a vcpkg toolchain file in this order:

1. `$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake` (env var), or
2. `<project>/../vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake` (sibling checkout).

This repo already uses the sibling-checkout convention on Windows
(`ShooterEvolved/vcpkg/vcpkg`). Match it on Linux with a separate, Linux-native
vcpkg checkout in the same location (the Windows vcpkg binaries are MSVC-built and
can't be reused):

```sh
cd /mnt/c/Projects/C++/ShooterEvolved/vcpkg
git clone https://github.com/microsoft/vcpkg.git vcpkg
./vcpkg/bootstrap-vcpkg.sh
```

No `VCPKG_ROOT` env var needed — CMake finds it via the sibling path automatically.

## 3. Stale CMake cache

`cmake-build-debug` must be deleted and regenerated any time the environment
changes underneath it, because `CMAKE_TOOLCHAIN_FILE` and other config are cached
in `CMakeCache.txt` on first configure and are **not** re-evaluated on later runs:

- If it was ever configured from the other OS (e.g. Windows CMake ran against the
  same directory mounted in WSL), CMake refuses to reconfigure ("CMakeCache.txt
  directory ... is different than the directory ... where it was created").
- If it was configured *before* vcpkg existed at `../vcpkg/vcpkg` (e.g. during
  earlier setup attempts), the toolchain file path never got recorded, and
  `find_package(glm ...)` etc. fail even after vcpkg is installed correctly.

Fix in both cases: delete the build directory and let `build.sh` regenerate it.

```sh
rm -rf cmake-build-debug
./build.sh
```

## 4. Build

```sh
./build.sh          # Debug
./build.sh release  # Release
```
