---
name: test-game
description: Run the shooter game's unit tests using test.bat. Use when the user asks to run tests, run the test suite, check that tests pass, or verify a change didn't break anything.
---

# Run the tests

Run this C++ shooter project's unit tests via its `test.bat` wrapper. Never invoke `cmake` or `ctest` directly — `test.bat` configures CMake with testing enabled, builds the test target, and runs the suite.

## How to run

Run the script from the project root:

```
test.bat
```

## What it does

- Configures CMake into `cmake-build-debug\` with `-DBUILD_TESTING=ON` (Ninja, Debug).
- Builds the `fps_demo_tests` target.
- Runs `ctest --output-on-failure`, so failing tests print their output.

## Reporting results

- On success, `ctest` reports `100% tests passed` — confirm all tests passed and note the count.
- On failure, the script exits non-zero. A failure can come from the configure step, the build step (compiler errors), or a failing test. Surface the actual error or the failing test's output so it can be fixed — don't just say "tests failed."
