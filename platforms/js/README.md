# Sloth in JavaScript

JavaScript implementation for web, server and embedded JS interpreters. It uses memory "blocks" for the [continuous address space](../README.md#memory-model).

## Usage

There is no build step: the sources are ESM with no dependencies and run on Node. `ROOT` is resolved from the module URL, so it can run from any directory.

```sh
node platforms/js/src/repl.js               # interactive REPL
node platforms/js/src/repl.js script.4th    # run a file
node platforms/js/src/repl.js --test        # Forth 2012 suite, baseline 0
```

## Testing

The unit tests use the built-in Node test runner and must run from `platforms/js`:

```sh
cd platforms/js
node --test
```

The Forth 2012 suite runs with `node platforms/js/src/repl.js --test` and expects a baseline of `0`. The cross-engine comparison in `platforms/js/test/differential.4th` is run by the repository-wide test scripts described in the [main README](../../README.md#testing).
