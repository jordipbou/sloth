# Sloth C VM — implementation description

Sources: `platforms/c/sloth.h` (state/API), `platforms/c/sloth.c` (VM), `4th/ans.4th` (everything above the kernel).

## 1. VM state

The whole VM is one `X` struct (`sloth.h:141-162`):

```c
CELL d, dz;          /* dictionary base + size            */
CELL u, uz;          /* user-area base + size             */
CELL ip;             /* instruction pointer (byte address) */
CELL s[64], sp;      /* data stack                        */
CELL r[64], rp;      /* return stack                      */
FCELL f[64], fp;     /* float stack (optional)            */
jmp_buf jmpbuf[8];   /* CATCH nesting                     */
int jmpbuf_idx;
F key, emit;         /* host-overridable I/O primitives   */
sloth_P *p;          /* { capacity, used, fn-pointer[] }  */
```

`CELL` is `intptr_t`, so tokens/addresses are 64/32/16-bit depending on target (`sloth.h:82-113`). Defaults are 64 cells per stack, overrideable by macros. `sloth_new()` is `sloth_create(512, 524288, 1024)`: 512 primitive slots, 512 KiB dictionary, 1 KiB user area (`sloth.c:2546`).

There is no global state: every primitive takes `X*`, so several VMs can coexist in one process. `ip = -1` is the "not executing" sentinel (`sloth.c:2488-2492`).

## 2. Memory model

- The dictionary is one `calloc` block; the first three cells are `HERE`, the internal wordlist head and the FORTH wordlist head (`sloth.c:2504-2510`).
- All stored addresses are **absolute native pointers** into that block (`sloth_to_abs`/`sloth_to_rel` only translate the fixed dictionary-header offsets, `sloth.c:120-121`).
- `@ ! C@ C!` dereference raw pointers with no bounds or tag checks (`sloth.c:125-190`). This is why the C port can also touch arbitrary host memory, and why it is not sandboxed.
- The user area is a fixed offset map: 16 search-order cells, `BASE`, `STATE`, `IBUF`, `>IN`, `(ILEN)`, `(SOURCE-ID)`, `(SOURCE-POS)`, `(LATESTXT)`, `(INTERPRET)`, path buffers and float precision (`sloth.h:177-222`).
- `WORD` returns a counted string in a transient 64-byte buffer at `HERE + SLOTH_CBUF` (`sloth.c:839-892`), i.e. above the current dictionary top.

## 3. Words, headers, wordlists

Every word is a header in the dictionary, linked into a wordlist:

```
+0   link    previous header (absolute address, 0 = end)
+8   xt      execution token
+16  flags   uCHAR: bit0 hidden, bit1 immediate
+17  namelen uCHAR
+18  name    namelen bytes, then padding to cell alignment
```

The code starts immediately after the aligned name; the header's `xt` is that address (`sloth_header`, `sloth.c:333-348`). Note the block comment at `sloth.c:322-331` is stale: it still mentions a per-header wordlist cell and different offsets; the code is authoritative.

A wordlist is simply a cell holding the head pointer. `CURRENT` and each `CONTEXT[n]` store the address of such a cell (`sloth.c:277-283`, `sloth__search_word` at 684-706). Lookup is:

1. always the locals wordlist first (loop index `i = -1`),
2. then `CONTEXT[0 .. #ORDER-1]`,
3. each chain is walked through `link`,
4. names are compared case-insensitively (ASCII fold), hidden words skipped.

`FIND` returns `(xt, 1)` for immediate, `(xt, -1)` for normal, `(c-addr, 0)` for not found (`sloth.c:711-730`).

## 4. Execution tokens: the key encoding

An xt is one of two things (`sloth__execute`, `sloth.c:375-378`):

| xt value | meaning | dispatch |
|---|---|---|
| `q < 0` | primitive number | `p->p[-1-q]`, an index into the C function table |
| `q > 0` | absolute address of a secondary's threaded body | enter the token loop at `q` |
| `0` | unused / invalid | — |

Primitives are assigned by `sloth_primitive`: the first one registered is `-1`, the next `-2`, etc. (`sloth.c:2192-2197`). The table is a flat `F*` array, capacity 512 by default.

