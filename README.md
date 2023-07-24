# SLOTH

A small, simple, relatively fast, double stack based virtual machine f with human readable/writeable bytecode meant for extending application.

In the future there will be other implementations (at least Java/Kotlin for Android development).

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

Features:

* One C89 header only. Very easily embedabble in a C/C++ application.
* Human readable bytecode (ASCII 32-126), no need to use an assembler.
* Cell based data and return stack
* Quotations at bytecode level.
* Relatively fast interpreter.
* Ability to add C function extensions (bytecodes A-Z).
 
## Bytecode (not updated)

I don't think I like strings here....but if I have them, there must be also accept/type.

    ' -> byte literal
    # -> cell literal
    @ -> relative to IP byte literal

    s -> swap
    d -> dup
    o -> over
    r -> rot
    _ -> drop
    
    + -> addition
    - -> substraction
    * -> multiplication
    / -> division
    % -> modulo
    
    < -> less than
    = -> equal
    > -> greater than
    
    x -> call, jump x] or x} or x<0>
    ] -> return, also } and <0>
    z -> call if zero, jump z] or z} or z<0>
    ( -> to r
    ) -> from r
    u -> peek top of r
    v -> peek next of r
    
    & -> and
    | -> or
    ! -> not
    ~ -> invert

    k -> key
    e -> emit

    b -> pushes b register address
    
    m -> malloc
    f -> free
    c -> cell size
    ; -> store byte
    : -> fetch byte
    , -> store byte
    . -> fetch byte

    --- Helpers

    "" -> string literal
    [] -> quotation literal
    0-9 -> number literal

    u -> accept
    g -> type

    i -> inspect
    
    ? -> branch
    w -> while
    t -> times

    \ -> create/find symbol (pushes CFA)
    $ -> call to symbol CFA

    q -> compile quotation without return
    j -> compile quotation with return

## ASCII ordered bytecodes (updated)

    (SPACE) -> noop
    ! -> not
    " -> string literal
    # -> cell literal
    $ -> find and call symbol
    % -> modulo
    & -> and
    ' -> byte literal
    ( -> to t
    ) -> from t
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
    @ -> (TODO) relative to IP byte literal
    A-Z -> C extensions
    [ -> start quotation
    \ -> define/find symbol (and initialize b if not done before)
    ] -> return/end of quotation
    ^ -> xor
    _ -> drop
    ` -> find
    a -> apply (call/jump)
    b -> address of b register
    c -> size of cell (8 on 64 bits, 4 on 32 bits, 2 on 16 bits)
    d -> drop
    e -> emit
    f -> free from heap (free)
    g -> compile quotation without ending return
    h -> header
    i -> inspect memory
    j -> 
    k -> key
    l ->
    m -> allocate on heap (malloc)
    n -> string to number
    o -> over
    p -> 
    q -> compile quotation
    r -> rot
    s -> swap
    t -> 
    u -> top of T
    v -> second of T
    w -> third of T
    x -> fourth of T
    y -> fifth of T
    z -> call/jump if zero
    { ->
    | -> or
    } -> return (alternative to ] for exiting returning from a quotation)
    ~ -> invert
