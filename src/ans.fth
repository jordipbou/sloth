\Sc IBUF 255 \Sa
\Sc PAD 255 \Sa

\S: : $S: \S;
: ; $S; \S;Si

: IMMEDIATE $Si ;

: PARSE $Sp ;

: \ 10 parse ; 
: ( 41 parse ; IMMEDIATE

\ SLOTH primitives

: DROP $_ ;
: DUP $d ;
: OVER $o ;
: SWAP $s ;
: ROT $r ;
: NIP $n ;

: + $+ ;
: - $- ;
: * $* ;
: / $/ ;
: MOD $% ;

: < $< ;
: = $= ;
: > $> ;

: ! $, ;
: @ $. ;
: C! $; ;
: C@ $: ;

\ BLOCK variables

: BLOCK-HEADER $b @ ;

: REL>ABS block-header + ;
: ABS>REL block-header - ;

: BLOCK-SIZE block-header @ ;

: CELL $c ;
: CELLS cell * ;

: LATEST block-header cell + @ rel>abs ;

\ WORD

: PREVIOUS @ rel>abs dup block-header = ${_0}{}? ;
: CODE cell + @ rel>abs ;
: FLAGS 2 cells + c@ ;
: NAME>STRING 2 cells + 1 + 1 + dup 1 - c@ ; 

: TYPE ( c-addr u -- ) ${d:E1+}t_ ;
: ACCEPT ( c-addr n -- n ) ( TODO ) ;