So a colon definition is a sequence of cells, each either a negative primitive number or a positive pointer to another sequence, terminated by `EXIT` (`-1` in the bootstrap ordering).

## 5. Threading model

**It is a sign-tagged, indirect/token-threaded interpreter — a hybrid, not pure ITC and not pure DTC.**

- **One cell per instruction.** The instruction stream is an array of `CELL`s in the dictionary.
- **Primitives are token indices** into a C function-pointer table and are dispatched indirectly through it.
- **Secondary words are direct pointers** to their token stream, so there is no code-field indirection for them.
- Dispatch is a per-token sign test plus an indirect C call, not a `switch` loop, computed goto, inline cache, or native compilation.

The whole inner interpreter is `sloth.c:350-390`:

```c
CELL sloth_op(X* x) { CELL o = sloth_fetch(x, x->ip); x->ip += sCELL; return o; }

void sloth__do_prim(X* x, CELL p) { (x->p->p[-1 - p])(x); }

void sloth__call(X* x, CELL q) {
    if (x->ip >= 0 || x->rp > 0) sloth_rpush(x, x->ip);
    x->ip = q;
}

void sloth__execute(X* x, CELL q) {
    if (q < 0) sloth__do_prim(x, q);
    else sloth__call(x, q);
}

void sloth__inner(X* x) {
    CELL t = x->rp;
    while (t <= x->rp && x->ip >= 0) {
        sloth__execute(x, sloth_op(x));
    }
}

void sloth_eval(X* x, CELL q) {
    sloth__execute(x, q);
    if (q > 0) sloth__inner(x);
}
```

Consequences of that design:

- **Forth calls do not recurse on the C stack.** `sloth__call` pushes `ip` on the VM return stack and jumps; `EXIT` pops it (`sloth.c:422`). The C stack is used only when a built-in re-enters the VM (`EXECUTE`, `EVALUATE`, `INTERPRET` under `CATCH`, `REFILL`→`ACCEPT`, `INCLUDED`), each of which starts a fresh `sloth__inner` loop.
- **Loop termination is return-stack-depth based.** `t = rp` at entry; the loop ends when a return pops below the frame that entered it, or when `ip` becomes `-1`. So an xt can only be run "until it returns", which is what lets `sloth_eval` be used as a synchronous host call.
- **No tail-call optimisation, no inlining, no superinstructions.** Every word call costs a return-stack push, and every primitive costs a table lookup plus an indirect call.
- **Code is not relocatable.** Positive tokens are absolute addresses; there is no image save/load path.
- `sloth_eval` can also run a primitive directly (`q < 0`) without starting the loop; that is the host's entry point.

The C port has **only** this threaded interpreter. The word-to-native compilation mentioned in `proj.sloth.org` is the JS/ZeppOS execution mode, not part of the C VM.

## 6. The instruction set (the "bytecode")

There is no opcode enum; the VM knows only the primitives, of which these are structural:

| primitive | encoding | semantics |
|---|---|---|
| `EXIT` | — | `ip = rp ? rpop() : -1` |
| `(LIT)` | inline cell | push the next cell |
| `(FLIT)` | inline `FCELL` | push to float stack |
| `(RIP)` | inline cell offset | pushes `ip + offset - cell`; used by `CREATE` to compute the body address |
| `(BRANCH)` | inline cell `o` | `ip = disp_cell + o` (o is target − disp address) |
| `(?BRANCH)` | inline cell `o` | pop flag; zero → branch as above, else skip the cell |
| `(STRING)` | inline length + bytes, padded | push body address and length, skip to aligned end (`sloth.c:633-643`) |
| `(CSTRING)` | counted string, padded | same for `C"`-style literals |
| `(QUOTATION)` | inline byte distance | push body address and jump over it |

The branch formula in `sloth_branch_` is `x->ip += sloth_op(x) - sCELL`: `ip` has already advanced past the displacement cell, and the displacement convention is `target − address_of_displacement_cell`, so the two corrections cancel to `ip = target` (`sloth.c:440-445`). The same expression relies on `sloth_op`'s write to `ip` being evaluated before the compound assignment reads it — correct in practice on the compilers used, but formally an unsequenced read/write of `x->ip`.

