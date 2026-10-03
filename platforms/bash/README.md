# Sloth in bash

A self-contained Sloth implementation written in bash, with no build step and no
dependency beyond bash 4+ and its builtins. It exists so that a working Forth is
available even when no compiled binary or other interpreter can be run, for
example when bootstrapping a system.

The native kernel is ported from `platforms/js/src/sloth.js`: 32-bit signed
cells, byte-addressable memory, `xt < 0` selects a primitive and `xt > 0` is a
colon body. Most of Sloth is the Forth source in `4th/`, loaded unchanged.

## Requirements

- bash 4 or later (associative arrays).
- The core kernel and the MEMORY word set use only bash builtins.
- The FLOAT word set needs `awk` for arithmetic (bash has no floats); IEEE-754
  byte packing uses bash's own `printf %a`.
- The FILE word set needs `stat`, `rm`, `mv`, `dd` and `od` (coreutils or
  busybox equivalents) for size, delete, rename and binary reads.

## Usage

```sh
bash platforms/bash/sloth.bash                # interactive REPL
bash platforms/bash/sloth.bash program.4th    # run a file after ans.4th
bash platforms/bash/sloth.bash --test         # kernel self-tests (fast)
```

`4th/ans.4th` is booted before a file, the REPL or stdin is processed. The
location of `4th/` is resolved next to the script; set `SLOTH_ROOT` to the
repository root to override it.

## Environment variables

- `SLOTH_ROOT` — repository root containing `4th/`.
- `SLOTH_DEBUG` — print the name of an undefined token before throwing.
- `SLOTH_TRACE` — trace every token, `>IN`, length and compiler state.

## Status

- Boots `4th/ans.4th` unchanged, including its floating point sections
  (`(ENVIRONMENT)` reports a floating stack, so `ans.4th` defines `F,`,
  `FVARIABLE`, `FCONSTANT`, the float fields, ...).
- Word sets: core, MEMORY, FILE and FLOAT.
- `--test` runs the kernel self-tests (currently 28), covering the core,
  MEMORY (`ALLOCATE`/stores), FILE (create/write/close/open/read) and FLOAT
  (`F+`, `F*`, `FSQRT`, `FNEGATE`, `F@`/`F!`, `>FLOAT`).
- `platforms/js/test/differential.4th` produces byte-identical output to the C
  and JavaScript engines, covering `IF`, `DO`/`LOOP`, `RECURSE`, `S"`/`TYPE`,
  `VARIABLE`, `CREATE`/`ALLOT`, `/MOD`, `MIN`/`MAX`.
- A float program through the full system works, e.g.
  `1.5e 2.5e f+ f.` prints `4.` and `s" 3.14" >float f.` prints `3.14...`.

## Known gaps

- Booting `ans.4th` takes tens of seconds (measured on the development host).
- FLOAT: `SF@`/`SF!`/`DF@`/`DF!` are aliases of the double versions, and float
  formatting (`F.`/`FS.`/`FE.`/`F.`), `REPRESENT` and the infinity/NaN corners
  are approximations that aim to match the JavaScript engine but are not
  verified against the full Forth 2012 floating point suite.
- FILE: `READ-FILE` reads through `dd`+`od`, so it is binary-safe but slow.
- Includes are resolved by name and by the root `4th/` path, not relative to the
  including file, so the Forth 2012 suite (which uses relative includes) cannot
  be run directly yet.

## Design notes

- `CATCH`/`THROW` use cooperative unwinding: a global throw value is checked
  after every primitive and at the top of the inner interpreter.
- Symbol lookup uses a hash scoped per wordlist, with a linked-list fallback for
  shadowed (hidden) definitions, so search order is respected.
- Performance: bash command substitution costs on the order of hundreds of
  microseconds, so hot helpers write to globals instead of echoing, arithmetic
  is inlined in the primitives, and byte<->char conversion uses precomputed
  lookup tables. `LC_ALL=C` keeps character and byte indexing identical.
