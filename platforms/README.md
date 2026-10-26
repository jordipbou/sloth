# Platforms

Each platform implements the Sloth VM and loads the Forth sources from `4th/` unchanged. Build, run and test instructions live in the platform's own README:

- [C](c/README.md) — portable C89 for MCUs, desktop, server and as a C/C++ scripting language.
- [Java](java/README.md) — for Android, WearOS and as a Java scripting language.
- [JavaScript](js/README.md) — for web, server and embedded JS interpreters.
- [bash](bash/README.md) — self-contained implementation with no build step, experimental and slow.

The [host/VM interaction API](host-api.md) describes the interface a platform implements to run Sloth and to interoperate with the host.