## 7. Compilation and defining words

All compilation is in the primitives, in Forth-idiomatic style: build tokens with `,`, patch inline cells with `!`.

- `:` parses a name, creates the header, sets `(LATESTXT)`, marks the word hidden, sets `STATE` to 1 (`sloth.c:1229-1241`).
- `;` appends `EXIT`, clears `STATE`, unhides the word (`sloth.c:1252-1258`).
- `:NONAME` pushes `HERE` as an anonymous xt and compiles without a header (`sloth.c:1242-1251`).
- `RECURSE` compiles `(LATESTXT)`.
- `POSTPONE`: immediate words are compiled directly; normal words become `LITERAL xt` + `COMPILE,` (`sloth.c:1265-1286`).
- `COMPILE,` is just `,` of an xt (`sloth.c:734`, `1288`).
- `LITERAL` emits `(LIT)` and the value (`sloth.c:736-742`).
- `CREATE` emits the 4-cell prologue `(RIP), 4*sCELL, EXIT, EXIT` (`sloth.c:1293-1317`). Executing it pushes the body address, then returns. The comment above it says 5 cells and three `EXIT`s; the code emits 4 and two, and `>BODY` in `ans.4th` is `4 CELLS +`, which matches the code, not the comment.
- `DOES>` compiles `LITERAL body-xt`, `(DOES)`, `EXIT`; at runtime `(DOES)` patches the first `EXIT` of the most recently created word with the does-body xt, i.e. self-modifying threaded code (`sloth.c:1330-1350`).
- User variables are ordinary threaded words: header, then `(LIT) (u+d) EXIT`, i.e. they push their user-area address (`sloth.c:2200-2205`).
- `[: ... ;]` compiles a quotation. `STATE` is used as a signed nesting counter: positive while compiling a definition, negative while compiling a top-level quotation, and `;]` compiles `EXIT`, patches the skip distance, restores `(LATESTXT)` and `STATE` (`sloth.c:752-791`). At interpret level the body address is left on the stack.

Supplementary structure comes from `ans.4th`, not the VM: `IF/ELSE/THEN`, `BEGIN/UNTIL/WHILE/REPEAT`, `DO/LOOP`, strings, etc., all built from the inline primitives above.

## 8. Outer interpreter, sources and REPL

`(INTERPRET)` is itself a user variable holding the xt of `sloth_interpret_`, so `EVALUATE` and `INCLUDED` can `CATCH` it and restore the input state (`sloth.c:2278`, `1391-1530`).

The outer loop:

1. `WORD(32)` extracts a token into the counted buffer at `HERE+64`.
2. `FIND` looks it up.
3. In interpret state (`STATE = 0`) or for an immediate word while compiling, the xt is executed; otherwise it is compiled.
4. If not found, the token is parsed as a number: char literal `'c'`, optional `+/-`, base prefixes `#`, `$`, `%`, `0x`, leading `0` as octal, trailing `.` for a double; otherwise `strtod` when `BASE = 10`; failure is `THROW -13` (`sloth.c:1413-1530`).

Source state is `IBUF`, `>IN`, `(ILEN)`, `(SOURCE-ID)`, `(SOURCE-POS)`. `SOURCE-ID` is `0` for the terminal, `-1` for `EVALUATE`, and a `FILE*` cast to `CELL` for files. `REFILL` branches on that: `-1` → false, `0` → call `ACCEPT`, file → `READ-LINE`; it also records the file position for nested includes (`sloth.c:954-1007`).

`INCLUDED` saves input and path state, opens the file against cwd/root, interprets it line by line through `REFILL`, prints file and line on an exception, then rethrows; `EVALUATE` saves/restores `IBUF`, `>IN`, `ILEN`, `SOURCE-ID` around a `CATCH` (`sloth.c:1044-1211`, `1355-1389`).

