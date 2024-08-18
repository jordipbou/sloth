0 trace!

: \			source >in ! drop ; immediate

\ End of line comments can be used now.

\ SLOTH v1.0

\ Sloth is an ANS Forth implemented on top of a minimal set
\ of words to allow easy portability between platforms.
\ At the same time, and to allow high performance when its
\ needed, conditional compilation is used almost on every
\ definition to allow implementing words in host as required
\ without the need to modify this source.

\ The second definition allows conditional compilation for
\ single line definitions.

\ TODO: Is it possible to do some magic to not need choose here?

: ?: parse-name find-name [: source >in ! drop ;] [: 1 >in ! ;] choose ;

\ -- Compilation ------------------------------------------

?: ,		here ! 1 cells allot ;
?: C,		here c! 1 chars allot ;

\ PLATFORM DEPENDENT
\ A system that prefers to pass everything thru the stack
\ (like ILO) could compile a literal as , postpone LIT
?: LITERAL	postpone LIT , ; immediate

\ -- Control structures -----------------------------------

\ This words have a good balance between simplicity and
\ usefulness. Some more complicated words (like DO/LOOP)
\ are not implemented now.

\ PLATFORM DEPENDENT (see comment for LITERAL)
?: AHEAD	postpone branch here 0 , ; immediate
\ PLATFORM DEPENDENT (see comment for LITERAL)
?: IF		postpone ?branch here 0 , ; immediate
\ I'm not sure if THEN is platform dependent, will it
\ work correctly if AHEAD or IF are compiled with
\ the literal first?
?: THEN		here over - swap ! ; immediate

?: ELSE		postpone ahead swap postpone then ; immediate

?: BEGIN	here ; immediate
\ PLATFORM DEPENDENT (see comment for LITERAL)
?: UNTIL	postpone ?branch here - , ; immediate
\ PLATFORM DEPENDENT (see comment for LITERAL)
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
?: ROLL		dup if swap >r 1 - recurse r> swap exit then drop ;

?: 2DROP	drop drop ;
?: 2DUP		over over ;
?: 2OVER	3 pick 3 pick ;

?: 2SWAP	>r -rot r> -rot ;

