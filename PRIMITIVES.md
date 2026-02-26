# Minimal Required Primitives to Bootstrap Forth (from `platforms/c/sloth.h`)

This document outlines the C functions defined in `sloth.h` that form the fundamental layer of the Sloth Forth interpreter. These functions are categorized into Core VM Operations, Primitive Forth Words, and Essential Helper Functions, representing the minimal set required to initialize and run a basic Forth environment.

### 1. Core VM Operations

These functions are the bedrock of the Sloth Virtual Machine, managing its internal state, stacks, and memory. They are not directly exposed as Forth words but are critical for the VM's overall functionality.

*   **Context Initialization/Destruction:**
    *   `void sloth_init(X* x, CELL d, CELL sz, CELL u, CELL uz)`: Initializes an existing VM context `x` with specified dictionary and user area parameters.
    *   `X* sloth_create(int psize, int dsize, int usize)`: Allocates and initializes a new VM context `X` with given primitive table, dictionary, and user area sizes.
    *   `X* sloth_new()`: A convenience function to create a new VM with default sizes.
    *   `void sloth_free(X* x)`: Frees the memory allocated for a VM context.

*   **Data Stack Management:**
    *   `void sloth_push(X* x, CELL v)`: Pushes a `CELL` value onto the data stack.
    *   `CELL sloth_pop(X* x)`: Pops and returns a `CELL` value from the data stack.
    *   `CELL sloth_pick(X* x, CELL a)`: Returns the `CELL` at a specific depth `a` from the top of the data stack without removing it.

*   **Return Stack Management:**
    *   `void sloth_rpush(X* x, CELL v)`: Pushes a `CELL` value onto the return stack.
    *   `CELL sloth_rpop(X* x)`: Pops and returns a `CELL` value from the return stack.
    *   `CELL sloth_rpick(X* x, CELL a)`: Returns the `CELL` at a specific depth `a` from the top of the return stack without removing it.

*   **Raw Memory Access:**
    *   `void sloth_bstore(X* x, CELL a, BYTE v)` / `BYTE sloth_bfetch(X* x, CELL a)`: Store/fetch a byte at absolute address `a`.
    *   `void sloth_cstore(X* x, CELL a, uCHAR v)` / `uCHAR sloth_cfetch(X* x, CELL a)`: Store/fetch an unsigned character at absolute address `a`.
    *   `void sloth_wstore(X* x, CELL a, WYDE v)` / `WYDE sloth_wfetch(X* x, CELL a)`: Store/fetch a 16-bit word at absolute address `a`.
    *   `void sloth_lstore(X* x, CELL a, LONG v)` / `LONG sloth_lfetch(X* x, CELL a)`: Store/fetch a 32-bit long at absolute address `a`.
    *   `void sloth_xstore(X* x, CELL a, EXTENDED v)` / `EXTENDED sloth_xfetch(X* x, CELL a)`: Store/fetch a 64-bit extended value (or `CELL`-sized) at absolute address `a`.
    *   `void sloth_store(X* x, CELL a, CELL v)` / `CELL sloth_fetch(X* x, CELL a)`: Type-dependent wrappers for storing/fetching a `CELL` at absolute address `a`.
    *   `CELL sloth_to_abs(X* x, CELL a)`: Converts a relative dictionary address to an absolute memory address.
    *   `CELL sloth_to_rel(X* x, CELL a)`: Converts an absolute memory address to a relative dictionary address.

*   **Inner Interpreter:**
    *   `CELL sloth_op(X* x)`: Fetches the next operation (XT or literal) from the instruction pointer and advances it.
    *   `void sloth__do_prim(X* x, CELL p)`: Executes a primitive C function identified by its index `p`.
    *   `void sloth__call(X* x, CELL q)`: Initiates execution of a Forth word at XT `q` by setting the instruction pointer and pushing the current IP to the return stack.
    *   `void sloth__execute(X* x, CELL q)`: Executes a given XT `q`, handling both primitives (negative XT) and compiled words (positive XT).
    *   `void sloth__inner(X* x)`: The core execution loop for compiled Forth code, interpreting XTs until the return stack is exhausted.
    *   `void sloth_eval(X* x, CELL q)`: Evaluates a given XT `q`, running the inner interpreter if it's a compiled word.

