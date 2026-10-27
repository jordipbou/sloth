# Sloth — multi-language, cross-platform virtual machine and ANS Forth implementation.

It provides a minimal foundation to bootstrap **ANS Forth** and test it in as many environments as possible — desktop, mobile, or embedded.

It’s built on the idea that **Forth itself** — simple, extensible, and close to the machine (real or virtual) — should be available across **platforms** and **languages**. That’s why **Sloth** is also designed to be **embedded as a scripting language** within other applications.

## Key features

- **Cross-language / cross-platform:** portable C89, Java, JavaScript and bash implementations.
- **Simple VM:** Dual-stack, linear memory, sign-tagged token threading virtual machine that can be understood by one programmer.
- **Minimal native implementation:** Only the VM and bootstrap are native; everything above them is Forth, shared by every platform.  
- **Embeddable:** Can be used as a scripting language and is designed to allow easy interoperation between host and Forth.
- **Performance:** Words can be implemented on the host for performance-critical sections without the need to modify sloth code.

---

## Layers

Sloth is three layers:

1. **VM** — the machine: two stacks (plus a float stack), one linear dictionary, sign-tagged token threading, calls, returns and exceptions. It has no parser and no compiler; it only fetches tokens and calls primitives.
2. **Bootstrap** — the native words that turn the VM into a minimal Forth: the outer interpreter, the compiler and defining words, word lookup, and the rest of the kernel. It is written against the VM and gives the Forth layer a language to build on.
3. **Forth** — `4th/`: control structures, loops, strings, tools and everything else up to full ANS Forth, written only with the words the bootstrap provides.

The C, Java, JavaScript and bash implementations are VM-plus-bootstrap pairs. The Forth layer is common to all of them: it stays fixed while a VM and its bootstrap are swapped, compared and simplified. The seam is `ans.4th`, plus late-bound hooks such as `(INTERPRET)` and `ACCEPT`.

The goal is to find the simplest and fastest VM and bootstrap while keeping the Forth layer common, and to let each platform be the best host it can be without forking the language.

---

## Layout

- `4th/` — the common Forth layer (`ans.4th`, `tools.4th`, ...), shared by every platform.
- [`platforms/c/`](platforms/c/README.md) — C89 VM and bootstrap for MCUs, desktop, server and as a C/C++ scripting language.
- [`platforms/java/`](platforms/java/README.md) — Java VM and bootstrap for Android, WearOS and as a Java scripting language.
- [`platforms/js/`](platforms/js/README.md) — JavaScript VM and bootstrap for web, server and embedded JS interpreters.
- [`platforms/bash/`](platforms/bash/README.md) — self-contained bash VM and bootstrap with no build step -experimental, slow-.

The [host/VM interaction API](platforms/host-api.md) describes the interface each VM and bootstrap implements.

---

## Quick start

Each platform README documents its own build, run and test steps. From the repository root:

| Platform   | Run                                                                                                                       |
| ---------- | ------------------------------------------------------------------------------------------------------------------------- |
| C          | `cmake -S platforms/c -B build -G "Ninja Multi-Config" && cmake --build build --config Release && ./build/Release/sloth`   |
| Java       | `cd platforms/java && ./gradlew run`                                                                                       |
| JavaScript | `node platforms/js/src/repl.js`                                                                                            |
| bash       | `bash platforms/bash/sloth.bash`                                                                                           |

---

## Memory Model

Continuous address space in all the implementations. C uses real native pointers and direct memory access. Java/JavaScript/bash implementation use memory "blocks" and addresses have two parts, one to index the block and another as the address inside the block.

---

## Testing

There is one entry point per OS. Both run every suite and report a single pass/fail at the end:

- Linux: `./run-tests.sh`
- Windows (from WSL): `./run-tests-windows.sh`

They run, in order, the C unit tests and the Forth 2012 suite through CTest, then the Java JUnit tests and the Forth 2012 suite through Gradle, then the JavaScript unit tests and Forth 2012 suite through `node --test` and `node platforms/js/src/repl.js --test`. `run-tests.sh` needs `java` on `PATH` (or `JAVA_HOME`); without it the Java tests are skipped and the run fails. It needs `node` on `PATH`, and without it the JavaScript tests are skipped. When the Debug C binary is present, `run-tests.sh` also runs a differential check: `platforms/js/test/differential.4th` must print identical output on the C and JavaScript engines. Set `SLOTH_TEST_BASH=1` to additionally run the bash kernel self-tests and the C-vs-bash differential check; this is off by default because booting `ans.4th` in bash takes tens of seconds.

Direct per-platform commands and their known failures are documented in each platform README: [C](platforms/c/README.md#testing), [Java](platforms/java/README.md#testing), [JavaScript](platforms/js/README.md#testing), [bash](platforms/bash/README.md).

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
