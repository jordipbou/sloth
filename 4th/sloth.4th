0 trace!

: \			source >in ! drop ; immediate

\ End of line comments can be used now. As comments are so
\ important (to me) to understand the way everything is
\ implemented an organized, and as there is no performance
\ reason to never implement \ in host, its the first word
\ defined and no conditional compilation is used.

\ SLOTH v1.0

\ Sloth is an ANS Forth implemented on top of a minimal set
\ of words to allow easy portability between platforms.
\ At the same time, and to allow high performance when its
\ needed, conditional compilation is used almost on every
\ definition to allow implementing words in host as required
\ without the need to modify this source.

\ The second definition allows conditional compilation for
\ single line definitions.

: ?: parse-name find-name [: source >in ! drop ;] [: 1 >in ! ;] choose ;

\ -- Control structures -----------------------------------

\ This words have a good balance between simplicity and
\ usefulness. Some more complicated words (like DO/LOOP)
\ are not implemented now.

?: AHEAD	postpone branch here 0 , ; immediate
?: IF		postpone ?branch here 0 , ; immediate
?: THEN		here over - swap ! ; immediate

?: ELSE		postpone ahead swap postpone then ; immediate

?: BEGIN	here ; immediate
?: UNTIL	postpone ?branch here - , ; immediate
?: AGAIN	postpone branch here - , ; immediate

?: WHILE	postpone if swap ; immediate
?: REPEAT	postpone again postpone then ; immediate

\ Now that we have conditional compilation and basic control
\ structures, we can implement a lot of fundamental words.

\ -- Stack shuffling --------------------------------------

\ The only required primitives for stack shuffling are:
\ DROP PICK OVER SWAP >R R@ R>

?: DUP		0 pick ;
?: ?DUP		dup if dup then ;
?: ROT		>r swap r> swap ;	
?: -ROT		rot rot ;
?: NIP		swap drop ;
?: TUCK		>r r@ swap r> ;

?: 2DROP	drop drop ;
?: 2DUP		over over ;
?: 2OVER	3 pick 3 pick ;
?: 2SWAP	>r -rot r> -rot ;