*   **Exceptions:**
    *   `void sloth_catch(X* x, CELL q)`: Establishes an exception frame, executing quotation `q` and catching any `THROW` calls.
    *   `void sloth_throw(X* x, CELL e)`: Throws an exception with error code `e`, unwinding the stack to the nearest `CATCH` or exiting.

*   **Platform-dependent I/O:**
    *   `int getch()`: Reads a single character from standard input without waiting for Enter (platform-dependent implementation).

### 2. Primitive Forth Words (82)

These are the C functions registered as Forth words during the `sloth_bootstrap_kernel` process, forming the initial vocabulary available to the Forth interpreter.

*   **Control Flow:**
    *   `EXIT` (`sloth_exit_`): Terminates execution of the current Forth word, popping the instruction pointer from the return stack.
    *   `(LIT)` (`sloth_lit_`): Pushes the next `CELL` from the instruction stream onto the data stack.
    *   `(RIP)` (`sloth_rip_`): Pushes a calculated address (relative to current IP) onto the data stack. Used for relative addressing.
    *   `(BRANCH)` (`sloth_branch_`): Unconditional jump; advances IP by an offset.
    *   `(?BRANCH)` (`sloth_zbranch_`): Conditional jump; advances IP by an offset if the top of stack is zero, otherwise skips the offset.
    *   `(STRING)` (`sloth_string_`): Processes a string literal, pushing its address and length.
    *   `(CSTRING)` (`sloth_c_string_`): Processes a C-style string literal, pushing its address.
    *   `(DOLOOP)` (`sloth_doloop_`): Implements the core logic for Forth's `DO ... LOOP` and `DO ... +LOOP` structures.

*   **Compilation & Definition:**
    *   `:` (`sloth_colon_`): Enters compilation mode, creates a new word header, and makes it hidden.
    *   `:NONAME` (`sloth_colon_no_name_`): Similar to `:`, but creates an anonymous word.
    *   `;` (`sloth_semicolon_`): Exits compilation mode, compiles `EXIT`, and unhides the last defined word. (Immediate)
    *   `RECURSE` (`sloth_recurse_`): Compiles a call to the word currently being defined. (Immediate)
    *   `ALLOT` (`sloth_allot_`): Reserves a specified number of bytes in the dictionary.
    *   `CELLS` (`sloth_cells_`): Converts a count of cells to a count of bytes (multiplies by `sCELL`).
    *   `CHARS` (`sloth_chars_`): (No operation) Designed for compatibility; `CHAR` size is `1`.
    *   `COMPILE,` (`sloth_compile_comma_`): Compiles the XT on top of the stack into the dictionary.
    *   `CREATE-NAME` (`sloth_create_name_`): Creates a header for a word given its name and length on the stack.
    *   `CREATE` (`sloth_create_`): Parses a name, creates a new word header, and compiles a standard code sequence to push its parameter field address.
    *   `DOES>` (`sloth_does_`): Modifies the most recently `CREATE`d word so that when it's executed, it first pushes its parameter field address, then executes the code following `DOES>`. (Immediate)

*   **Stack Manipulation:**
    *   `DROP` (`sloth_drop_`): Removes the top item from the data stack.
    *   `DUP` (`sloth_dup_`): Duplicates the top item from the data stack.
    *   `OVER` (`sloth_over_`): Copies the second item on the data stack to the top.
    *   `>R` (`sloth_to_r_`): Moves the top item from the data stack to the return stack.
    *   `R>` (`sloth_r_from_`): Moves the top item from the return stack to the data stack.
    *   `SWAP` (`sloth_swap_`): Exchanges the top two items on the data stack.
    *   `DEPTH` (`sloth_depth_`): Pushes the number of items on the data stack.
    *   `RDEPTH` (`sloth_r_depth_`): Pushes the number of items on the return stack.
    *   `(EMPTY-RETURN-STACK)` (`sloth_empty_rs_`): Empties the return stack.

