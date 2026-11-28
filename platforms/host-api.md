# Host/VM interaction API

Sloth is designed to allow easy interoperation between host and Sloth.

## Stack API

push(v) -> pushes a value to the stack
pop() -> pops a value from the stack
pick(a) -> picks the value at TOP - a
place(a, v) -> places a value v at TOP - a

rpush(v) -> pushes an address to the return stack
rpop() -> pops an address from the return stack

## Execution API

eval(q) -> executes quotation/primitive q inside the VM
do([...]) -> executes host functions chained

## Input/output contract

The VM keeps two typed hooks, declared in `platforms/c/sloth_io.h`:

```c
int32_t input (X* x, int op);        /* KEY, KEY?, EKEY, EKEY? */
int32_t output(X* x, int op, uint8_t c); /* EMIT, EMIT? */
```

Input operations are `SLOTH_IO_KEY_READ` (returns a byte),
`SLOTH_IO_KEY_AVAILABLE` (flag), `SLOTH_IO_EKEY_READ` (event) and
`SLOTH_IO_EKEY_AVAILABLE` (flag). Output operations are
`SLOTH_IO_EMIT_WRITE` (consumes `c`) and `SLOTH_IO_EMIT_AVAILABLE`
(returns a flag, used by `EMIT?`). Install or replace them with
`sloth_set_input()` and `sloth_set_output()`.

The default input uses `getch`; the default output uses `printf`. The
desktop `sloth` executable installs a CPNBI input adapter when the
Facility word set is built. CPNBI EOF is mapped to the return key so
`ACCEPT` and `QUIT` terminate; `KEY_AVAILABLE` follows CPNBI's
"a read will not block" rule. `EKEY` uses the 32-bit CPNBI event
encoding described in `sloth_io.h`; a 16-bit CELL build would need a
compact profile or a token table (see the 16-bit SOMEDAY in
`~/hub/org/proj.sloth.org`).
