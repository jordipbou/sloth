\ Small differential program for the JavaScript runner check: the C and
\ JavaScript engines must print exactly the same bytes for this file.
\ Avoids floating point and addresses so the comparison is stable.

: SQUARE DUP * ;
: SUM 0 SWAP 0 DO I + LOOP ;
: FACT DUP 1 > IF DUP 1- RECURSE * ELSE DROP 1 THEN ;
: FIB 0 1 ROT 0 DO SWAP OVER + LOOP DROP ;
: BOX 10 MIN 0 MAX ;

CR
1 2 + . 6 7 * . -5 . 10 3 /MOD . .
9 SQUARE . 10 SUM . 5 FACT . 10 FIB .
S" hello" TYPE CR
VARIABLE X 42 X ! X @ .
CREATE ARR 3 CELLS ALLOT 7 ARR ! 9 ARR CELL+ ! 11 ARR 2 CELLS + !
ARR @ ARR CELL+ @ ARR 2 CELLS + @ . . .
-3 BOX . 4 BOX . 99 BOX .
: ADD3 3 + ; 4 ADD3 .
CR