*   **Memory Access (Forth Words):**
    *   `B@` (`sloth_b_fetch_`) / `B!` (`sloth_b_store_`): Fetch/store as a 8-bit value from at an address.
    *   `C@` (`sloth_c_fetch_`) / `C!` (`sloth_c_store_`): Fetch/store as an unsigned character from/at an address.
    *   `W@` (`sloth_w_fetch_`) / `W!` (`sloth_w_store_`): Fetch/store as a 16-bit value from/at an address.
    *   `L@` (`sloth_l_fetch_`) / `L!` (`sloth_l_store_`): Fetch/store as a 32-bit value from/at an address.
    *   `X@` (`sloth_x_fetch_`) / `X!` (`sloth_x_store_`): Fetch/store as a 64-bit value from/at an address.
    *   `@` (`sloth_fetch_`) / `!` (`sloth_store_`): Generic fetch/store a `CELL` at an address (size dependent on `sCELL`).
    *   `INT@` / `INT!` : Fetch/store an `int` at an address (size dependent on `sizeof(int)`).

*   **Arithmetic & Logic:**
    *   `AND` (`sloth_and_`): Bitwise AND.
    *   `INVERT` (`sloth_invert_`): Bitwise NOT.
    *   `LSHIFT` (`sloth_l_shift_`): Logical left shift.
    *   `M*` (`sloth_m_star_`): Signed 64-bit by 64-bit multiply to produce a 128-bit result (pushed as two cells).
    *   `-` (`sloth_minus_`): Subtraction.
    *   `+` (`sloth_plus_`): Addition.
    *   `D+` (`sloth_d_plus_`): Double-cell addition (adds two 2-cell numbers).
    *   `RSHIFT` (`sloth_r_shift_`): Logical right shift.
    *   `*` (`sloth_star_`): Multiplication.
    *   `2/` (`sloth_two_slash_`): Division by 2.
    *   `UM*` (`sloth_u_m_star_`): Unsigned 64-bit by 64-bit multiply to produce a 128-bit result.
    *   `UM/MOD` (`sloth_u_m_slash_mod_`): Unsigned 128-bit by 64-bit divide, producing remainder and quotient.

*   **Comparison:**
    *   `=` (`sloth_equals_`): Checks if two numbers are equal.
    *   `<` (`sloth_less_than_`): Checks if the second number is less than the first.


*   **Definite loops**
   *   `UNLOOP` (`sloth_unloop_`): Adjusts loop parameters on the return stack, used to prematurely exit loops.

*   **String & I/O:**
    *   `MOVE` (`sloth_move_`): Copies a block of memory.
    *   `EMIT` (`sloth_emit_`): Outputs a character.
    *   `KEY` (`sloth_key_`): Reads a single character.
    *   `WORD` (`sloth_word_`): Parses a word from the input stream using a delimiter.
    *   `SOURCE` (`sloth_source_`): Pushes the address and length of the current input source.

*   **Input Source Management:**
    *   `REFILL` (`sloth_refill_`): Attempts to fill the input buffer from the current input source.
    *   `SAVE-INPUT` (`sloth_save_input_`): Saves the current input source state onto the data stack.
    *   `RESTORE-INPUT` (`sloth_restore_input_`): Restores the input source state from the data stack.
    *   `INCLUDED` (`sloth_included_`): Processes an included file, managing input sources.

*   **Error Handling:**
    *   `CATCH` (`sloth_catch_`): Catches exceptions thrown by `THROW`.
    *   `THROW` (`sloth_throw_`): Throws an exception.

