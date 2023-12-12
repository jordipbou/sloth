# SLOTH (another SLOw forTH)

An ANS Forth implemented on a small, simple, relatively fast (or relatively slow), double stack based virtual machine with human readable/writeable bytecode meant for extending applications.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

## DODO VIRTUAL MACHINE

[In the future there will be other implementations (at least Java/Kotlin for Android development)]

Features:

* C89. Very easily embedabble in a C/C++ application.
* String based human readable bytecode (ASCII 32-126).
* Relatively fast interpreter meant to be extended.
* Ability to add C functions thru bytecode extensions (bytecodes A-Z).

### ARCHITECTURE

#### DATA TYPES

* B - 8 bit signed integer
* W - 16 bit signed integer
* L - 32 bit signed integer
* X - 64 bit signed integer
* C - 32/64 bit signed integer (platform dependent)

#### DICTIONARY

#### PROCESSOR

##### STACKS

* S - Data stack
* R - Return stack

##### REGISTERS

* SP - Data stack pointer
* RP - Return stack pointer
* IP - Instruction pointer
* ERR - Error code
* D - Dictionary base address 
* X - Address of extensions table
* M - Context private memory

### EXTENSIBILITY

There are two interpreters (as in Forth), the inner and the outer interpreter.
The outer interpreter is the one that allows Forth/Sloth to be extensible, it parses the input string word by word and if its found in the dictionary its evaluated/compiled.
Only : ; \<asm> &<asm> are hardcoded in the outer interpreter, but they happen after trying to find a word. If the word its found, its executed. This allows full extensibility of the system just by redefining any possible word.

### LITERALS ENCODING

There are 3 types of literals:

#### SMALL LITERAL

-1 0 1 represented as bytecodes 2 0 1

#### UNSIGNED 16 BIT LITERAL

Used for addresses of words.

Words and code are always aligned, so with 16 bit it's possible to use 65536 cells of memory. On 32 bits that's 256kb and on 64 bits that's 512kb.

#### CELL LITERAL



### ARCHITECTURE

#### Interpreter Context

Each interpreter context has its own dedicated memory. Its mapping is:

-1024 (64) Assembler buffer
 -960 (80) Input buffer
 -880 (84) Pad area
 -796 (33) CString area
 -763 (18) #> area

#### Dictionary
  
### BYTECODE

### ASCII ordered

	  (< 32) -> noop
    (SPACE) -> noop
    ! -> cell store
    " -> RESERVED for string literal if needed
    # -> RESERVED for number literal if needed
    $ -> compile next byte
    % -> modulo
    & -> and
    ' -> 8 bits literal
    ( -> to r
    ) -> from r
    * -> multiplication
    + -> addition
    , -> 8 bits store
    - -> substraction
    . -> 8 bits fetch
    / -> division
    0 -> equal to zero
    1 -> literal 1
    2 -> 16 bits literal
    3 -> 
    4 -> 32 bits literal
    5 -> 
    6 -> 
    7 -> 
    8 -> 64 bits literal
    9 -> 
    : -> 16 bits fetch
    ; -> 16 bits store
    < -> less than
    = -> equal
    > -> greater than
    ? -> RESERVED for if combinator
    @ -> cell fetch
		A -> C extension
		B -> C extension
		C -> C extension
		D -> C extension
		E -> emit extension
		F -> C extension
		G -> C extension
		H -> C extension
		I -> C extension
		J -> C extension
		K -> key extension
		L -> C extension
		M -> C extension
		N -> C extension
		O -> C extension
		P -> C extension
		Q -> C extension
		R -> C extension
		Q -> C extension
		S -> C extension
		T -> C extension
		U -> C extension
		V -> C extension
		W -> C extension
		X -> C extension
		Y -> C extension
		Z -> C extension
    [ -> quotation (push ip and jump)
    \ -> 
    ] -> return (quotation end)
    ^ -> xor
    _ -> drop
    ` -> dump memory
    a -> allot
    b -> 
		c -> RESERVED for sizeof cell
    d -> dup
    e -> RESERVED for error
    f -> fetch top of return stack
    g -> aligned
    h -> here
    i -> interpret/call/execute
    j -> jump
    k -> 
    l -> 
		m -> RESERVED for cmove> if needed
    n -> 
    o -> over
    p -> RESERVED for compare if needed
    q -> quit (exit app)
    r -> rot
    s -> swap
    t -> RESERVED for times combinator
    u -> RESERVED for unsigned operations
    v -> view/unview traces
    w -> RESERVED for while loop combinator
    x -> RESERVED for context operations (self or other)
    y -> RESERVED for type if needed
    z -> zjump
    { -> start quotation block literal
    | -> or
    } -> end quotation block (return)
    ~ -> invert
