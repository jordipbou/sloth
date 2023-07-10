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

    ' -> byte literal
    "" -> string literal
    [] -> quotation literal
    \ -> symbol literal
    0-9 -> number literal
    # -> cell literal
    
    + -> addition
    - -> substraction
    * -> multiplication
    / -> division
    % -> modulo
    
    ba -> alloc
    bb -> block base
    bh -> here
    b; -> compile byte
    b, -> compile cell
    bq -> compile quotation
    bs -> compile string
    bl -> block latest
    bf -> find
    bc -> create
    
    < -> less than
    = -> equal
    > -> greater than
    
    $ -> call, jump $] or $} or $<0>
    ] -> return, also } and <0>
    ? -> call if zero, jump ?] or ?} or ?<0>
    ( -> to r
    ) -> from r
    p -> peek r
    
    & -> and
    | -> or
    ! -> not
    ~ -> invert

    k -> key
    e -> emit
    
    m -> malloc
    f -> free
    c -> cell size
    ; -> store byte
    : -> fetch byte
    , -> store byte
    . -> fetch byte
    i -> inspect

    ha -> accept
    ht -> type
    h. -> display number
    hc -> string comparison
    hd -> dip
    hb -> binary recursion
    hl -> linear recursion
    hs -> sip
    hn -> times
    hw -> while
    h? -> ifthen
    
    s -> swap
    d -> dup
    o -> over
    r -> rot
    _ -> drop