*   **Debugging & Environment:**
    *   `UNUSED` (`sloth_unused_`): Pushes the amount of unused dictionary space.
    *   `DEBUG` (`sloth_debug_`): Non-ANS word for debugging execution.
    *   `HERE` (`sloth_here_`): Pushes the address of the next available dictionary space.
    *   `IMMEDIATE` (`sloth_immediate_`): Marks the most recently defined word as immediate.
    *   `POSTPONE` (`sloth_postpone_`): Compiles the compilation semantics of the next word. (Immediate)
    *   `EVALUATE` (`sloth_evaluate_`): Interprets a string from memory as if it were input.
    *   `EXECUTE` (`sloth_execute_`): Executes an XT on the data stack.
    *   `FIND` (`sloth_find_`): Searches for a word in the dictionary given its counted string, returning its XT and flags.
    *   `(ENVIRONMENT)` (`sloth_environment_`): Provides information about the execution environment.
    *   `TO-ABS` (`sloth_to_abs_`): Converts a relative address to an absolute one.
    *   `TO-REL` (`sloth_to_rel_`): Converts an absolute address to a relative one.
    *   `INTS` (`sloth_ints_`): Multiplies a number by `sizeof(int)`.
    *   `(SELF)` (`sloth_self_`): Pushes the address of the VM context structure.

### 3. Essential Helper Functions (Internal to C)

These functions are critical for the C-level implementation of the Forth VM, particularly for bootstrapping, dictionary management, and defining new words. They are not directly exposed as Forth words to the user.

*   **Memory Management:**
    *   `CELL sloth_aligned(CELL a)`: Returns `a` aligned to `sCELL` boundaries.
    *   `void sloth_align(X* x)`: Aligns the dictionary pointer (`HERE`) to the next `CELL` boundary.
    *   `CELL sloth_here(X* x)`: (Internal getter for `HERE` value from user area).
    *   `void sloth_allot(X* x, CELL v)`: (Internal setter for `HERE`, allocating `v` bytes).

*   **Variable Access:**
    *   `void sloth_set(X* x, CELL a, CELL v)` / `CELL sloth_get(X* x, CELL a)`: Set/get a `CELL` in the dictionary at relative address `a`.
    *   `void sloth_cset(X* x, CELL a, uCHAR v)` / `uCHAR sloth_cget(X* x, CELL a)`: Set/get an `uCHAR` in the dictionary at relative address `a`.
    *   `void sloth_user_area_set(X* x, CELL a, CELL v)` / `CELL sloth_user_area_get(X* x, CELL a)`: Set/get a `CELL` in the user area at offset `a`.

*   **Compilation Primitives:**
    *   `void sloth_comma(X* x, CELL v)`: Compiles a `CELL` value into the dictionary at `HERE` and advances `HERE`.
    *   `void sloth_ccomma(X* x, uCHAR v)`: Compiles an `uCHAR` value into the dictionary at `HERE` and advances `HERE`.
    *   `void sloth_compile(X* x, CELL xt)`: Compiles an XT into the dictionary.
    *   `void sloth_literal(X* x, CELL n)`: Compiles `(LIT)` followed by `n` into the dictionary.

*   **Word Header & Dictionary Management:**
    *   `CELL sloth_get_latest(X* x)` / `void sloth_set_latest(X* x, CELL w)`: Get/set the address of the most recently defined word.
    *   `CELL sloth_header(X* x, CELL n, CELL l)`: Creates a new word header in the dictionary for a word named `n` of length `l`.
    *   `CELL sloth_get_link(X* x, CELL w)`: Returns the link field of word `w`.
    *   `CELL sloth_get_xt(X* x, CELL w)` / `void sloth_set_xt(X* x, CELL w, CELL xt)`: Get/set the execution token (XT) of word `w`.
    *   `uCHAR sloth_get_flags(X* x, CELL w)` / `void sloth_set_flag(X* x, CELL w, uCHAR v)` / `void sloth_unset_flag(X* x, CELL w, uCHAR v)` / `CELL sloth_has_flag(X* x, CELL w, CELL v)`: Manage word flags (e.g., `IMMEDIATE`, `HIDDEN`).
    *   `uCHAR sloth_get_namelen(X* x, CELL w)` / `CELL sloth_get_name_addr(X* x, CELL w)`: Get the name length and address of word `w`.

