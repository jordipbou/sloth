# Sloth — Universal Forth Engine

**Portable, multi-language virtual machine and ANS Forth 
implementation for that virtual machine for desktops, 
mobile, embedded and scripting.**

---

**Sloth** is a portable, multi-language, cross-platform Forth 
system.

It provides the minimal foundation needed to bootstrap 
**ANS Forth** and test it in any environment — desktop, mobile, 
or embedded.

It’s built on the idea that **Forth itself** — simple, extensible, 
and close to the machine — should be available anywhere.  
And not just across **platforms**, but also across **languages**.  
That’s why **Sloth** is also designed to be **embedded as a 
scripting language** within other applications.

**ANS Forth** also serves as a **common virtual machine and
high-level assembler**, enabling the creation of higher-level 
languages that remain portable, lightweight, and consistent.

---

## Key Features

- **Based on Forth:** Utilizes Forth’s dual-stack, linear memory 
  virtual machine for efficient computation from microcontrollers 
  to large  computers.  
- **Interactive programming:** Designed for interactive use, 
allowing immediate feedback and iteration.  
- **Extensible:** Easy to extend with additional backends and
  functionalities.  
- **Performance:** Words can be implemented on the host for
  performance-critical sections without the need to modify sloth
	code.
- **Embeddable:** Can be used as a scripting language in other
  applications.  
- **Minimal native implementation:** Most of the system is 
  implemented in Forth itself, allowing easy porting to other 
	platforms.  
- **Cross-language / cross-platform:** Sloth aims to run on as many
  platforms and programming languages as possible. Right now there
	are a C implementation and a Java implementation.
- **Easily hackable:** Every part of Sloth should be simple enough 
  for one developer to understand and modify for specific use cases.

---

## FAQ

**Why the name Sloth?**  
Sloths are beautiful animals. And I liked the play on words:
**“SLOw forTH.”**  
Later, I also thought of **“Scripting Language Of The Hell/Heavens”** —  
and it stuck.

**Why Forth?**  
These are interesting properties of some programming languages:

- **Interactive** like Lisp, Smalltalk, or Python  
- **Low-level** enough for microcontrollers (like C)  
- **Simple** enough to be implemented by a single developer  
- **No garbage collector** for realtime applications

Forth is the only language that fulfills all four.

---

## Memory Model

Continuous address space in all the implementations. C uses 
real native pointers and direct memory access. Java implementation
uses memory "blocks" and addresses have two parts, one to 
index the block and another as the address inside the block.

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
