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
* Instruction Pointer Register (IP)
* Error Register (ERR)
* Memory Block Register (B)

## Bytecodes

    (SPACE) -> no ->noop
    ! -> not
    " -> string literal
    # -> cell literal
    $ -> find and call symbol
    % -> modulo
    & -> and
    ' -> byte literal
    ( -> to R
    ) -> from R
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
    A-Z -> C extensions
    [ -> start quotation
    \ -> define/find symbol (and initialize b if not done before)
    ] -> return/end of quotation
    ^ -> xor
    _ -> drop
    ` -> return
    a -> apply (call)
		bh -> block here address
		bl -> block latest address
		bs -> block size address
		cb -> compile byte
		cc -> compile cell
		cg -> compile quotation without ending return
		cq -> compile quotation
    d -> dup
    e -> emit
    f -> find
    g -> (MOVE TO C)compile quotation without ending return
    h -> create header
    i -> if (jump if zero)
    j -> jump
    k -> key
    l -> load and evaluate file
		ma -> allot
		mf -> free
		mi -> inspect memory
		mm -> malloc
    n -> (MOVE TO EXTENSION)string to number
    o -> over
    p -> 
    q -> 
    r -> rot
    s -> swap
    t -> times (combinator)
    u -> 
    v ->
    w -> while (combinator)
    xa -> context address
		xb -> address of block variable
		xc -> size of cell
		xq -> quit/halt
		xr -> RP@
		xs -> SP@
		xt -> trace/untrace
    y -> 
    z -> zcall (call if zero)
    { -> 
    | -> or
    } -> return (alternative to ] for exiting returning from a quotation)
    ~ -> invert
