# SLOTH (another SLOw forTH)

An ANS Forth implemented on a small, simple, relatively fast (or relatively slow), double stack based virtual machine with human readable/writeable bytecode meant for extending applications.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

## IDEAS

* Hyperstatic environment, inline as much as possible
* 

## VIRTUAL MACHINE

[In the future there will be other implementations (at least Java/Kotlin for Android development)]

Features:

* C89. Very easily embedabble in a C/C++ application.
* String based human readable bytecode (ASCII 32-126).
* Data stack can hold pointers/objects (depending on implementation).
* Relatively fast interpreter.
* Ability to add C functions thru bytecode extensions (bytecodes A-Z).

### EXTENSIBILITY

There are two interpreters (as in Forth), the inner and the outer interpreter.
The outer interpreter is the one that allows Forth/Sloth to be extensible, it parses the input string word by word and if its found in the dictionary its evaluated/compiled.
Only : ; \<asm> are hardcoded in the outer interpreter, but they happen after trying to find a word. If the word its found, its executed. This allows full extensibility of the system just by redefining any possible word.

### LITERALS ENCODING

Literals are stored in memory with a variable length encoding.

### ARCHITECTURE

* Inner interpreter
* Outer interpreter
  
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

    ]/}/0 -> return
    $ -> call
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

    c -> size of cell in bytes

    [ { -> inspect memory ]

##### Dictionary

    h -> header (create label)
    i -> set latest word as immediate
    b -> compile byte

##### Input/Output

    e -> emit
    k -> key
    [ l -> load and eval file ]

### ASCII ordered

    (SPACE) -> noop
    ! -> not
    " -> 
    # -> cell literal
    $ -> call
    % -> modulo
    & -> and
    ' -> 
    ( -> push to R
    ) -> pop from R
    * -> multiplication
    + -> addition
    , -> store cell
    - -> substraction
    . -> fetch cell
    / -> division
    0 -> literal 0
    1 -> literal 1
    2 ->
    3 ->
    4 ->
    5 ->
    6 ->
    7 ->
    8 ->
    9 ->
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
    [ -> 
    \ -> find symbol from string
    ] -> 
    ^ -> xor
    _ -> drop
    ` -> 
    a -> accept
    b -> compile byte to dictionary
		c -> size of cell
    d -> dup
    e -> emit
    f -> free
    g -> 
    h -> 
    i -> immediate
    j -> jump
    k -> key
    l -> 
		m -> malloc
    n -> 
    o -> over
    p -> 
    q -> quit
    r -> rot
    s -> swap
    t -> times (combinator)
    u -> 
    v -> 
    w -> while (combinator)
    x -> 
    y -> 
    z -> zjump (jump if zero)
    { -> start quotation/push IP+1 
    | -> or
    } -> return/end of quotation 
    ~ -> invert
