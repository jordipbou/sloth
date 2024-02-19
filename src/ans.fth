: FIRST_COLON ;

: DROP $_ ;
: DUP $d ;
: OVER $o ;
: SWAP $s ;
: ROT $r ;

: 2DROP drop drop ;

: >R $( ;
: R> $) ;
: R@ $f ;

: + $+ ;
: - $- ;
: * $* ;
: / $/ ;
: MOD $% ;

: < $< ;
: = $= ;
: > $> ;
: 0= $0 ;

: 0< 0 < ;

: AND $& ;
: OR $| ;
: XOR $^ ;
: INVERT $~ ;

: @ $@ ;
: ! $! ;
: W@ $: ;
: W! $; ;
: C@ $. ;
: C! $, ;

: +! dup @ rot + swap ! ;

: CELL $c ;
: CELL+ cell + ;
: CELLS cell * ;

: HERE $h ;
: ALLOT $a ;

: , here ! cell allot ;
: W, here w! 2 allot ;
: C, here c! 1 allot ;

: BYE $q ;

: EMIT $E ;
: KEY $K ;

: IMMEDIATE $Si ;
: PARSE $Sp ;

: ( 41 parse drop drop ; immediate

: CREATE $Sh ;
: VARIABLE create 0 , ;

VARIABLE BASE

: DECIMAL   10 base !  ;
: OCTAL      8 base !  ;
: HEX       16 base !  ;
: BINARY     2 base !  ;

decimal

: PAD here 176 + ;

\ ------------------------ OUTPUT ( pForth ) -------------------------
\ Number output based on F83
variable HLD    \ points to last character added

: hold   ( char -- , add character to text representation)
    -1 hld  +!
    hld @  c!
;
: <#     ( -- , setup conversion )
    pad hld !
;
: #>     ( d -- addr len , finish conversion )
    2drop  hld @  pad  over -
;
\ : sign   ( n -- , add '-' if negative )
\     0<  if  ascii - hold  then
\ ;
: #      ( d -- d , convert one digit )
   base @  mu/mod rot 9 over <
   IF  7 +
   THEN
   ascii 0 + hold
;

: #s     ( d -- d , convert remaining digits )
    BEGIN  #  2dup or 0=
    UNTIL
;