*   **Word Searching:**
    *   `int sloth__compare_without_case(X* x, CELL a1, uCELL u1, CELL a2, uCELL u2)`: Compares two strings case-insensitively.
    *   `CELL sloth__search_word(X* x, CELL n, int l)`: Searches for a word in the defined word lists.
    *   `CELL sloth_find_word(X* x, char* name)`: A convenience function to find a word given a C-style string name.

*   **Bootstrapping Specifics:**
    *   `CELL sloth_primitive(X* x, F f)`: Registers a C function `f` as a primitive and returns its negative XT.
    *   `CELL sloth_code(X* x, char* name, CELL xt)`: Creates a word header for `name` and assigns `xt` as its execution token.
    *   `void sloth_user_variable(X* x, char*name, CELL d, CELL v)`: Defines a user variable with initial value `v` at offset `d` in the user area.
    *   `void sloth_bootstrap_kernel(X* x)`: The main function responsible for initializing the dictionary and user area, and registering all C primitives.
    *   `void sloth_bootstrap(X* x)`: A wrapper function that calls `sloth_bootstrap_kernel`.

*   **C-level Utilities:**
    *   `void sloth_set_root_path(X* x, char* s)`: Sets the root path for file operations.
    *   `int sloth_include(X* x, char* f)`: Includes and interprets a Forth source file from C.
    *   `void sloth_evaluate(X* x, char* s)`: Evaluates a C string as Forth code.
    *   `void sloth_repl(X* x)`: Starts a basic Read-Eval-Print Loop (REPL).

### 4. Possible Reimplementations of Memory Access Primitives (freeing 10 primitives, leaving just B@ and B!)

This section explores how several C-defined memory access primitives (`W@`, `W!`, `L@`, `L!`, `X@`, `X!`, `@`, `!`) could be reimplemented in Forth using more fundamental byte-level access primitives (`B@`, `B!`). This strategy aims to reduce the C codebase size, trading off some performance for greater minimality in the C core.

**Core Primitives Required to Remain in C:**

To support these Forth implementations, the following primitives must remain in C:

*   **Byte-level memory access:** `B@` (addr -- byte), `B!` (byte addr --)
*   **Arithmetic:** `+`, `8` (as a literal or primitive), `LSHIFT`, `RSHIFT`.
*   **Stack manipulation:** `DUP`, `SWAP`, `OVER`, `>R`, `R>`.

**Assumptions:**

1.  **`sCELL` Constant:** The size of a native `CELL` (`sCELL`) must be exposed to Forth as a constant during bootstrap. This is a value, not a new primitive *function*, and is crucial for proper `CELL`-sized operations.
2.  **Little-Endian:** The implementations below assume a Little-Endian memory layout (least significant byte at the lowest address). Big-Endian systems would require reversing the byte order.
3.  **Signed/Unsigned:** `B@` fetches an unsigned byte (0-255). Composition logic handles this for larger numbers.

**Proposed Forth Implementations (Little-Endian):**

First, the `sCELL` constant would be defined in a core Forth bootstrap file (e.g., `4th/ans.4th`), with its value selected based on the C build environment.

(NOTE FROM ME: There is no need to create the sCELL based on the C build environment, just do : sCELL 1 CELLS ; if its required, or 1 CELLS CONSTANT sCELL)

```forth
\ In bootstrap code (e.g., ans.4th)
\ This value would be set by the C code during initialization,
\ or the build system would select the correct file based on sCELL.
4 CONSTANT sCELL  \ Example for a 32-bit system
\ 8 CONSTANT sCELL  \ Example for a 64-bit system
```

---

**1. 16-bit Words (`W@` / `W!`)**

These words fetch and store 16-bit values.

