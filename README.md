# SLOTH

A small, simple, relatively fast, double stack based virtual machine with human readable/writeable bytecode meant for extending applications.

In the future there will be other implementations (at least Java/Kotlin for Android development).

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor and Dawn.

Features:

* C89. Very easily embedabble in a C/C++ application.
* Human readable bytecode (ASCII 32-126), no need to use an assembler.
* Cell (64, 32 or 16 bits) based data stack, return stack and retain stack
* Quotations at bytecode level.
* Relatively fast interpreter.
* Ability to add C function thru bytecode extensions (bytecodes A-Z).

## VM Architecture

* Data Stack (S)
* Return Stack (R)
* Retain Stack (T) -don' think i need this-
* Name Stack (N)
* Instruction Pointer Register (IP)
* Error Register (ERR)
* Memory Block Register (B)

## Bytecodes

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
    a -> 
    b -> address of b register
    c -> size of cell (8 on 64 bits, 4 on 32 bits, 2 on 16 bits)
    d -> dup
    e -> emit
    f -> free from heap (free)
    g -> compile quotation without ending return
    h -> header
    i -> inspect memory
    j -> jump/call
    k -> key
    l -> 
    m -> allocate on heap (malloc)
    n -> string to number
    o -> over
    p -> toggle printing traces
    q -> compile quotation
    r -> rot
    s -> swap
    t -> throw (set E)
    u -> top of T
    v -> second of T
    w -> third of T
    x -> fourth of T
    y -> fifth of T
    z -> jump/call if zero
    { -> to R
    | -> or
    } -> return (alternative to ] for exiting returning from a quotation)
    ~ -> invert
