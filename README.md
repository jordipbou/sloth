# SLOTH (another SLOw forTH)

An ANS Forth implemented on a small, simple, relatively fast (or relatively slow), double stack based virtual machine with human readable/writeable bytecode meant for extending applications.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

## VIRTUAL MACHINE

[In the future there will be other implementations (at least Java/Kotlin for Android development)]

Features:

* C89. Very easily embedabble in a C/C++ application.
* String based human readable bytecode (ASCII 32-126).
* Relatively fast interpreter meant to be extended.
* Ability to add C functions thru bytecode extensions (bytecodes A-Z).

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

* Inner interpreter
* Outer interpreter
  
### BYTECODE

### ASCII ordered

	  (< 32) -> noop
    (SPACE) -> noop
    ! -> store cell
    " -> short fetch
    # -> fetch byte
    $ -> 
    % -> modulo
    & -> and
    ' -> short store
    ( -> push to R
    ) -> pop from R
    * -> multiplication
    + -> addition
    , -> cell store
    - -> substraction
    . -> cell fetch
    / -> division
    0 -> literal 0
    1 -> literal 1
    2 -> short literal (16 bit)
    3 ->
    4 -> int literal (32 bit)
    5 ->
    6 ->
    7 ->
    8 -> long literal (64 bit)
    9 ->
    : -> byte fetch
    ; -> byte store
    < -> less than
    = -> equal
    > -> greater than
    ? -> RESERVED for branch (combinator)
    @ -> 
		A -> C extension
		B -> C extension
		C -> C extension
		D -> C extension
		E -> C extension
		F -> C extension
		G -> C extension
		H -> C extension
		I -> C extension
		J -> C extension
		K -> C extension
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
    ` -> 
    a -> 
    b -> push block variable address
		c -> sizeof cell
    d -> dup
    e -> RESERVED FOR EMIT
    f -> 
    g -> 
    h -> 
    i -> 
    j -> jump
    k -> RESERVED FOR KEY
    l -> 
		m -> 
    n -> nip
    o -> over
    p -> 
    q -> 
    r -> rot
    s -> swap
    t -> RESERVED for times combinator
    u -> 
    v -> 
    w -> 
    x -> execute
    y -> 
    z -> jump if zero
    { -> start quotation (non nestable)
    | -> or
    } -> end quotation (return)
    ~ -> invert