`sloth_run` chooses: if stdin is not a TTY, interpret stdin line by line; otherwise run the Forth `QUIT` word (`sloth.c:2691-2715`). `QUIT` (`ans.4th:1426`) empties the return stack, sets `SOURCE-ID` to 0, and loops `REFILL` / `['] INTERPRET CATCH`, printing `OK`, `Compiling`, or the exception number.

`EMIT` and `KEY` are per-VM function pointers, defaulting to `printf` and a `getch` that toggles termios on each key; hosts can replace them (`sloth.c:804-829`).

## 9. Exceptions

`CATCH` snapshots `sp`, `rp`, `ip`, then `setjmp`s into `jmpbuf[idx]` and evaluates the xt; `THROW` does a `longjmp` to the innermost buffer (`sloth.c:449-495`). There are 8 nesting slots. If nothing catches, `sloth_throw` prints the current input buffer and remaining token and calls `exit(e)`. Recovery restores all three stack pointers, which is possible precisely because the return stack holds the Forth call chain.

## 10. Bootstrap

`sloth_bootstrap` registers the kernel and then the optional word sets:

- kernel: 71 named primitives plus 18 user variables (`sloth.c:2214-2311`), including `EXIT`, `(LIT)`, `(RIP)`, branches, stacks, memory, exceptions, defining words, `EVALUATE`, `EXECUTE`, `DEBUG`, `FIND`, quotations, and `(ENVIRONMENT)`;
- floating point: 65 words (`sloth.c:2315-2466`), optional;
- memory and file word sets live in `memory.c` and `file.c` and are registered by the executable, not the VM.

Then the host includes `4th/ans.4th`; from that point the outer interpreter, `QUIT`, control structures and loops are Forth. `DEBUG` (`sloth.c:394-419`) is a primitive that runs three host-supplied xts (pre/instruction/post) and an inner-loop variant for tracing; `4th/debug.4th` builds `TRACE:` on it.

## 11. Loops: why a separate stack

`ans.4th` implements `DO ?DO I J K LEAVE UNLOOP LOOP +LOOP` entirely in Forth, using:

- a 90-cell `(LOOP-STACK)` with 3 cells per frame (limit, index, body quotation),
- `(DOLOOP)` executing the body quotation with `EXECUTE` on every iteration,
- pForth's overflow test in `(LOOPCHECK)`,
- `I`/`J`/`K` reading the loop stack directly,
- `LEAVE` setting `(LEAVE)` and doing `R> DROP EXIT` to unwind.

The design reason is in `proj.sloth.org`: keeping indices on the return stack would have required `RPICK` and would make `I`/`J` unusable in words called from the loop. The cost is one extra `EXECUTE` indirection per iteration. It is conditional-compiled with `?:` / `?\`, so a port that provides native loop primitives skips it.

## 12. Properties and rough edges worth knowing

- **Portable but pointer-bound.** The VM is C89, context-parameterised, and small (one interpreter loop, a table of primitives), but compiled code is machine-address threaded and therefore not relocatable or persistable.
- **8 bytes per instruction on 64-bit.** One `CELL` per token is simple but memory-hungry compared with bytecode.
- **Dispatch is not optimal.** Each token goes through `sloth_op` → sign test → `sloth__do_prim` → function pointer. There is no threaded-code "next" macro or caching.
- **Unchecked call-frame overflow.** `>R` checks the return stack, but `sloth__call` pushes the return address without any bound check (`sloth.c:370-373`); deep Forth recursion past `SLOTH_RETURN_STACK_SIZE` writes past `x->r`.
- **Unchecked data-stack writes in places.** `push`/`pop` do not check; primitives call `sloth__check_data_stack` individually, and some internal paths push without it.
- **No memory protection.** `@ !` dereference arbitrary pointers; only the dictionary's `HERE`/size is tracked, via `UNUSED`.
- **Single memory block.** `proj.sloth.org` lists multiple memory blocks as a TODO; today the model is one contiguous block.
- **Stale comments.** The header layout comment and the `CREATE` cell-count comment no longer match the code; the code and `>BODY` agree with each other.
- **Two sequence points of subtlety** in branch dispatch as noted above, and `sloth_eval`'s "run until this frame returns" contract, which is what makes nested `EXECUTE`/`EVALUATE` work without C recursion.
