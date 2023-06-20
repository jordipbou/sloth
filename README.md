# SLOTH

A small, simple, relatively fast, double stack based virtual machine with human readable/writeable bytecode.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

Features:

* One header only. Very easily embedabble in a C/C++ application.
* Human readable bytecode.
* Bytecode helpers for easier bootstrapping of higher level languages.
* Relatively fast interpreter.
* Ability to add extensions (C functions).
