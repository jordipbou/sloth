# SLOTH (another SLOw forTH)

An ANS Forth implemented on a small, simple, relatively fast (or relatively slow), double stack based virtual machine with human readable/writeable bytecode meant for extending applications.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

## VIRTUAL MACHINE

[In the future there will be other implementations (at least Java/Kotlin for Android development)]

Features:

* C89. Very easily embedabble in a C/C++ application.
* Human readable bytecode (ASCII 32-126).
* Cell (64, 32 or 16 bits) based data stack, return stack and retain stack
* Quotations and combinators at bytecode level.
* Relatively fast interpreter.
* Ability to add C functions thru bytecode extensions (bytecodes A-Z).

### BYTECODE

#### Ordered by use

##### Stack operations

    _ -> drop
    s -> swap
    d -> dup
    o -> over
    r -> rot
    ( -> push
    ) -> pop

##### Arithmetic operations

    + -> addition
    - -> substraction
    * -> multiplication
    / -> division
    % -> modulo

##### Bitwise operations

    ! -> not
    ~ -> invert
    & -> and
    | -> or
    ^ -> xor

##### Comparison operations

    < -> less than
    = -> equal
    > -> greater than
    
##### Execution operations

    ]/} -> return
    x -> call/tail call
    j -> jump
    z -> zjump
    y -> yield
    
##### Helpers

    ? -> branch (combinator)
    t -> times (combinator)
    w -> while (combinator)
    u -> unview traces
    v -> view traces
    [ n -> string to number ]

##### Memory operations

    m -> malloc
    f -> free

    . -> fetch cell
    : -> fetch byte
    , -> store cell
    ; -> store byte

    b -> address of b var (dict/block)
    c -> size of cell in bytes

    [ { -> inspect memory ]

##### Dictionary

    h -> header (create label)
    $ -> find symbol

##### Input/Output

    e -> emit
    k -> key
    [ l -> load and eval file ]

### ASCII ordered

    (SPACE) -> noop
    ! -> not
    " -> 
    # -> cell literal
    $ -> colon
    % -> modulo
    & -> and
    ' -> symbol (find/create)
    ( -> push to R
    ) -> pop from R
    * -> multiplication
    + -> addition
    , -> store cell
    - -> substraction
    . -> fetch cell
    / -> division
    0-9 -> parsed numeric literal
    : -> fetch byte
    ; -> store byte
    < -> less than
    = -> equal
    > -> greater than
    ? -> branch
    @ -> relative to IP byte literal
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
    [ -> start quotation/push IP+1
    \ -> find symbol from string
    ] -> return/end of quotation
    ^ -> xor
    _ -> drop
    ` -> 
    a -> accept
    b -> address of b variable
		c -> size of cell
    d -> dup
    e -> emit
    f -> free
    g -> 
    h -> 
    i -> interpret/call/execute
    j -> jump
    k -> key
    l -> 
		m -> malloc
    n -> 
    o -> over
    p -> print
    q -> quit
    r -> rot
    s -> swap
    t -> times (combinator)
    u -> untrace
    v -> view traces
    w -> while (combinator)
    x -> 
    y -> 
    z -> zjump (jump if zero)
    { -> semicolon
    | -> or
    } -> return
    ~ -> invert

## FORTH

