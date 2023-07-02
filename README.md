# SLOTH

A small, simple, relatively fast, double stack based virtual machine with human readable/writeable bytecode.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

Features:

* One C89 header only. Very easily embedabble in a C/C++ application.
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
    ) TO R (POP S TO R)
    ( FROM R (POP R TO S)
		~ PEEK R (COPY R TO S)
    n TIMES (LOOP)
		w WHILE (LOOP)
		q EXIT
    // Memory
    : BFETCH
    ; BSTORE
    . CFETCH
    , CSTORE
    c CELL SIZE IN BYTES
		// Input/Output
		k KEY
		e EMIT
		a ACCEPT
		t TYPE
    // A-Z C extensions
