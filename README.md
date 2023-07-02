# SLOTH

A small, simple, relatively fast, double stack based virtual machine with human readable/writeable bytecode.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

Features:

* One header only. Very easily embedabble in a C/C++ application.
* Human readable bytecode.
* Quotations at bytecode level.
* Relatively fast interpreter.
* Ability to add C function extensions.

## Bytecode

    // Literals
    ' BYTE/CHAR LITERAL
    # CELL LITERAL
    0-9 NUMBER LITERAL
    [] QUOTATION LITERAL
    "" STRING LITERAL
    // Arithmetic
    + ADD
    - SUB
    * MUL
    / DIV
		% MOD
    // Comparison
    < LT
    = EQ
    > GT
    // Logical
    & AND
    | OR
    ! NOT
    // Stack
    s SWAP
    d DUP
    o OVER
    r ROT
    _ DROP
    // Execution
    ] RETURN (POP FROM R TO IP)
    $ CALL (PUSH FROM IP TO R AND SET IP TO S)
    $] JUMP (SET IP TO S)
    ? IF (ZCALL)
    ?] IF (ZJUMP)
    t TO R (POP S TO R)
    f FROM R (POP R TO S)
		p PEEK R (COPY R TO S)
    t TIMES (LOOP)
    b BINARY RECURSION
		q EXIT
    // Memory
    : BFETCH
    ; BSTORE
    . CFETCH
    , CSTORE
    c CELL SIZE IN BYTES
    // A-Z C extensions
