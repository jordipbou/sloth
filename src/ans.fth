: DROP $_ ;
: DUP $d ;
: OVER $o ;
: SWAP $s ;
: ROT $r ;
: NIP $n ;

: >R $( ;
: R> $) ;
: R@ $u ;
: U $u ;
: V $v ;
: W $w ;
: X $x ;
: Y $y ;
: Z $z ;

: + $+ ;
: - $- ;
: * $* ;
: / $/ ;
: MOD $% ;
: LSHIFT ${ ;
: RSHIFT $} ;

: < $< ;
: = $= ;
: > $> ;
: 0= $0 ;

: AND $& ;
: OR $| ;
: XOR $^ ;
: INVERT $~ ;

: @ $@ ;
: ! $! ;
: L@ $` ;
: L! $" ;
: W@ $: ;
: W! $; ;
: C@ $. ;
: C! $, ;

: BYE $q ;

: EMIT $E ;
: KEY $K ;

: CELL $c ;
: CELL+ cell + ;
: CELLS cell * ;

: BLOCK-BASE $b ;
: REL>ABS block-base + ;
: ALG>ABS 4 * rel>abs ;
: ABS>REL block-base - ;
: HERE@ block-base cell+ ;
: HERE here@ @ rel>abs ;
: ALLOT here@ @ + here@ ! ;
: ALIGN here@ @ cell 1 - + cell 1 - invert and here@ ! ;

: , here ! cell allot ;
: L, here l! 4 allot ;
: W, here w! 2 allot ;
: C, here c! 1 allot ;

: LATEST block-base cell+ cell+ w@ alg>abs ;
: NAME>INTERPRET 2 + w@ alg>abs ;
: NAME>FLAGS 2 + 2 + ;

: FLAG-IMMEDIATE 32 ;
: IMMEDIATE latest name>flags dup c@ flag-immediate or swap c! ;

: IF 50 c, here 0 w, $$? ; immediate
: ELSE 50 c, here swap 0 w, $$j dup here swap - 3 - swap w! ; immediate
: THEN dup here swap - 3 - swap w! ; immediate

: BEGIN here ; immediate
: AGAIN 50 c, here - 3 - w, $$j ; immediate
: UNTIL 50 c, here - 3 - w, $$? ; immediate
: WHILE 50 c, here swap 0 w, $$? ; immediate
: REPEAT 50 c, here - 3 - w, $$j dup here swap - 3 - swap w! ; immediate

: DO $$($( here ; immediate
: I $))d(s( ; 
: LOOP $$)$)$1$+$o$o$($($= 50 c, here - 3 - w, $$?$)$)$_$_ ; immediate

: EXIT $$] ; immediate
: RECURSE 50 c, latest 2 + w@ w, $$a ; immediate

: TYPE 0 do dup c@ emit 1 + loop drop ;

: IS-SPACE 
	dup 32 = swap 
	dup 10 = swap 
	dup 9 = swap 
	dup 11 = swap 
	dup 12 = swap 
	dup 10 = nip 
	or or or or or 
;
