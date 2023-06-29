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
    ' BYTE LITERAL
    " CELL LITERAL
    0-9 NUMBER LITERAL
    [] QUOTATION LITERAL
    // Arithmetic
    + ADD
    - SUB
    * MUL
    / DIVMOD
    // Comparison
    < LT
    = EQ
    > GT
    // Logical
    & AND
    | OR
    ! NOT
    // Stack
    $ SWAP
    { DUP
    ^ OVER
    @ ROT
    _ DROP
    // Execution
    ] RETURN (POP FROM R TO IP)
    % CALL (PUSH FROM IP TO R AND SET IP TO S)
    %] JUMP (SET IP TO S)
    ? IF (ZCALL)
    ?] IF (ZJUMP)
    ( SAVE (S to R)
    ) RESTORE (R to S)
    # TIMES (LOOP)
    // Memory
    : BFETCH
    ; BSTORE
    . CFETCH
    , CSTORE
    ~ CELL
    // a-z variables/registers
    // A-Z C extensions