```forth
: W@ ( addr -- w )  \ Fetches a 16-bit word.
  DUP B@          \ ( addr byte0 )  \ Fetch byte 0
  SWAP 1+ B@      \ ( byte0 addr+1 byte1 ) \ Fetch byte 1
  8 LSHIFT +      \ ( word )        \ Combine bytes (byte1 << 8) | byte0
;

: W! ( w addr -- )  \ Stores a 16-bit word.
  >R              \ ( w ) R:( addr ) \ Save address on return stack
  DUP R@ B!       \ ( w ) Store low byte (byte0)
  8 RSHIFT        \ ( w' )  \ Shift to get high byte (byte1)
  R> 1+ B!        \ ( )     \ Store high byte at addr+1
;
```

---

**2. 32-bit Words (`L@` / `L!`)**

These words fetch and store 32-bit values.

```forth
: L@ ( addr -- l )  \ Fetches a 32-bit long word.
  DUP B@          \ ( addr byte0 )
  OVER 1+ B@      \ ( addr byte0 byte1 )
  OVER 2+ B@      \ ( addr byte0 byte1 byte2 )
  SWAP 3+ B@      \ ( byte0 byte1 byte2 addr byte3 )
  ROT ROT ROT     \ ( byte3 byte2 byte1 byte0 )
  8 LSHIFT +      \ ( byte3 byte2 byte1_0 )
  8 LSHIFT +      \ ( byte3 byte2_1_0 )
  8 LSHIFT +      \ ( long_word )
;

: L! ( l addr -- )  \ Stores a 32-bit long word.
  >R                \ ( l ) R:( addr )
  DUP R@ B!         \ ( l ) Store byte 0
  8 RSHIFT          \ ( l' )
  DUP R@ 1+ B!      \ ( l' ) Store byte 1
  8 RSHIFT          \ ( l'' )
  DUP R@ 2+ B!      \ ( l'' ) Store byte 2
  8 RSHIFT          \ ( l''' )
  R> 3+ B!          \ ( ) Store byte 3
;
```

---

**3. Cell-sized Words (`@` / `!`)**

These words handle memory access for a native `CELL`. Their definitions depend on the size of `sCELL`. The build system would typically include the correct definition based on `sCELL`.

**For a 32-bit system (where `sCELL` is 4 bytes):**

```forth
\ In a file for 32-bit Sloth (or conditionally compiled)
4 CONSTANT sCELL

: @ ( addr -- cell ) L@ ; \ @ is an alias for L@
: ! ( cell addr -- ) L! ; \ ! is an alias for L!
```

(NOTE FROM ME: This should be something like: 1 CELLS 4 = [IF] : @ L@ ; [ELSE] : @ X@ ; [THEN])

**For a 64-bit system (where `sCELL` is 8 bytes):**

First, the 64-bit fetch/store (`X@` / `X!`) primitives would be defined:

```forth
: X@ ( addr -- x )  \ Fetches a 64-bit extended word.
  DUP B@
  OVER 1+ B@
  OVER 2+ B@
  OVER 3+ B@
  OVER 4+ B@
  OVER 5+ B@
  OVER 6+ B@
  SWAP 7+ B@      \ ( b0 b1 b2 b3 b4 b5 b6 b7 )

  \ Now combine them (byte7 << 56) | ... | byte0
  8 LSHIFT +
  8 LSHIFT +
  8 LSHIFT +
  8 LSHIFT +
  8 LSHIFT +
  8 LSHIFT +
  8 LSHIFT +
;

: X! ( x addr -- )  \ Stores a 64-bit extended word.
  >R
  DUP R@ B!           \ Store byte 0
  8 RSHIFT DUP R@ 1+ B! \ Store byte 1
  8 RSHIFT DUP R@ 2+ B! \ Store byte 2
  8 RSHIFT DUP R@ 3+ B! \ Store byte 3
  8 RSHIFT DUP R@ 4+ B! \ Store byte 4
  8 RSHIFT DUP R@ 5+ B! \ Store byte 5
  8 RSHIFT DUP R@ 6+ B! \ Store byte 6
  8 RSHIFT R> 7+ B!     \ Store byte 7
;

\ In a file for 64-bit Sloth (or conditionally compiled)
8 CONSTANT sCELL

: @ ( addr -- cell ) X@ ;
: ! ( cell addr -- ) X! ;
```
