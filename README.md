# SLOTH

A small, simple, relatively fast, double stack based virtual machine with human readable/writeable bytecode meant for extending applications.

In the future there will be other implementations (at least Java/Kotlin for Android development).

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

Features:

* One C89 header only. Very easily embedabble in a C/C++ application.
* Human readable bytecode (ASCII 32-126)
* Cell based data and return stack
* Quotations at bytecode level.
* Relatively fast interpreter.
* Ability to add C function extensions (bytecodes A-Z).
* No dictionary/words/symbols needed at bytecode level.
* Input/output thru 2 user defined key/emit functions
 
## Bytecode

I don't think I like strings here....

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

## Extensions

I don't really like this for base.

### Combinators

    ? -> branch
    1b -> 1bi
    2b -> 2bi
    3b -> 3bi
    1t -> 1tri
    2t -> 2tri
    3t -> 3tri
    b -> binrec
    d -> dip
    i -> ifte
    l -> linrec
    s -> sip
    t -> times
    u -> until
    w -> while

### Dictionary

    a -> allot
    h -> here
    c -> create
    f -> find
    i -> init
    ; -> compile byte
    , -> compile cell
    q -> compile quotation
    Q -> compile quotation and return
    s -> compile string

### Strings

    a -> accept
    c -> compare
    t -> type