?: 2>R		]] swap >r >r [[ ; immediate
?: 2R@		]] r> r> swap 2dup swap >r >r [[ ; immediate
?: 2R>		]] r> r> swap [[ ; immediate

?: 2RDROP	]] r> r> drop drop [[ ; immediate

\ -- Parsing ----------------------------------------------

?: CHAR		parse-name drop c@ ;
?: [CHAR]	char postpone literal ; immediate

\ Parse (advance input) to next ) -- not multiline --
?: (		[char] ) parse 2drop ; immediate

( Now we can use stack comments too )

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

\ -- Bit\Logic --------------------------------------------

?: NEGATE ( n1 -- n2 ) invert 1+ ;

?: OR ( x1 x2 -- x3 ) invert swap invert and invert ;
?: XOR ( x1 x2 -- x3 ) over over invert and >r swap invert and r> or ;

\ -- Comparisons ------------------------------------------

?: < ( n1 n2 -- flag ) 2dup xor 0< if drop 0< else - 0< then ;
?: > ( n1 n2 -- flag ) swap < ;
?: <= ( x1 x2 -- flag ) > invert ;
?: >= ( x1 x2 -- flag ) < invert ;
?: 0= ( x -- flag ) if 0 else 0 invert then ;
?: 0> ( x -- flag ) 0 > ;
?: 0<> ( x -- flag ) 0= 0= ;
?: NOT ( x1 -- x2 ) 0= ;
?: = ( x1 x2 -- flag ) - 0= ;
?: <> ( x1 x2 -- flag ) = invert ;
?: U< ( u1 u2 -- flag ) 2dup xor 0< if swap drop 0< else - 0< then ;
?: U> ( u1 u2 -- flag ) swap u< ;
?: WITHIN ( n1 | u1	n2 | u2 n3 | u3 -- flag ) over - >r - r> u< ; 

?: MIN ( n1 n2 -- n3 ) 2dup swap < if swap then drop ;
?: MAX ( n1 n2 -- n3 ) 2dup < if swap then drop ;

\ -- Memory -----------------------------------------------

?: CELL		1 cells ;

?: CELL+	1 cells + ;
?: CHAR+	1 chars + ;
?: CELL-	1 cells - ;
?: CHAR-	1 chars - ;

?: ALIGNED ( addr -- a-addr ) cell+ 1- 1 cells 1- invert and ;
?: ALIGN ( -- ) here aligned here - allot ;

?: 0! ( a-addr -- ) 0 swap ! ;
?: +! ( n | u a-addr -- ) swap over @ + swap ! ;
?: 1+! ( a-addr -- ) dup @ 1 + swap ! ;
?: 1-! ( a-addr -- ) dup @ 1 - swap ! ;

?: 2! ( x1 x2 a-addr -- ) swap over ! cell+ ! ;
?: 2@ ( a-addr -- x1 x2 ) dup cell+ @ swap @ ;

\ BMOVE deals with adress units and BMOVE> and <BMOVE are
\ used to implement MOVE with also deals with adress
\ units.
?: BMOVE> ( addr1 addr2 u -- ) >r r@ 1- + swap r@ 1- + swap r> [: over b@ over b! 1- swap 1- swap ;] times 2drop ;
?: <BMOVE ( addr1 addr2 u -- ) [: over b@ over b! 1+ swap 1+ swap ;] times 2drop ;
?: MOVE ( addr1 addr2 u -- ) >r 2dup u< if r> bmove> else r> <bmove then ;
\ CMOVE deals with CHARS and can be implemented with
\ MOVE.
?: CMOVE> ( c-addr1 c-addr2 u -- ) chars bmove> ;
?: CMOVE ( c-addr1 c-addr2 u -- ) chars <bmove ;

?: FILL ( c-addr u char -- ) -rot [: 2dup c! char+ ;] times 2drop ;

?: BOUNDS ( c-addr u -- c-addr c-addr ) over + swap ;

\ -- Header/Colon -----------------------------------------

\ Now that I have the ability to copy blocks of memory
\ its easier to create headers.

\ PLATFORM DEPENDENT
?: >XT ( nt u -- ) swap cell+ ! ;

\ PLATFORM DEPENDENT
?: HEADER ( c-addr u -- ) align here latest@ , latest! 0 , get-current , 0 c, dup c, here swap dup chars allot cmove align latest@ here >xt ;

\ PLATFORM DEPENDENT
?: NAME>LINK ( nt1 -- nt2 ) @ ;
\ PLATFORM DEPENDENT
?: NAME>XT ( nt -- xt ) cell+ @ ;
\ PLATFORM DEPENDENT
?: NAME>WORDLIST ( nt -- wordlist ) 2 cells + @ ;
\ PLATFORM DEPENDENT
?: NAME>FLAGS ( nt -- flag ) 3 cells + c@ ;
\ PLATFORM DEPENDENT
?: NAME>STRING ( nt -- c-addr u ) dup 3 cells + char+ c@ swap 3 cells + 2 chars + swap ;

\ -- Variables, constants and defer words -----------------

?: VARIABLE	create 0 , ;
?: CONSTANT	create , does> @ ;

\ PLATFORM DEPENDENT - NON ANS
2 constant IMMEDIATE-FLAG
\ PLATFORM DEPENDENT - NON ANS
?: IMMEDIATE? ( -- flag ) NAME>FLAGS IMMEDIATE-FLAG AND ;

?: NAME>INTERPRET dup 0= if 0 else name>xt then ;

?: '		parse-name find-name name>interpret ;
?: [']		' postpone literal ; immediate

?: NAME>COMPILE dup 0<> if dup immediate? if name>interpret ['] execute else name>interpret ['] compile, then then ;

?: VALUE	create , does> @ ;
?: DEFER	create 0 , does> @ execute ;
?: TO		state @ if postpone ['] postpone >body postpone ! else ' >body ! then ; immediate
?: IS		state @ if postpone to else ['] to execute then ; immediate 

?: DEFER@	>body @ ;
?: DEFER!	>body ! ;

?: ACTION-OF state @ if ]] ['] defer@ [[ else ' defer@ then ; immediate

\ -- Markers ----------------------------------------------

\ TODO: What should marker do?
\ Store current here and on execution set here to that value.
\ Store current latest and on execution set latest to that value.

?: MARKER ( "<spaces>name" -- ) here latest@ parse-name header ]] literal latest! here literal - allot exit [[ ;

\ -- Input/output -----------------------------------------

?: BL ( -- char ) 32 ;
?: CR ( -- ) 10 emit ;

?: SPACE ( -- ) bl emit ;
?: SPACES ( n -- ) begin dup 0> while space 1- repeat drop ;

?: TYPE ( c-addr u -- ) [: dup c@ emit char+ ;] times drop ;

?: .( ( "ccc<paren>" -- ) [char] ) parse type ;

?: ." ( C: "ccc<quote>" -- ) ( -- ) postpone s" state @ if postpone type else type then ; immediate

\ -- Dictionary/Finding names -----------------------------

?: NT>XT+FLAG	dup if dup name>interpret swap immediate? if 1 else -1 then then ;
?: SEARCH-WORDLIST	find-name-in nt>xt+flag ;

\ -- Conditional compilation ------------------------------

?: [DEFINED]	parse-name find-name 0<> ;
?: [UNDEFINED]	parse-name find-name 0= ;

\ Implementation of [IF] [ELSE] [THEN] by ruv, as seen in:
\ https://forth-standard.org/standard/tools/BracketELSE
 
wordlist dup constant BRACKET-FLOW-WL 
get-current swap set-current
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

\ -- True/False constants ---------------------------------

[UNDEFINED] TRUE [IF] 0 invert constant TRUE [THEN]
[UNDEFINED] FALSE [IF] 0 constant FALSE [THEN]

\ -- Return stack manipulation of multiple items ----------

[UNDEFINED] N>R [IF]
\ Reference implementation in ANS Forth document
: N>R ( xn .. x1 N -- ) ( R: -- x1 .. xn n )
\ Transfer N items and count to the return stack.
   dup                        \ xn .. x1 N N --
   begin
      dup
   while
      rot r> swap >r >r      \ xn .. N N -- ; R: .. x1 --
      1-                      \ xn .. N 'N -- ; R: .. x1 --
   repeat
   drop                       \ N -- ; R: x1 .. xn --
   r> swap >r >r
;
[THEN]

[UNDEFINED] NR> [IF]
\ Reference implementation in ANS Forth document
: NR> ( -- xn .. x1 N ) ( R: x1 .. xn N -- )
\ Pull N items and count off the return stack.
   r> r> swap >r dup
   begin
      dup
   while
      r> r> swap >r -rot
      1-
   repeat
   drop
;
[THEN]

\ -- Double numbers ---------------------------------------

?: S>D ( n -- d ) dup 0< ;

?: U+D		dup rot + dup rot u< negate ;
?: D+		>r rot u+d rot + r> + ;
?: D+-		0< if invert swap invert 1 u+d rot + then ;
?: M* ( n1 n2 -- d ) 2dup xor >r abs swap abs um* r> d+- ;
?: DNEGATE ( d1 -- d2 ) invert swap invert 1 u+d rot + ;
?: DABS ( d -- ud ) dup 0 < if dnegate then ;
?: UD/MOD	>r 0 r@ um/mod r> swap >r um/mod r> ;

[UNDEFINED] FM/MOD [IF]
\ Divide d1 by n1, giving the floored quotient n3 and the 
\ remainder n2.
\ Taken from Swapforth (Adapted from hForth)
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
\ Divide d1 by n1, giving the symmetric quotient n3 and the 
\ remainder n2.
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

\ -- Numeric output (taken from SwapForth?) ---------------

variable HLD
create <HOLD 100 chars dup allot <hold + constant HOLD>

?: <# ( -- ) hold> hld ! ;
?: HOLD ( char -- ) hld @ char- dup hld ! c! ;
?: # ( d1 -- d2 ) base @ ud/mod rot 9 over < if 7 + then 48 + hold ;
?: #S ( d1 -- d2 ) begin # over over or 0= until ;
?: #> ( d -- c-addr u ) drop drop hld @ hold> over - 1 chars / ;

?: SIGN		0 < if 45 hold then ;

?: UD.R ( d n -- ) >r <# #s #> r> over - spaces type ;
?: UD.		0 ud.r space ;
?: U.R		0 swap ud.r ;
?: U. ( u -- ) 0 ud. ;
?: D.R ( d n -- ) >r swap over dabs <# #s rot sign #> r> over - spaces type ;
?: D.		0 d.r space ;
?: .R		>r dup 0 < r> d.r ;
?: .			dup 0 < d. ;

?: ? ( addr -- ) @ . ;

\ -- Do/Loop (implemented with combinators) ---------------

\ TODO Implement the combinators in high level Forth itself
\ TODO Implement DO/LOOP in non-combinator high level way

\ DO/?DO/LOOP/+LOOP is one of the things I don't like about the ANS Forth
\ standard. IF/ELSE/THEN or BEGIN/WHILE/REPEAT have one word that does one
\ thing. You can mix them and get different results, different constructions,
\ but the word meaning is clear and easily implemented.
\ DO/?DO/LOOP/+LOOP is a combination of words with different semantics that
\ have to work together.

?: DO		postpone lit 1 , postpone [: ; immediate
?: ?DO		postpone lit 0 , postpone [: ; immediate
?: LOOP		postpone lit 1 , postpone ;] postpone doloop ; immediate
?: +LOOP	postpone ;] postpone doloop ; immediate

\ -- Setting BASE independent of current BASE setting --
\ ( as seen on https://forth-standard.org/standard/core/HEX#reply-816 )

[undefined] BASE [if] variable BASE [then]

base @ \ remember current BASE value
1 2* base !	\ get to binary
?: DECIMAL	1010 base ! ;
?: HEX		10000 base ! ;
base ! \ restore previous base

\ -- Number conversion --

\ TODO: This number conversion seems to be pretty basic.

[undefined] DIGIT [if]
\ DIGIT Taken from pFORTH
\ Convert a single character to a number in the given base.
: DIGIT   ( char base -- n true | char false )
    >r
\ convert lower to upper
    dup [char] a < not
    if
        [char] a - [char] A +
    then

    dup dup [char] A 1- >
    if [char] A - [char] 9 + 1+
    else ( char char )
        dup [char] 9 >
        if
            ( between 9 and A is bad )
            drop 0 ( trigger error below )
        then
    then
    [char] 0 -
    dup r> <
    if dup 1+ 0>
        if nip true
        else drop false
        then
    else drop false
    then
;
[then]

[undefined] >NUMBER [if]
\ >NUMBER taken from pForth
: >NUMBER ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 , convert till bad char , CORE )
    >r
    begin
        r@ 0>    \ any characters left?
        if
            dup c@ base @
            digit ( ud1 c-addr , n true | char false )
            if
                true
            else
                drop false
            then
        else
            false
        then
    while ( -- ud1 c-addr n  )
        swap >r  ( -- ud1lo ud1hi n  )
        swap  base @ ( -- ud1lo n ud1hi base  )
        um* drop ( -- ud1lo n ud1hi*baselo  )
        rot  base @ ( -- n ud1hi*baselo ud1lo base )
        um* ( -- n ud1hi*baselo ud1lo*basello ud1lo*baselhi )
        d+  ( -- ud2 )
        r> char+     \ increment char*
        r> 1- >r  \ decrement count
    repeat
    r>
;
[then]

\ -- Stacks -----------------------------------------------

\ Taken from:
\ https://forth-standard.org/proposals/minimalistic-core-api-for-recognizers#reply-515

[undefined] STACK: 
[undefined] SET-STACK 
[undefined] GET-STACK
and and [if]
: STACK: ( size "name" -- ) create 0 , cells allot ;

: SET-STACK ( item-n .. item-1 n stack-id -- )
	2dup ! cell+ swap cells bounds
	?do i ! cell +loop 
;

: GET-STACK ( stack-id -- item-n .. item-1 n )
	dup @ >r r@ cells + r@ begin
		?dup while
		1- over @ rot cell - rot
	repeat
	drop r> 
;
[then]

\ -- Recognizers ------------------------------------------

\ Minimal reference implementation taken from:
\ https://forth-standard.org/proposals/minimalistic-core-api-for-recognizers#reply-515

defer forth-recognize ( addr u -- i*x translator-xt / notfound )

: interpret ( i*x -- j*x )
	begin
		\ ?stack \ TODO: ?stack is not implemented right now
		parse-name dup while
		forth-recognize execute
	repeat
;

: lit,  ( n -- )  postpone literal ;
: notfound ( state -- ) -13 throw ;
: translate: ( xt-interpret xt-compile xt-postpone "name" -- )
	create , , ,
	does> state @ 2 + cells + @ execute 
;
:noname name>interpret execute ;
:noname name>compile execute ;
:noname name>compile swap lit, compile, ;
translate: translate-nt ( nt -- )
' noop
' lit,
:noname lit, postpone lit, ;
translate: translate-num ( n -- )

: rec-nt ( addr u -- nt nt-translator / notfound )
	1 \ forth-wordlist \ TODO: Still don't have wordlists correctly
	find-name-in dup if
		['] translate-nt
	else
		drop ['] notfound
	then
;
: rec-num ( addr u -- n num-translator / notfound )
	0 0 \ 0. \ double number literals are not implemented yet
	2swap >number 0= if
		2drop ['] translate-num
	else
		2drop drop ['] notfound
	then
;

: minimal-recognize ( addr u -- nt nt-translator / n num-translator / notfound )
	2>r	2r@	rec-nt dup ['] notfound	= if
		drop 2r@ rec-num
	then
	2rdrop
;

' minimal-recognize is forth-recognize

\ -- Recognizers extensions -------------------------------

: set-forth-recognize ( xt -- ) is forth-recognize ;

: forth-recognizer ( -- xt ) action-of forth-recognize ;


\ -- Transient memory -------------------------------------

\ Its difficult to define transient memory here as I use it
\ for bootstrapping....

\ [UNDEFINED] THERE [IF]
\ variable (THERE)
\ here unused + (there) !
\ : THERE (there) @ ;
\ : TALLOT (there) @ dup >r - (there) ! r> ;
\ [THEN]


\ -- Forth Words needed to pass test suite ----------------

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
