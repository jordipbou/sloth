# Sloth — multi-language, cross-platform virtual machine and ANS Forth implementation.

It provides the minimal foundation needed to bootstrap **ANS Forth** and test it in any environment — desktop, mobile, or embedded.

It’s built on the idea that **Forth itself** — simple, extensible, and close to the machine — should be available anywhere. And not just across **platforms**, but also across **languages**. That’s why **Sloth** is also designed to be **embedded as a scripting language** within other applications.

---

## Key Features

- **Cross-language / cross-platform:** C, Java, JavaScript and bash implementations.
- **Simple VM:** Dual-stack, linear memory virtual machine that can be understood by one programmer.
- **Minimal native implementation:** Most of the system is implemented in Forth itself, allowing easy porting to other platforms.  
- **Embeddable:** Can be used as a scripting language and is designed to allow easy interoperation between host and Forth.
- **Performance:** Words can be implemented on the host for performance-critical sections without the need to modify sloth code.

---

## Layout

- `4th/` — Forth sources (`ans.4th`, `tools.4th`, ...). Most of Sloth is written here.
- `platforms/c/` — C implementation (C89) for MCUs, desktop, server and as a C/C++ scripting language.
- `platforms/java/` — Java implementation for Android, WearOS and as a Java scripting language.
- `platforms/js/` — JavaScript implementation for web, server and embedded JS interpreters.
- `platforms/bash/` — self-contained bash implementation with no build step (see `platforms/bash/README.md`).

---

## FAQ

**Why the name Sloth?**  
Sloths are beautiful animals. And I liked the play on words: **“SLOw forTH.”**  
Later, I also thought of **“Scripting Language Of The Hell/Heavens”** and it stuck.

**Why Forth?**  
These are interesting properties of some programming languages:

- **Interactive** like Lisp, Smalltalk, or Python  
- **Low-level** enough for microcontrollers (like C)  
- **Simple** enough to be implemented by a single developer  
- **No garbage collector** for realtime applications

Forth is the only language that fulfills all four.

---

## Memory Model

Continuous address space in all the implementations. C uses real native pointers and direct memory access. Java/JavaScript/bash implementation use memory "blocks" and addresses have two parts, one to index the block and another as the address inside the block.

---

## Building the C implementation

The C implementation uses [CMake] to provide a cross-platform build system.

### Prerequisites

* **CMake 3.17+**
* **A C89-compatible compiler** (GCC, Clang, MSVC)
* **Ninja** (recommended)

### Recommended: Ninja Multi-Config

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

### Configuration behavior

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

### Using another generator

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

### Running the REPL

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

### Building from WSL

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

### Build options

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

## Running the JavaScript implementation

There is no build step: the sources are ESM with no dependencies and run on
Node. `ROOT` is resolved from the module URL, so it can run from any directory.

```sh
node platforms/js/src/repl.js               # interactive REPL
node platforms/js/src/repl.js script.4th    # run a file
node platforms/js/src/repl.js --test        # Forth 2012 suite, baseline 0
```

The unit tests use the built-in Node test runner and must run from
`platforms/js`:

```sh
cd platforms/js
node --test
```

## Running the bash implementation

`platforms/bash/sloth.bash` is an implementation with no build step and no
dependency beyond bash (4+) for the core kernel and the MEMORY word set (only
bash builtins). The FLOAT word set additionally needs `awk` for arithmetic, and
the FILE word set needs common `stat`/`rm`/`mv`/`dd`/`od` tools. It is meant as a
fallback when no compiled binary or other interpreter is available, for example
when bootstrapping a system.

```sh
bash platforms/bash/sloth.bash                # interactive REPL
bash platforms/bash/sloth.bash program.4th    # run a file
bash platforms/bash/sloth.bash --test         # kernel self-tests (fast)
```

It boots `4th/ans.4th` unchanged, so all normal Forth words are available. Set
`SLOTH_ROOT` to the repository root if `4th/` is not found next to the script.
Booting `ans.4th` takes tens of seconds because the whole system is interpreted
by bash; usage, status and known gaps are documented in
`platforms/bash/README.md`.

## Testing

There is one entry point per OS. Both run every suite and report a single
pass/fail at the end:

- Linux: `./run-tests.sh`
- Windows (from WSL): `./run-tests-windows.sh`

They run, in order, the C unit tests and the Forth 2012 suite through CTest,
then the Java JUnit tests and the Forth 2012 suite through Gradle, then the
JavaScript unit tests and Forth 2012 suite through `node --test` and
`node platforms/js/src/repl.js --test`. `run-tests.sh` needs `java` on `PATH`
(or `JAVA_HOME`); without it the Java tests are skipped and the run fails. It
needs `node` on `PATH`, and without it the JavaScript tests are skipped. When
the Debug C binary is present, `run-tests.sh` also runs a differential check:
`platforms/js/test/differential.4th` must print identical output on the C and
JavaScript engines. Set `SLOTH_TEST_BASH=1` to additionally run the bash kernel
self-tests and the C-vs-bash differential check; this is off by default because
booting `ans.4th` in bash takes tens of seconds.

The C tests can also be run directly with CTest:

```sh
cmake -S platforms/c -B build -G "Ninja Multi-Config"
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

`ctest` registers `sloth_unit`, `sloth_unit_fp` and `sloth_forth`. The last one
runs `sloth --test` (the Forth 2012 suite and the floating point suite) and
checks the reported failures against a baseline: `1` for the default build (the
`-0.4999E FROUND` difference) and `0` without the floating point word set. The
interactive Core ACCEPT test is run with a non-TTY stdin by
`platforms/c/check_forth_tests.cmake`, so its `KEY` returns the return key and
the test passes without touching the console. The baseline checker is
`platforms/c/check_forth_tests.cmake`.

### Known failures

- `-0.4999E FROUND` differs from the reference suite on the C implementation; it
  is counted in the `sloth_forth` baseline. The Java and JavaScript
  implementations round half to even and pass it, so their `--test` baselines
  are `0`.
