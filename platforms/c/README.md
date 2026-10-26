# Sloth in C

Portable C89 implementation for MCUs, desktop, server and use as a C/C++ scripting language. It uses real native pointers and direct memory access for the [continuous address space](../README.md#memory-model).

The C implementation uses [CMake] to provide a cross-platform build system.

## Prerequisites

* **CMake 3.17+**
* **A C89-compatible compiler** (GCC, Clang, MSVC)
* **Ninja** (recommended)

## Recommended: Ninja Multi-Config

Ninja Multi-Config provides the same build workflow on Linux, Windows, and macOS.

From the repository root:

```sh
cmake -S platforms/c -B build -G "Ninja Multi-Config"
```

Then select the configuration when building:

```sh
cmake --build build --config Release
```

or:

```sh
cmake --build build --config Debug
```

Debug and Release builds can coexist in the same build directory.

## Configuration behavior

The `Debug` and `Release` configurations use different locations for the Forth scripts.

In **Debug** mode, `ROOT_PATH` points to the root of the Sloth repository. The interpreter therefore uses the scripts directly from the source tree. This means that changes to the scripts can be tested without rebuilding Sloth.

In **Release** mode, `ROOT_PATH` points to the build directory. The `4th/` directory from the source tree is copied there as part of the build.

This means a Release build is self-contained:

```text
build/
└── Release/
    ├── sloth
    └── 4th/
```

while a Debug build uses:

```text
sloth/
├── 4th/
├── platforms/
│   └── c/
│       └── ...
```

## Using another generator

CMake supports many different generators. You can use whichever generator is appropriate for your platform or development environment.

For example, Visual Studio:

```powershell
cmake -S platforms/c -B build -G "Visual Studio 16 2019"
cmake --build build --config Release
```

Xcode:

```sh
cmake -S platforms/c -B build -G Xcode
cmake --build build --config Release
```

Ordinary Ninja:

```sh
cmake -S platforms/c -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Unix Makefiles:

```sh
cmake -S platforms/c -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The difference is that **Ninja Multi-Config, Visual Studio and Xcode are multi-config generators**, while ordinary Ninja and Unix Makefiles are **single-config generators**.

With a multi-config generator, select the configuration when building:

```sh
cmake --build build --config Release
```

With a single-config generator, select it when configuring:

```sh
cmake -S platforms/c -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

## Running the REPL

On Linux and macOS:

```sh
./build/Release/sloth
```

On Windows:

```powershell
.\build\Release\sloth.exe
```

For single-config generators, the executable is normally located directly under `build`:

```sh
./build/sloth
```

## Building from WSL

When building the Windows version from a WSL checkout, use the `wsl.localhost` UNC path when invoking CMake from Windows.

For example, if the repository is located at:

```text
\\wsl.localhost\NixOS\home\jordi\factory\sloth
```

open PowerShell in:

```powershell
cd \\wsl.localhost\NixOS\home\jordi\factory\sloth\platforms\c
```

and configure:

```powershell
cmake -S . -B C:\build -G "Ninja Multi-Config"
```

Then build normally:

```powershell
cmake --build C:\build --config Release
```

**Do not use the older `\\wsl$\...` path when generating Ninja build files.** The `$` character is interpreted by Ninja and can result in a `bad $-escape` error.

## Build options

Options are passed as `-D<flag>=ON` during configuration:

| Option                          | Description                    |
| ------------------------------- | ------------------------------ |
| `SLOTH_WITHOUT_FLOATING_POINT`  | Disable floating point support |
| `SLOTH_WITHOUT_FILE_WORD_SET`   | Disable the FILE word set      |
| `SLOTH_WITHOUT_MEMORY_WORD_SET` | Disable the MEMORY word set    |

Example:

```sh
cmake -S platforms/c -B build -G "Ninja Multi-Config" \
    -DSLOTH_WITHOUT_FLOATING_POINT=ON
```

[CMake]: https://cmake.org/

## Testing

The C tests can be run directly with CTest:

```sh
cmake -S platforms/c -B build -G "Ninja Multi-Config"
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

`ctest` registers `sloth_unit`, `sloth_unit_fp` and `sloth_forth`. The last one runs `sloth --test` (the Forth 2012 suite and the floating point suite) and checks the reported failures against a baseline: `1` for the default build (the `-0.4999E FROUND` difference) and `0` without the floating point word set. The interactive Core ACCEPT test is run with a non-TTY stdin by `platforms/c/check_forth_tests.cmake`, so its `KEY` returns the return key and the test passes without touching the console. The baseline checker is `platforms/c/check_forth_tests.cmake`.

The repository-wide test scripts are described in the [main README](../../README.md#testing).

### Known failures

- `-0.4999E FROUND` differs from the reference suite on the C implementation; it is counted in the `sloth_forth` baseline. The Java and JavaScript implementations round half to even and pass it, so their `--test` baselines are `0`.
