# SLOTH

A small, simple, relatively fast, double stack based virtual machine with a human readable/writeable bytecode.

Inspired by STABLE Forth, RetroForth/ilo, XY.

Features:

* One header only. Very easily embedabble in a C/C++ application.
* Human readable bytecode.
* Bytecode helpers for easier bootstrapping of higher level languages.
* Relatively fast interpreter.
* Ability to add extensions (C functions).