?: 2>R		]] swap >r >r [[ ; immediate
?: 2R@		]] r> r> swap 2dup swap >r >r [[ ; immediate
?: 2R>		]] r> r> swap [[ ; immediate

\ -- Execution --------------------------------------------

?: LITERAL	postpone LIT , ; immediate

?: VARIABLE	create 0 , ;
?: CONSTANT	create , does> @ ;

\ -- Parsing ----------------------------------------------

?: CHAR		parse-name drop c@ ;
?: [CHAR]	char postpone literal ; immediate

\ Parse (advance input) to next )
?: (		[char] ) parse 2drop ; immediate

( Now we can use stack comments too )

\ -- Comparisons ------------------------------------------

?: <= ( x1 x2 -- flag ) > invert ;
?: >= ( x1 x2 -- flag ) < invert ;
?: 0= ( x -- flag ) if 0 else 0 invert then ;
?: 0< ( x -- flag ) 0 < ;
?: 0> ( x -- flag ) 0 > ;
?: 0<> ( x -- flag ) 0= 0= ;
?: NOT ( x1 -- x2 ) 0= ;
?: = ( x1 x2 -- flag ) - 0= ;
?: <> ( x1 x2 -- flag ) = invert ;
?: U< ( u1 u2 -- flag ) 2dup xor 0< if swap drop 0< else - 0< then ;
?: U> ( u1 u2 -- flag ) swap u< ;
?: WITHIN ( n1 | u1	n2 | u2 n3 | u3 -- flag ) over - >r - r> u< ; 

\ -- Arithmetic -------------------------------------------

?: + ( x1 x2 -- x3 ) 0 swap - - ;
?: * ( x1 x2 -- x3 ) 1 */mod swap drop ;
?: / ( x1 x2 -- x3 ) 1 swap */mod swap drop ;
?: */ ( x1 x2 -- x3 ) */mod swap drop ;
?: MOD ( x1 x2 -- x3 ) 1 swap */mod drop ;
?: /MOD ( x1 x2 -- x3 ) 1 swap */mod ;

?: 1+ ( n1 -- n2 ) 1 + ;
?: 1- ( n1 -- n2 ) 1 - ;

?: 2* ( n1 -- n2 ) 2 * ;

?: ABS ( n -- u ) dup 0< if invert 1+ then ;

?: MIN ( n1 n2 -- n3 ) 2dup swap < if swap then drop ;
?: MAX ( n1 n2 -- n3 ) 2dup < if swap then drop ;

\ -- Bit\Logic --------------------------------------------

?: NEGATE ( n1 -- n2 ) invert 1+ ;

\ -- Memory -----------------------------------------------

?: CELL+	1 cells + ;
?: CHAR+	1 chars + ;

?: +! ( n | u a-addr -- ) swap over @ + swap ! ;
?: 1+! ( a-addr -- ) dup @ 1 + swap ! ;

?: 2! ( x1 x2 a-addr -- ) swap over ! cell+ ! ;
?: 2@ ( a-addr -- x1 x2 ) dup cell+ @ swap @ ;

?: MOVE ( addr1 addr2 u -- ) [: over c@ over c! 1+ swap 1+ swap ;] times 2drop ;

\ -- Input/output -----------------------------------------

?: BL ( -- char ) 32 ;
?: CR ( -- ) 10 emit ;

?: TYPE ( c-addr u -- ) [: dup c@ emit char+ ;] times drop ;

?: .( ( "ccc<paren>" -- ) [char] ) parse type ;

\ -- Dictionary/Finding names -----------------------------

?: NT>XT+FLAG	dup if dup nt>xt swap immediate? if 1 else -1 then then ;
?: SEARCH-WORDLIST	find-name-in nt>xt+flag ;

?: ' ( "<spaces>name" -- xt ) parse-name find-name nt>xt ;
?: ['] ( C: "<spaces>name" -- ) ( -- xt ) ' postpone literal ; immediate

\ -- Conditional compilation ------------------------------

?: [DEFINED]	parse-name find-name 0<> ;
?: [UNDEFINED]	parse-name find-name 0= ;

\ Implementation of [IF] [ELSE] [THEN] by ruv, as seen in:
\ https://forth-standard.org/standard/tools/BracketELSE
 
wordlist dup constant BRACKET-FLOW-WL get-current swap set-current
: [IF]			1+ ;
: [ELSE]		dup 1 = if 1- then ;
: [THEN]		1- ;
set-current

: [ELSE] \ ( -- )
 	1 begin 
 		begin 
 			parse-name dup while
			bracket-flow-wl search-wordlist if 
				execute dup 0= if drop exit then
			then
 		repeat 
 		2drop refill 0= 
 	until 
 	drop
; immediate
 
: [THEN]	; immediate 
 
: [IF]		0= if postpone [else] then ; immediate

\ -- Forth Words needed to pass test suite ----------------

\ -- Double numbers --

?: S>D ( n -- d ) dup 0< ;

?: U+D		dup rot + dup rot u< negate ;
?: D+-		0< if invert swap invert 1 u+d rot + then ;
?: M* ( n1 n2 -- d ) 2dup xor >r abs swap abs um* r> d+- ;
?: DNEGATE ( d1 -- d2 ) invert swap invert 1 u+d rot + ;
?: DABS ( d -- ud ) dup 0 < if dnegate then ;

[UNDEFINED] FM/MOD [IF]
\ Divide d1 by n1, giving the floored quotient n3 and the remainder n2.
\ Adapted from hForth
\ Taken from Swapforth
: FM/MOD \ ( d1 n1 -- n2 n3 )
    dup >r 2dup xor >r
    >r dabs r@ abs
    um/mod
    r> 0< if
        swap negate swap
    then
    r> 0< if
        negate         \ negative quotient
        over if
            r@ rot - swap 1-
        then
    then
    r> drop
;
[THEN]

?: SGN ( u1 n1 -- n2 ) 0< if negate then ;

[UNDEFINED] SM/REM [IF]
\ Divide d1 by n1, giving the symmetric quotient n3 and the remainder n2.
\ Taken from SwapForth
: SM/REM \ ( d1 n1 -- n2 n3 )
    2dup xor >r     \ combined sign, for quotient
    over >r         \ sign of dividend, for remainder
    abs >r dabs r>
    um/mod          \ ( remainder quotient )
    swap r> sgn     \ apply to remainder
    swap r> sgn     \ apply to quotient
;
[THEN]

\ -- Do/Loop --

?: DO		postpone [: ; immediate
?: ?DO		postpone [: ; immediate
?: LOOP		postpone lit 1 , postpone ;] postpone doloop ; immediate
?: +LOOP	postpone ;] postpone doloop ; immediate

?: UNLOOP	; \ Unloop is a noop in this implementation

\ -- Word and counted strings --

?: /STRING		tuck - >r chars + r> ;		\ ( c-addr1 u1 n -- c-addr2 u2 )

\ Puts on stack the non parsed area of source
?: /SOURCE		source >in @ /string dup 0< if drop 0 then ; \ ( -- c-addr u )

[UNDEFINED] >COUNTED [IF]
: >COUNTED ( c-addr u -- c-addr )
	dup 1+ chars tallot
	dup >r
	2dup c!
	char+ swap chars move
	r>
;
[THEN]

?: COUNT ( c-addr1 -- c-addr2 u ) dup c@ swap char+ swap ;

[UNDEFINED] WORD [IF]
variable wlen
: WORD ( char "<chars>ccc<char>" -- c-addr )
	>r 
	begin /source nip 0> while /source drop c@ r@ = while >in 1+! repeat then
	/source drop 0 wlen !
	begin /source nip 0> while /source drop c@ r@ <> while >in 1+! wlen 1+! repeat then
	>in @ source nip < if >in 1+! then
	wlen @
	>counted
	r> drop
;
[THEN]

?: FIND			dup count find-name nt>xt+flag dup if rot drop then ;

\ -- Testing --

: dotests -1 trace! s" ../../../forth2012-test-suite/src/runtests.fth" included ;

1 trace!
