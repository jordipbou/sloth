?: \	source >in ! drop ; immediate

\ TODO Use three variables (like registers) for i, j and k indexes, 
\ with some auto operations for them, like i@+ or i@j!+ (for copying arrays)
\ That will allow using same indexes both in do/loop form and in times/iter forms.
\ Also, it will make easy to implement fill/cmove/etc in top of do/loop and bounds

\ This is necessary to test in another Forth implementation,
\ but I need  parse-name find-name if else then  to be able to use it

\ : ?: parse-name find-name if source >in ! drop else 1 >in ! then ;

\ REQUIRED PRIMITIVES:

\ >R R>
\ DROP SWAP PICK
\ - */MOD
\ INVERT
\ ?: : ; IMMEDIATE POSTPONE '
\ CELLS CHARS ALLOT HERE UNUSED

\ -----------------------------------------------------------------------------

?: NOOP			;						\ ( -- )

?: [			0 state ! ; immediate	\ ( -- ) ( STATE: 0 )
?: ]			1 state ! ; immediate	\ ( -- ) ( STATE: 1 )

?: DUP			0 pick ;				\ ( x -- x x )
?: OVER			1 pick ;				\ ( x1 x2 -- x1 x2 x1 )
?: NIP			swap drop ;				\ ( x1 x2 -- x2 )
?: TUCK			swap over ;				\ ( x1 x2 -- x2 x1 x2 )
?: ROT			>r swap r> swap ;		\ ( x1 x2 x3 -- x2 x3 x1 )
?: -ROT			rot rot ;				\ ( x1 x2 x3 -- x3 x1 x2 ) Non ANS

?: 2DUP			over over ;				\ ( x1 x2 -- x1 x2 x1 x2 )
?: 2DROP		drop drop ;				\ ( x1 x2 -- )
?: 2OVER		3 pick 3 pick ;			\ ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 )
?: 2SWAP		rot >r rot r> ; 		\ ( x1 x2 x3 x4 -- x3 x4 x1 x2 )

?: R@			r> r> dup >r swap >r ;	\ ( -- x ) ( R: x -- x )

\ ( x1 x2 -- ) ( R: -- x1 x2 )
?: 2>R		postpone swap postpone >r postpone >r ; immediate
\ ( -- x1 x2 ) ( R: x1 x2 -- )
?: 2R>		postpone r> postpone r> postpone swap ; immediate
\ ( -- x1 x2 ) ( R: x1 x2 -- x1 x2 )
?: 2R@		postpone r> postpone r> postpone 2dup postpone 2>r ; immediate

?: 2ROT		2>r 2swap 2r> 2swap ; \ ( x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2 )

?: +			0 swap - - ;			\ ( x1 x2 -- x3 )
?: *			1 */mod swap drop ;		\ ( x1 x2 -- x3 )
?: /			1 swap */mod swap drop ;	\ ( x1 x2 -- x3 )
?: */			*/mod swap drop ;		\ ( x1 x2 -- x3 )
?: MOD			1 swap */mod drop ;		\ ( x1 x2 -- x3 )
?: /MOD			1 swap */mod ;			\ ( x1 x2 -- x3 )
?: 2*			dup + ;					\ ( x1 x2 -- x3 )

?: 1-			1 - ;					\ ( x1 -- x2 )
?: 1+			1 + ;					\ ( x1 -- x2 )
?: 2+			2 + ;					\ ( x1 -- x2 )

?: OR		invert swap invert and invert ;	\ ( x1 x2 -- x3 )
?: XOR		over over invert and >r swap invert and r> or ; \ ( x1 x2 -- x3 )

?: NEGATE		invert 1+ ;				\ ( x1 -- x2 )

?: CELL			1 cells ;				\ ( a-addr1 -- a-addr2 )
?: CHAR+		1 chars + ;				\ ( c-addr1 -- c-addr2 )
?: CELL+		1 cells + ; 			\ ( a-addr1 -- a-addr2 )
?: CELL-		1 cells - ; 			\ ( a-addr1 -- a-addr2 )

?: ,			here ! 1 cells allot ;	\ ( x -- )
?: C,			here c! 1 chars allot ; \ ( char -- )
 
?: +!			swap over @ + swap ! ;	\ ( n | u a-addr -- )
?: 0!			0 swap ! ;				\ ( a-addr -- ) non ANS
?: 1+!			dup @ 1+ swap ! ;		\ ( a-addr -- ) non ANS
?: 1-!			dup @ 1- swap ! ;		\ ( a-addr -- ) non ANS

?: 2!			swap over ! cell+ ! ;	\ ( x1 x2 a-addr -- )
?: 2@			dup cell+ @ swap @ ;	\ ( a-addr -- x1 x2 )

?: ALIGNED		cell+ 1- 1 cells 1- invert and ;	\ ( addr -- a-addr )
?: ALIGN		here aligned here - allot ;			\ ( -- )

?: CONSTANT		create , does> @ ;		\ ( x "name" -- )
?: VARIABLE		create 0 , ;			\ ( "name" -- )

?: 2CONSTANT	create , , does> 2@ ;	\ ( x1 x2 "name" -- )
?: 2VARIABLE	create 0 , 0 , ;		\ ( "name" -- )

0				constant FALSE
false invert	constant TRUE

?: ON			true swap ! ;			\ ( a-addr -- )
?: OFF			false swap ! ;			\ ( a-addr -- )

?: LITERAL		postpone doLIT , ; immediate 

?: [']			' postpone literal ; immediate 
?: 2LITERAL		swap postpone literal postpone literal ; 

\ ?: RECURSE		latestxt @ , ; immediate						\ Impl. dep.
\ ?: :NONAME		here dup latestxt ! postpone ] ;				\ Impl. dep.

\ -----------------------------------------------------------------------------

\ Control structures

\ ?: >MARK		here 0 , ;										\ Impl. dep.
\ ?: >RESOLVE		here over - swap ! ;							\ Impl. dep.
\ ?: >>RESOLVE	here swap ! ; \ Absolute address >resolve		\ Impl. dep.
\ 
\ ?: <MARK		here ;											\ Impl. dep.
\ ?: <RESOLVE		here - , ;										\ Impl. dep.

\ ?: AHEAD		postpone branch >mark ; immediate				\ Impl. dep.
\ ?: IF			postpone ?branch >mark ; immediate				\ Impl. dep.
\ ?: THEN			>resolve ; immediate							\ Impl. dep.

\ ?: BEGIN		<mark ; immediate								\ Impl. dep.
\ ?: UNTIL		postpone ?branch <resolve ; immediate			\ Impl. dep.
\ ?: AGAIN		postpone branch <resolve ; immediate			\ Impl. dep.

\ TODO swap here should be 1 CS-ROLL to be standarad

?: ELSE			postpone ahead swap postpone then ; immediate

?: WHILE		postpone if swap ; immediate
?: REPEAT		postpone again postpone then ; immediate

\ -----------------------------------------------------------------------------

\ Basic words that could not be defined until we had control structures

?: ?DUP			dup if dup then ; \ ( x -- 0 | x x )

?: <			2dup xor 0< if drop 0< else - 0< then ; \ ( n1 n2 -- flag )
?: >			swap < ;								\ ( n1 n2 -- flag )
?: <=			> invert ;								\ ( n1 n2 -- flag )
?: >=			< invert ;					\ ( n1 n2 -- flag )
?: 0=			if 0 else 0 invert then ;	\ ( x -- flag )
?: 0>			0 > ;						\ ( n -- flag )
?: 0<>			0= 0= ;						\ ( n -- flag )
?: NOT			0= ;						\ ( x -- flag ) Non ANS
?: =			- 0= ;						\ ( x1 x2 -- flag )
?: <>			= invert ;					\ ( x1 x2 -- flag )
?: U<			2dup xor 0< if swap drop 0< else - 0< then ; \ ( u1 u2 -- flag )
?: U>			swap u< ;					\ ( u1 u2 -- flag )
?: WITHIN		over - >r - r> u< ; 

?: MIN			2dup swap < if swap then drop ;
?: MAX			2dup	  < if swap then drop ;

?: -IF			postpone 0= postpone if ; immediate

\ TODO Set latesxt to quotation start !!! In both situations, previous one must be saved,
\ as quotations can be nested (inside colon definitions and inside other quotations)
\ ?: [:	state @ if postpone BLOCK >mark else -1 state ! here then ; immediate \ Impl. dep. 
\ ?: ;]	postpone EXIT state @ 0< if state off else >resolve then ; immediate \ Impl. dep.

\ -----------------------------------------------------------------------------

\ Dataflow combinators 

?: DIP		swap >r execute r> ; \ ( n xt -- n )
?: 2DIP		-rot 2>r execute 2r> ; \ ( n1 n2 xt -- n1 n2 )
?: 3DIP		swap >r swap >r swap >r execute r> r> r> ; \ ( n1 n2 n3 xt -- n1 n2 n3 )
?: 4DIP		swap >r swap >r swap >r swap >r execute r> r> r> r> ; \ ( n1 n2 n3 n4 xt -- n1 n2 n3 n4 )

?: KEEP		over >r execute r> ; \ ( n xt[ n -- i*x ] -- i*x n )
?: 2KEEP	2 pick 2 pick >r >r execute r> r> ; \ ( n1 n2 xt -- n1 n2 )
?: 3KEEP	3 pick 3 pick 3 pick >r >r >r execute r> r> r> ; \ ( n1 n2 n3 xt -- n1 n2 n3 )

\ -----------------------------------------------------------------------------

\ Additional stacks

?: STACK:		create dup , 0 , cells allot does> 2 cells + ; \ ( u "name" -- )
?: >STACK 		[: dup cell- @ cells + ! ;] keep cell- 1+! ; \ ( x sid -- )
?: STACK>		dup cell- 1-! dup cell- @ cells + @ ; \ ( sid -- x )
?: @STACK 		dup cell- @ rot 1+ - cells + ; \ ( u sid -- )

\ -----------------------------------------------------------------------------

\ TODO This is overly complicated because it uses the stack as it used R
\ Putting and taking form it

64 stack: INDEXES
 
: >IDX				indexes >stack ;
: IDX>				indexes stack> ;

: I					1 indexes @stack @ ;
: J					4 indexes @stack @ ;
: K					7 indexes @stack @ ;

: (INIT)			postpone doLIT >mark postpone >idx ;
: (BEGIN)			postpone begin postpone >idx postpone >idx ;
: (CHECK-BOUNDS)	2dup = if 2drop r> drop idx> >r exit then ;

: (GET-BOUNDS)		postpone idx> postpone idx> ;
: (CLEAN)			postpone 2drop postpone idx> postpone drop ;

: DO (init) (begin) ; immediate \ ( limit start )
: ?DO (init) postpone (check-bounds) (begin) ; immediate \ ( limit start -- )

: (LOOP) 1+ 2dup = ;
: LOOP (get-bounds) postpone (loop) postpone until (clean) >>resolve ; immediate

\ TODO This will fail if limit is smaller than index and n is positive, as its 
\ circular arithmetic and I don't check it here right now.
: (+LOOP) rot dup 0< if + 2dup swap < else + 2dup swap 1- > then ;
: +LOOP	(get-bounds) postpone (+loop) postpone until (clean) >>resolve ; immediate

: LEAVE idx> idx> 2drop r> drop idx> >r exit ;

: UNLOOP (get-bounds) (clean) ; immediate

\ -----------------------------------------------------------------------------

?: BOUNDS	over + swap ; \ ( addr u -- u u )
\ ?: ITER		-rot cells bounds ?do i @ swap dup >r execute r> cell +loop drop ; \ ( addr u xt -- )
?: ITER		>r cells bounds ?do i @ r@ execute cell +loop drop r> drop ;
\ Executes xt for each item of the array. If xt returns true, leaves the loop and returns -1
\ else ends the loop and returns 0
?: *ITER	-rot cells bounds ?do i @ swap dup >r execute r> swap ?dup if nip unloop exit then cell +loop drop 0 ; \ ( addr u xt -- )

?: TIMES		0 swap >r ?do r@ execute loop r> drop ;
?: ITER/ADDR	swap [: dup >r keep cell+ r> ;] times 2drop ; \ ( addr u xt -- )
?: -ITER/ADDR	over 2>r 1- cells + 2r> [: dup >r keep cell- r> ;] times 2drop ; \ ( addr u xt -- )
?: CITER/ADDR	swap [: dup >r keep char+ r> ;] times 2drop ; \ ( c-addr u xt -- )
?: CITER		-rot [: dup c@ -rot over 2dip char+ ;] times 2drop ; \ ( c-addr u xt -- )

\ -----------------------------------------------------------------------------

?: SET-STACK 	[: swap [: ! ;] iter/addr ;] 2keep cell- ! ; \ ( xn .. x1 n sid -- )
?: GET-STACK 	dup cell- @ [: [: @ ;] -iter/addr ;] keep ; \ ( sid -- xn .. x1 n )

: MAP-STACK		dup cell- @ rot *iter ; \ ( xt rid -- f )

\ -----------------------------------------------------------------------------

\ Wordlists

16 stack: ORDER
variable LAST-WORDLIST-ID 1 last-wordlist-id !
\ variable CURRENT

?: SET-ORDER	order set-stack ; \ ( widn ... wid1 n -- )
?: GET-ORDER	order get-stack ; \ -- widn .. wid1 n )
?: +ORDER		>r get-order 1+ r> swap set-order ; \ ( xt -- )
?: -ORDER		get-order nip 1- set-order ; \ ( -- )
?: SET-CURRENT	current ! ; \ ( wid -- )
?: GET-CURRENT	current @ ; \ ( -- wid )
?: WORDLIST		last-wordlist-id @ 1+ dup last-wordlist-id ! ; \ ( -- wid )
?: DEFINITIONS	get-order over set-current set-order ; \ ( -- )
?: FORTH-WORDLIST 1 ; \ ( -- 1 )

forth-wordlist	set-current
forth-wordlist	1 set-order

\ -----------------------------------------------------------------------------

?: /STRING		tuck - >r chars + r> ; 
?: /SOURCE		source >in @ /string dup 0< if drop 0 then ; 

?: <P			/source ;
?: *P			>r begin /source nip while /source drop c@ r@ execute while >in 1+! repeat then r> drop ;
?: P>			/source nip dup >r - r> if >in 1+! then ;

?: PARSE		<p rot [: over <> ;] *p drop p> ;

?: (			41 parse drop drop ; immediate 

?: BL			32 ; 
?: CR			10 emit ;

?: SPACE		bl emit ;
?: SPACES		begin dup 0> while space 1- repeat drop ;

?: CHAR			bl parse drop c@ ; 
?: [CHAR]		char postpone literal ; immediate 

?: PARSE-NAME	[: bl <= ;] *P <P [: bl > ;] *P P> ;
\ I don't need (PARSE-NAME) as it will not change hosts implementation
\ ' PARSE-NAME (PARSE-NAME) !

\ -----------------------------------------------------------------------------

?: TEND		here unused + ; 
?: TMARGIN	here 256 + ; 
variable THERE tmargin THERE !
?: TALLOT >r there @ r@ + tmargin < if tmargin there ! then	there @ r@ + tend > if tmargin there ! then	there @	there @ r@ + there ! r> drop ;

\ -----------------------------------------------------------------------------

?: CMOVE	[: over c@ over c! 1+ swap 1+ swap ;] times 2drop ;
?: CMOVE>	1- rot over + -rot dup rot + swap 1+ [: over c@ over c! 1- swap 1- swap ;] times 2drop ;

?: MOVE		>r 2dup u< if r> cmove> else r> cmove then ; \ ( addr1 addr2 u -- )

?: FILL		-rot [: over swap c! ;] citer/addr drop ; \ ( c-addr u char -- )
?: ERASE	0 fill ; \ ( c-addr u -- )

?: TYPE		[: dup c@ emit char+ ;] times drop ;

?: >COUNTED	dup 1+ tallot dup >r 2dup c! 1+ swap cmove r> ; 

?: WORD		[: over = ;] *P parse >counted ;

?: PAD		here 80 + ; 

\ -----------------------------------------------------------------------------

\ 4 constant IMMEDIATE-FLAG											\ Impl. dep.
\ 2 constant COLON-FLAG												\ Impl. dep.
\ 1 constant HIDDEN-FLAG												\ Impl. dep.
\ 
\ ?: NT>LINK		@ ;													\ Impl. dep.
\ ?: NT>XT		1 cells + @ ;										\ Impl. dep.
\ ?: NT>DT		2 cells + @ ;										\ Impl. dep.
\ ?: NT>WORDLIST	3 cells + c@ ;										\ Impl. dep.
\ ?: NT>FLAGS		3 cells + 1 chars + c@ ;							\ Impl. dep.
\ ?: NAME>STRING	3 cells + 2 chars + dup c@ swap 1 chars + swap ;	\ Impl. dep.
\ 
\ ?: HAS-FLAG?	swap nt>flags over and = ;							\ Impl. dep.
\ 
\ ?: IMMEDIATE?	immediate-flag has-flag? ;							\ Impl. dep.
\ ?: COLON?		colon-flag has-flag? ;								\ Impl. dep.
\ ?: HIDDEN?		hidden-flag has-flag? ;								\ Impl. dep.

\ -----------------------------------------------------------------------------

?: UPPER ( u1 -- u2 ) dup dup 97 >= swap 122 <= and if 32 - then ;

: COMPARE-WITHOUT-CASE ( c-addr1 u1 c-addr2 u2 -- flag )
	rot over - if drop drop drop 0 exit then
	>r
	begin
		r@ while
		over c@ upper over c@ upper - if drop drop r> drop 0 exit then
		1+ swap 1+ swap r> 1- >r 
	repeat
	drop drop r> drop
	-1
;

: FIND-NAME-IN ( c-addr u wid -- nt | 0 )
	>r
	latest @
	begin
		dup while
		dup hidden? invert over nt>wordlist r@ = and if
			dup >r name>string 2over compare-without-case if 
				2drop r> r> drop exit 
			else 
				r> 
			then
		then
		nt>link
	repeat
	drop drop drop r> drop 0
;

\ ( c-addr u -- 0 | nt )
?: FIND-NAME	[: >r 2dup r> find-name-in ;] order map-stack nip nip ;

\ ( c-addr u wid -- 0 | xt 1 | xt - 1 )
?: SEARCH-WORDLIST find-name-in dup if dup immediate? if nt>xt 1 else nt>xt -1 then then ;

\ -----------------------------------------------------------------------------

\ : HEADER ( c-addr u -- )
\ 	align 
\ 	latest @ here latest ! ,
\ 	0 ,
\ 	0 ,
\ 	get-current c,
\ 	0 c,
\ 	dup c, 
\ 	dup here swap allot swap cmove
\ 	align
\ 	here latest @ 2 cells + !
\ ;																	\ Impl. dep.

\ ' HEADER (HEADER) !

\ : : ( "name" -- )
\ 	parse-name
\ 	header
\ 	latest @ nt>dt latest @ cell+ ! \ Save DT on XT
\ 	latest @ nt>xt latestxt !
\ 	colon-flag hidden-flag or latest @ 3 cells + 1 chars + c!
\ 	postpone ]
\ ;																	\ Impl. dep.
\ 
\ : ; ( -- )
\  	postpone EXIT
\  	postpone [
\ 	latest @ nt>xt latestxt @ = if
\ 	 	colon-flag latest @ 3 cells + 1 chars + c! \ unhide, but I don't like how it's done
\ 	then
\ ; immediate															\ Impl. dep.

\ -----------------------------------------------------------------------------

\ Recognizers 

17 stack: FORTH-RECOGNIZER

: SET-RECOGNIZERS ( xtn ... xt1 n -- ) forth-recognizer set-stack ;
: GET-RECOGNIZERS ( -- xtn ... xt1 n ) forth-recognizer get-stack ;
: +RECOGNIZER ( xt -- ) forth-recognizer >stack ;
: -RECOGNIZER ( -- ) forth-recognizer stack> drop ;

\ Taken from theforth.net/recognizers package

: RECTYPE:		create swap rot , , , ; ( xt-int xt-comp xt-post "<spaces>name" -- )

:NONAME type -13 throw ; 
dup 
dup 
rectype: RECTYPE-NULL

\ ( addr u xt -- addr len 0 | i*x rtok -1 )
: (RECOGNIZE)	2keep rot dup rectype-null = if drop 0 else nip nip -1 then ;

: RECOGNIZE		['] (recognize) swap map-stack 0= if rectype-null then ; \ ( addr u rid -- rtok | rnull )

\ -----------------------------------------------------------------------------

\ Word recognizer based on find-name (uses current wordlists order)
\ These are implementation dependent.

:noname	dup colon? -if dup nt>dt swap then nt>xt ?dup if execute then ;
:noname
	dup colon? -if dup nt>dt postpone literal then
	dup immediate? if
		nt>xt ?dup if execute then
	else
		nt>xt ?dup if compile, then
	then
;
:noname 
	dup colon? -if dup nt>dt postpone literal ['] literal compile, then
	dup immediate? if
		nt>xt ?dup if compile, then
	else
		nt>xt ?dup if postpone literal ['] compile, compile, then
	then
;
rectype: RECTYPE-NT

: REC-FIND ( c-addr len -- xt flags rectype-nt | rectype-null )
	find-name ?dup if 
		rectype-nt 
	else 
		rectype-null 
	then 
;

\ -----------------------------------------------------------------------------

?: INCLUDE ( i*x "name" -- j*x ) parse-name included ;

?: @EXECUTE @ ?dup if execute then ;

\ Taken from SwiftForth
: INTERPRET
	begin \ ?stack
		parse-name dup while
		forth-recognizer recognize
		state @ cells + @execute
	repeat 2drop
;

\ I need some easy way to check if something is defined before defining [DEFINE]
\ [DEFINED] (INTERPRET) [IF] ' INTERPRET (INTERPRET) ! [THEN]
' INTERPRET (INTERPRET) !

\ -----------------------------------------------------------------------------

: [DEFINED]		parse-name find-name 0<> ;
: [UNDEFINED]	parse-name find-name 0= ;

\ Implementation of [IF] [ELSE] [THEN] by ruv, as seen in:
\ https://forth-standard.org/standard/tools/BracketELSE

wordlist dup constant BRACKET-FLOW-WL get-current swap set-current
: [IF]		1+ ; \ ( level1 -- level2 )
: [ELSE]	dup 1 = if 1- then ; \ ( level1 -- level2 )
: [THEN]	1- ; \ ( level1 -- level2 )
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

: [THEN]	; immediate \ ( -- )

: [IF]		0= if postpone [else] then ; immediate \ ( flag -- )

\ -----------------------------------------------------------------------------

\ Throw/Catch 

[UNDEFINED] CATCH		[IF]
VARIABLE EXCP-HANDLER 0 excp-handler ! \ last exception handler

: CATCH ( xt -- exception# | 0 ) \ return addr on stack
   sp@ >r	            ( xt )       \ save data stack pointer
   excp-handler @ >r    ( xt )       \ and previous handler
   rp@ excp-handler !   ( xt )       \ set current handler
   execute				( )          \ execute returns if no THROW
   r> excp-handler !    ( )          \ restore previous handler
   r> drop				( )          \ discard saved stack ptr
   0	                ( 0 )        \ normal completion
;

: THROW ( ??? exception# -- ??? exception# )
    ?dup if					( exc# )     \ 0 THROW is no-op
      excp-handler @ rp!	( exc# )     \ restore prev return stack
      r> excp-handler !		( exc# )     \ restore prev handler
      r> swap >r		    ( saved-sp ) \ exc# on return stack
      sp! drop r>			( exc# )     \ restore stack
      \ Return to the caller of CATCH because return
      \ stack is restored to the state that existed
      \ when CATCH began execution
    then
;

: ABORT -1 throw ;

create ABORT-MESSAGE 2 cells allot

: (ABORT") abort-message cell+ ! abort-message ! -2 throw ;
: ABORT" postpone s" postpone rot postpone if postpone (abort") postpone else postpone 2drop postpone then ; immediate

[THEN]












\ Conditional combinators 

?: ?		rot if drop else swap drop then ; \ ( flag n1 n2 -- n1 | n2 )
?: IFTE		? execute ; \ ( flag xt1 xt2 -- )
?: WHEN		swap if execute else drop then ; \ ( flag xt -- )
?: UNLESS	swap if drop else execute then ; \ ( flag xt -- )

\ Additional stack shuffling operator that could not be defined until if/then definition

?: ?*		rot ?dup if -rot drop else swap drop then ; \ ( flag n1 n2 -- flag n1 | n2 )
?: IFTE*	?* execute ; \ ( n1 xt1 xt2 -- )
?: WHEN*	>r ?dup r> when ; \ ( n1 xt -- )
?: UNLESS*	over if drop else nip execute then ; \ ( n1 xt -- )

\ Looping and iterator combinators 

\ -----------------------------------------------------------------------------

?: LSHIFT	begin dup while >r 2* r> 1- repeat drop ;
?: ABS		dup 0< if negate then ;

?: +-		0< if negate then ;

?: U<		2dup xor 0< if nip 0< else - 0< then ; 
?: U+D		dup rot + dup rot u< negate ;
?: D+		>r rot u+d rot + r> + ;
?: D+-		0< if invert swap invert 1 u+d rot + then ;
\ [UNDEFINED] UM* [IF] 
\ : UM* 
\ 	1 2>r 0 0 rot 0 
\ 	begin 
\ 		r@ while
\ 		2r> 2dup 2* 2>r and if 2swap 2over d+ 2swap then 2dup d+
\     repeat 2drop 2r> 2drop ;
\ [THEN]
?: M*		2dup xor >r abs swap abs um* r> d+- ;
?: DNEGATE	invert swap invert 1 u+d rot + ;
?: DABS		dup 0< if dnegate then ;

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

[UNDEFINED] SM/REM [IF]
: SGN \ ( u1 n1 -- n2 ) \ n2 is u1 with the sign of n1
    0< if negate then
;

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
\ : SM/REM 2dup xor >r >r dabs r> abs um/mod r> +- ; 
[THEN]

\ ?: HEADER ( c-addr u -- )
\ 	align 
\ 	latest @ here latest ! ,
\ 	0 ,
\ 	0 ,
\ 	get-current c,
\ 	0 c,
\ 	dup c, 
\ 	dup here swap allot swap cmove
\ 	align
\ 	here latest @ 2 cells + !
\ ; \ Impl. dep.
\ ' HEADER (HEADER) !

\ : CREATE parse-name header ;
\ ' CREATE (CREATE) !

\ TODO to define FALSE and TRUE it's necessary to check if they're already defined

?: SP0		0 ;			\ Impl. dep.
?: DEPTH	sp@ sp0 - ; \ Impl. dep.

\ ------------- Words ------------

\ There are four types of words:
\ non-colon words: words that have no colon and no does> like variable
\ does> words: words that have no colon and have does> like constant
\ colon words: words defined with : like dup
\ immediate words: words defined with : ; immediate like if

\ ?: NAME>INTERPRET	dup colon? -if dup nt>dt swap then nt>xt ?dup -if ['] noop then ; \ Impl. dep.
\ 
\ ?: NAME>COMPILE		
\ 	dup colon? if
\ 		dup nt>xt swap immediate? if
\ 			['] execute
\ 		else
\ 			['] compile,
\ 		then
\ 	else
\ 		dup nt>dt swap nt>xt dup if
\ 			swap [: postpone literal compile, ;]
\ 		else
\ 			drop ['] literal
\ 		then
\ 	then
\ ;	
\ ?: NAME>POSTPONE
\ 	dup colon? if
\ 		dup nt>xt swap immediate? if
\ 			['] compile,
\ 		else
\ 			[: postpone literal postpone compile, ;]
\ 		then
\ 	else
\ 		dup nt>dt swap nt>xt dup if
\ 			swap [: postpone literal postpone literal postpone compile, ;]
\ 		else
\ 			drop [: postpone literal ;]
\ 		then
\ 	then
\ ;
\ 
\ variable test1 11 test1 !
\ 13 constant test2
\ : test3 17 ;
\ : test4 19 ; immediate

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
   R> R> SWAP >R dup
   BEGIN
      dup
   WHILE
      R> R> SWAP >R -ROT
      1-
   repeat
   DROP
;
[THEN]

\ Additional stacks are used for locals, wordlists and recognizers. Let's add
\ some code taken from theforth.net to work with them.

\ TODO Add some error management to the stacks ?!

\ TODO: N>STACK NSTACK> 

\ Locals allow to simplify definitions a lot, let's define very basic locals handling now.

64 stack: LOCALS

\ TODO: This implementation does not allow use of LOCALS stack from outside
\ a definition.

\ TODO: Right now, I only use this locals inside ACCEPT, is it worth it?

?: L1 0 locals @stack ;
?: L2 1 locals @stack ;
?: L3 2 locals @stack ;
?: L4 3 locals @stack ;

?: 1L> locals stack> drop ;
?: 2L> 1L> 1L> ;
?: 3L> 2L> 1L> ;
?: 4L> 3L> 1L> ;

?: 1>L locals >stack r> ['] 1L> >r >r ;
?: 2>L swap locals >stack locals >stack r> ['] 2L> >r >r ;
?: 3>L rot locals >stack swap locals >stack locals >stack r> ['] 3L> >r >r ;
?: 4>L >r rot locals >stack swap locals >stack locals >stack r> locals >stack r> ['] 4L> >r >r ;

\ This is an implementation of transient region memory. Its similar to a circular
\ buffer, maintaining things in memory until it gets written again. Its not a
\ full garbage collector but has no timing problems and its enough for working
\ with transient structures.

\ Impl. dep.
[UNDEFINED] SLITERAL [IF]
: SLITERAL \ ( C: c-addr1 u -- ) ( -- c-addr2 u )
	postpone doSTRING
	dup , >r
	begin
		r@ while	
		dup c@ c, 1 +
		r> 1 - >r
	repeat
	align
	drop r> drop
; immediate
[THEN]

[UNDEFINED] S" [IF] 
: S" 
 	34 parse state @ if 
 		postpone sliteral 
 	else
 		dup tallot swap 
 		2dup 2>r cmove 2r>
 	then 
 ; immediate 
 [THEN]
 
[UNDEFINED] COUNT [IF] : COUNT ( c-addr1 -- c-addr2 u ) dup char+ swap c@ ; [THEN]
 
\ [UNDEFINED] TYPE [IF] : TYPE ( c-addr u -- ) begin dup 0> while swap count emit swap 1- repeat 2drop ; [THEN]
 
[UNDEFINED] .( [IF] : .( ( "ccc<paren>" -- ) 41 parse type ; immediate [THEN]

[UNDEFINED] ." [IF] 
: ." 
	34 parse state @ if 
		postpone sliteral postpone type 
	else 
		type 
	then 
; immediate 
[THEN]

\ [UNDEFINED] PICK [IF] : PICK dup if swap >r 1 - recurse r> swap exit then drop dup ; [THEN]
\ [UNDEFINED] ROLL [IF] : ROLL dup if swap >r 1 - recurse r> swap exit then drop ; [THEN]
 
\ \ See [https://dxforth.mirrors.minimaltype.com/cfsext.html] for an implementation
\ \ of more advanced control-flow stack extensions (CS-MARK CS-TEST ....)
\ [UNDEFINED] CS-PICK [IF] : CS-PICK pick ; [THEN]
\ [UNDEFINED] CS-ROLL [IF] : CS-ROLL roll ; [THEN]


\ [UNDEFINED] R@ [IF] : R@ r> dup >r ; [THEN]
 
[UNDEFINED] RP0 [IF] : RP0 0 ; [THEN] \ Impl. dep.
[UNDEFINED] RDEPTH [IF] : RDEPTH rp@ rp0 - ; [THEN] \ Impl. dep.
 
\ [UNDEFINED] 2/ [IF] : 2/ 2 / ; [THEN]
\ [UNDEFINED] 2/ [IF] : 2/ 1 rshift ; [THEN]
\ U< U+D D+ D+- UM* and M* taken from lbForth
[UNDEFINED] HEX [IF] : HEX 16 base ! ; [THEN]
[UNDEFINED] DECIMAL [IF] : DECIMAL 10 base ! ; [THEN]

DECIMAL

\ ------ Double variables and constants --------

\ ------------- Transient memory ---------------

\ This implementation of transient memory allows
\ a simple way of using the memory above HERE to
\ store strings, input buffers and whatever 
\ without the need to define buffers for them,
\ acting like a very basic Garbage Collector.

\ TODO: There should be some way to put the limit before unused,
\ in the case some memory at the end was to be reserved.

\ Output

\ ----------------- Finding names ---------------------

\ [UNDEFINED] COMPARE [IF]
\ : SAME? ( c-addr1 c-addr2 u -- -1|0|1 )
\     bounds ?do
\         i c@ over c@ - ?dup if
\             0> 2* 1+
\             nip unloop exit
\         then
\         1+
\     loop
\     drop 0
\ ;
\ 
\ : COMPARE
\     rot 2dup swap - >r          \ ca1 ca2 u2 u1  r: u1-u2
\     min same? ?dup
\     if r> drop exit then
\     r> dup if 0< 2* 1+ then 
\ ;
\ [THEN]


?: FIND		dup count find-name dup if nip dup nt>xt swap immediate? if 1 else -1 then then ;

?: XT>NT	>r	latest @ begin dup nt>xt r@ <> while @ repeat r> drop ;
?: >BODY	xt>nt nt>dt ;

?: VALUE	create , does> @ ;
?: DEFER	create 0 , does> @ execute ;
?: TO		state @ if postpone ['] postpone >body postpone ! else ' >body ! then ; immediate
?: IS		state @ if postpone to else ['] to execute then ; immediate 

\ Input

\ Accept is the only word where I need to use locals...
\ .,.also, being such many lines I can not use ?:
[UNDEFINED] ACCEPT [IF]
: ACCEPT 0 0 4>l \ l4 = c-addr l3 = u l2 = 0 l1 = 0
	begin
		l3 @ 0> while
		key l2 !
		l2 @ 10 <> while
		l2 @ 13 <> while
		l2 @ 127 = if
			l1 @ 0 > if
				l1 @ 1 - l1 !
				l3 @ 1 + l3 !
				l4 @ 1 - l4 !
				8 emit 32 emit 8 emit
			then
		else
			l2 @ dup emit l4 @ c!
			l1 @ 1 + l1 !
			l4 @ 1 + l4 !
			l3 @ 1 - l3 !
		then
	repeat then then
	l1 @
;
[DEFINED] (ACCEPT) [IF] ' ACCEPT (ACCEPT) ! [THEN]
[THEN]

?: RDROP	postpone r> postpone drop ;		( R: n -- )


\ ------------- Doubles ------- (Taken mostly from SwapForth)

: D+		swap >r + swap r@ + swap over r> u< - ;
: DNEGATE	invert swap negate tuck 0= - ;

\ --------- Numeric output ---------------

: DABS		dup 0 < if dnegate then ;		( d1 -- d2 )
: UD/MOD	>r 0 r@ um/mod r> swap >r um/mod r> ;

VARIABLE hld
create <HOLD 100 chars dup allot <hold + CONSTANT HOLD>

: <#		hold> hld ! ;													( -- )
: HOLD		hld @ 1 - dup hld ! c! ;										( c -- )
: #			base @ ud/mod rot 9 over < if 7 + then 48 + hold ;				( d1 -- d2 )
: #S		begin # over over or 0= until ;									( d1 -- d2 )
: #>		drop drop hld @ hold> over - ;									( d -- c-addr len )

: SIGN		0 < if 45 hold then ;

: UD.R		>r <# #s #> r> over - spaces type ;								( d n -- )
: UD.		0 ud.r space ;
: U.R		0 swap ud.r ;
: U.		0 ud. ;
: D.R		>r swap over dabs <# #s rot sign #> r> over - spaces type ;		( d n -- )
: D.		0 d.r space ;
: .R		>r dup 0 < r> d.r ;
: .			dup 0 <  d. ;

: ?			@ . ; ( addr -- )

\ --------------- Dump stack ------------

: (.S)		depth if >r recurse r> dup . then ;
: .S		[char] < emit depth 0 .r [char] > emit space (.s) ;	

\ ----------------------------------------

\ ASCII taken from pForth
: ASCII ( <char> -- char , state smart )
        bl parse drop c@
        state @
        \ IF [compile] literal \ This was changed as I don't have [compile] in SLOTH
		IF postpone literal
        THEN
; immediate

\ DIGIT Taken from pFORTH
\ Convert a single character to a number in the given base.
: DIGIT   ( char base -- n true | char false )
    >r
\ convert lower to upper
    dup ascii a < not
    IF
        ascii a - ascii A +
    THEN
\
    dup dup ascii A 1- >
    IF ascii A - ascii 9 + 1+
    ELSE ( char char )
        dup ascii 9 >
        IF
            ( between 9 and A is bad )
            drop 0 ( trigger error below )
        THEN
    THEN
    ascii 0 -
    dup r> <
    IF dup 1+ 0>
        IF nip true
        ELSE drop FALSE
        THEN
    ELSE drop FALSE
    THEN
;
\ >NUMBER taken from pForth
: >NUMBER ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 , convert till bad char , CORE )
    >r
    BEGIN
        r@ 0>    \ any characters left?
        IF
            dup c@ base @
            digit ( ud1 c-addr , n true | char false )
            IF
                TRUE
            ELSE
                drop FALSE
            THEN
        ELSE
            false
        THEN
    WHILE ( -- ud1 c-addr n  )
        swap >r  ( -- ud1lo ud1hi n  )
        swap  base @ ( -- ud1lo n ud1hi base  )
        um* drop ( -- ud1lo n ud1hi*baselo  )
        rot  base @ ( -- n ud1hi*baselo ud1lo base )
        um* ( -- n ud1hi*baselo ud1lo*basello ud1lo*baselhi )
        d+  ( -- ud2 )
        r> 1+     \ increment char*
        r> 1- >r  \ decrement count
    repeat
    r>
;

[UNDEFINED] TRACE [IF]
: TRACE ( -- )
	." [" state @ . ." ] " .s
	." <" /source type ." >" cr
;
[THEN]

[UNDEFINED] S>D [IF] : S>D dup 0< ; [THEN]
[UNDEFINED] D>S [IF] : D>S drop ; [THEN]

\ : TIMES-WHILE ( n xt -- ) swap begin dup while over 2dip rot while 1- repeat then 2drop ;
\ : TRAVERSE-ORDER ( xt -- ) 
\ 	order dup cell- @ rot swap [: over @ -rot dup 2dip swap cell+ swap rot ;] times-while 2drop ;
\ : FIND-NAME ( c-addr u -- ) [: -rot 2dup 2>r rot find-name-in dup if 2r> false else 2r> true then ;] traverse-order 2drop ;
\ 
\ : FIND2 ( c-addr u -- ) [: [: find-name-in ;] 3keep drop rot dup 0= ;] traverse-order nip nip ;

\ : FIND2 ( c-addr u -- )
\ 	0 get-order 0 ?do
\ 	loop
\ ;

\ : FIND-TEST 2 1 2 set-order parse-name find2 ;

: hex2. ( u -- )
    base @ swap
    hex
    s>d <# # # #> type space
    base !
;

: dump
    ?dup
    if
        base @ >r hex
        1- 4 rshift 1+
        0 do
            cr dup dup [ 2 cells ] literal u.r space space
            16 0 do
                dup c@ hex2. 1+
            loop
            space swap
            16 0 do
                dup c@
                dup bl 127 within invert
                if drop [char] . then
                emit 1+
            loop
            drop
        loop
        r> base !
    then
    drop
;

\ ------------------------------------ Double numbers

[UNDEFINED] D0= [IF] : D0= 0= swap 0= and ; [THEN]
[UNDEFINED] D< [IF] : D< rot 2dup = if 2drop u< else > nip nip then ; [THEN]

\ \ ------------------------------------ Floating point
\ 
\ [UNDEFINED] S>F [IF] : S>F s>d d>f ; [THEN]
\ [UNDEFINED] F>S [IF] : F>S f>d d>s ; [THEN]
\ 
\ [UNDEFINED] FMAX [IF] : FMAX fover fover f< if fswap then fdrop ; [THEN]
\ [UNDEFINED] FMIN [IF] : FMIN fover fover f< invert if fswap then fdrop ; [THEN]
\ 
\ [UNDEFINED] FNEGATE [IF] : FNEGATE 0e fswap f- ; [THEN]
\ 
\ [UNDEFINED] FLOAT+ [IF] : FLOAT+ 1 floats + ; [THEN]
\ [UNDEFINED] FALIGNED [IF] : FALIGNED 1 floats 1- + 1 floats / 1 floats * ; [THEN]
\ [UNDEFINED] FALIGN [IF] : FALIGN here dup faligned swap - allot ; [THEN]
\ 
\ [UNDEFINED] F, [IF] : F, here f! 1 floats allot ; [THEN]
\ [UNDEFINED] FCONSTANT [IF] : FCONSTANT create f, ; [THEN]
\ [UNDEFINED] FVARIABLE [IF] : FVARIABLE create 0e f, does> f@ ; [THEN]
\ 
\ \ Impl. dep.
\ [UNDEFINED] FLITERAL [IF] : FLITERAL postpone doFLIT f, ; immediate [THEN]


\ ==== COMBINATORS ====

\ Taken from Factor and RetroForth

\ Dataflow combinators

: BI ( n xt1 xt2 -- ) [: keep ;] dip execute ;
: 2BI ( n1 n2 xt1 xt2 -- ) [: 2keep ;] dip execute ;
: 3BI ( n1 n2 n3 xt1 xt2 -- ) [: 3keep ;] dip execute ;

: TRI ( n xt1 xt2 xt3 -- ) [: [: keep ;] dip keep ;] dip execute ;
: 2TRI ( n1 n2 xt1 xt2 xt3 -- ) [: [: 2keep ;] dip 2keep ;] dip execute ;
: 3TRI ( n1 n2 n3 xt1 xt2 xt3 -- ) [: [: 3keep ;] dip 3keep ;] dip execute ;

\ In Factor, there is CLEAVE, 2CLEAVE, 3CLEAVE and 4CLEAVE, but they need sequences
\ of quotations. I don't have that right now. Leave for later.

: BI* ( n1 n2 xt1 xt2 -- ) [: dip ;] dip execute ;
: 2BI* ( n1 n2 n3 n4 xt1 xt2 -- ) [: 2dip ;] dip execute ;

: TRI* ( n1 n2 n3 xt1 xt2 xt3 -- ) [: [: 2dip ;] dip dip ;] dip execute ;
: 2TRI* ( n1 n2 n3 n4 n5 n6 xt1 xt2 xt3 -- ) [: 4dip ;] 2dip 2bi* ;

\ In Factor, there is SPREAD, that takes n values and n quotations and acts as bi or tri.

: BI@ ( n1 n2 xt -- ) dup bi* ;
: 2BI@ ( n1 n2 n3 n4 xt -- ) dup 2bi* ;

: TRI@ ( n1 n2 n3 xt -- ) dup dup tri* ;
: 2TRI@ ( n1 n2 n3 n4 n5 n6 xt -- ) dup dup 2tri* ;

: BOTH? ( n1 n2 xt -- flag ) bi@ and ;
: EITHER? ( n1 n2 xt -- flag ) bi@ or ;

\ Looping combinators

\ The problem with some Joy/Factor combinators is that their names are taken by Forth.

: *LOOP ( xt -- ) >r begin r@ execute while repeat r> drop ;

: *WHILE ( xt1 xt2 -- ) begin over 2dip rot while dup 2dip repeat 2drop ;
: *UNTIL ( xt1 xt2 -- ) begin over 2dip rot 0<> while dup 2dip repeat 2drop ;

\ : ITER ( addr u xt -- ) -rot [: dup @ -rot over 2dip cell+ ;] times 2drop ;		
: -ITER ( addr u xt -- ) -rot swap over 1- cells + swap [: dup @ -rot over 2dip cell- ;] times 2drop ;

\ \ : CITER ( c-addr u xt -- ) 3>i begin i2 @ while i3 @ c@ i1 @ execute i3 1+! i2 1-! repeat ;
\ \ : CITER/ADDR ( c-addr u xt -- ) 3>i begin i2 @ while i3 @ i1 @ execute i3 1+! i2 1-! repeat ;
\ \ : CITER2 ( c-addr1 c-addr2 u xt -- ) 4>i begin i2 @ while i3 @ i4 @ i1 @ execute i3 1+! i4 1+! i2 1-! repeat ;
\ \ 
\ \ 
\ \ : FILL ( c-addr u char -- ) -rot [: 2dup c! drop ;] citer/addr drop ;
\ \ : ERASE ( c-addr u -- ) 0 fill ;
\ \ 
\ \ : CMOVE ( c-addr1 c-addr2 u -- ) [: c@ swap c! ;] citer2 ;
\ \ 
\ \ : UPPER ( u1 -- u2 ) dup dup 97 >= swap 122 <= and if 32 - then ;
\ \ 
\ \ : >UPPER ( c-addr u -- ) [: dup c@ upper swap c! ;] citer ;
\ \ : >UPPER/T ( c-addr1 u -- c-addr2 u ) dup tallot swap 2dup 2>r [: c@ upper swap c! ;] citer2 2r> ;
\ 
\ \ TODO: I need to different local stacks, one to not share and one to share with quotations.
\ 
\ \ Iterate over chars and execute q for each char.
\ \ : CITER ( c-addr u q -- ) 3>l begin l2 @ while l3 @ l1 @ execute l3 1+! l2 1-! repeat ;
\ 
\ : CMAP ( c-addr1 c-addr2 u q -- ) 
\ 	4>l 
\ 	begin 
\ 		l2 @ while 
\ 		l4 @ c@ 
\ 		l1 @ execute 
\ 		l3 @ c! 
\ 		l4 1+! l3 1+! l2 1-!
\ 	repeat 
\ ;
\ 
\ : CMAP>T ( c-addr u q | t-c-addr -- t-c-addr u )
\ 	over tallot 4>l
\ 	l4 @ l1 @ l3 @ l2 @ cmap
\ 	l1 @ l3 @
\ ;
\ 
\ : new-cmap ( c-addr u q -- ) 3>l begin l2 @ while l3 @ c@ l1 @ execute l3 1+! l2 1-! repeat ;
\ : new-cmove ( c-addr1 c-addr2 u -- ) swap -rot [: over c! 1+ ;] new-cmap ;
\ 

\ Compilation is done to current wordlist, that means that header must use that

\ -----------------------------------------------------------------------------

\
\ Purpose: Number recognizers in Forth CORE
\
\ Author: Matthias Trute
\ Date: Sep 14, 2017
\ License: Public Domain
\
\ not really smart but portable code, tested with old gforth's and vfxlin's
\

decimal

' noop 
:NONAME		postpone literal ; 
dup  
rectype: RECTYPE-NUM 

' noop 
:NONAME		postpone 2literal ; 
dup 
rectype: RECTYPE-DNUM
 
\
\ check for the 'c' syntax for single
\ characters.
\
: rec-char ( addr len -- n rectype-num | rectype-null )
  3 = if                       \ a three character string
    dup c@ [char] ' = if       \ that starts with a ' (tick)
      dup 2 + c@ [char] ' = if \ and ends with a ' (tick)
        1+ c@ rectype-num exit
      then
    then
  then
  drop rectype-null
;


?: STR= ( c-addr1 u c-addr2 u -- ) rot over = if bounds ?do dup c@ i c@ <> if drop unloop 0 exit then char+ loop drop -1 else drop drop drop 0 then ;

\
\ helper words for number recognizers
\
\ set BASE according the the character at addr
\ or the 0x prefix string for HEX numbers.
\ returned string is stripped of the prefix
\ character(s) if found.
\ #: 10, $: 16, %: 2, &: 10 (again)
create num-bases 10 , 16 , 2 , 10 ,
: set-base ( addr len -- addr' len' )
  over c@ [CHAR] # - dup 0 4 within if
    cells num-bases + @ base ! 1 /string
  else
    drop
    \ check for a 0x prefix, requires the STRING wordset
    dup 2 >= if
       over 2 s" 0x" str= if \ compare 0= if \\ Changed to use str=, its simpler to implement
         2 /string hex
       then
    then
  then
;

\ check for a character. return string is
\ without it if found. f is true if found.
: skip-char? ( addr len c -- addr' len' f)
  >r over c@ r> = dup >r
  if 1 /string then r>
;

: -sign? ( addr len -- addr' len' f )
   [char] - skip-char?
;

: +sign  ( addr len -- addr' len' )
   [char] + skip-char? drop
;

\ allows $- and -$ combinations. skip +
\ f is true is - sign is found (and stripped off)
: base-and-sign? ( addr len -- addr' len' f)
  set-base +sign -sign? >r set-base r>
;

\ a factor the recognizers below.
: (rec-number) ( addr len -- d addr' len' f )
   base @ >r           \ save BASE
   base-and-sign? >r   \ handle prefix and sign characters
   2>r 0 0 2r> >number \ do the actual conversion
   r> r> base !        \ restore BASE and sign information
;

\ TODO !!! This two recognizers are not working !!!

\
\ check for double number input
\
: rec-dnum ( addr len -- d rectype-dnum | rectype-null )
    \ simple syntax check: last character in addr/len is a dot . ?
    2dup + 1- c@ [char] . = if
      1-              \ strip trailing dot
      (rec-number) >r \ do the dirty work
      \ a number and only a number?
      nip if
        2drop r> drop rectype-null
      else
        r> if dnegate then rectype-dnum
      then
    else
      2drop rectype-null  \ no, it cannot be a double cell number.
    then
;

\
\ check for single cell number input
\
: rec-snum ( addr len -- n rectype-num | rectype-null )
    (rec-number) >r 
    nip if
      2drop r> drop rectype-null
    else
      \ a "d" with a non-empty upper cell cannot be an "n"
      if    r> drop rectype-null
      else  r> if negate then rectype-num
      then
    then
;

\ combine them
\ 3 STACK VALUE recstack-numbers
3 stack: recstack-numbers

' rec-char ' rec-dnum ' rec-snum 3 recstack-numbers SET-STACK

: REC-NUM ( addr len -- n RECTYPE-NUM | d RECTYPE-DNUM | RECTYPE-NULL )
  recstack-numbers RECOGNIZE
;

\
\ Purpose: Use two " characters as the delimiters 
\          of a string.
\
\ Author: Matthias Trute
\ Date:   Sep 14, 2017
\ License: Public Domain
\
\ This is an example for a recognizer that depends
\ on >IN (and thus SOURCE) for input. So, no test
\ cases.
\

' noop 
:noname postpone sliteral ;
\ TODO This code does not accept to postpone a string, but it should
\ compile the code to compile the string !!
:noname -48 throw ; rectype: rectype-string

: rec-string ( addr len -- addr' len' rectype-string | rectype-null )
	over c@ [char] " <> if 2drop rectype-null exit then
	drop source drop - 1+ >in !
	[char] " parse
	rectype-string
;

' rec-num ' rec-string ' rec-find 3 forth-recognizer set-stack

\ TODO Work with recognizers

\ \ Taken from SwiftForth
\ ?: INTERPRET
\ 	begin \ ?stack
\ 		parse-name dup while
\ 		forth-recognizer recognize
\ 		state @ cells + @execute
\ 	repeat 2drop
\ ;
\ 
\ [DEFINED] (INTERPRET) [IF] ' INTERPRET (INTERPRET) ! [THEN]

: POSTPONE parse-name forth-recognizer recognize cell+ cell+ @ execute ; immediate

\ -----------------------------------------------------------------------------


[DEFINED] SOURCE-ID! [IF]
\ QUIT can not be implemented portably in ANS Forth because:
\ 1 - There is no portable way to set user input device as input source.
\ 2 - There is no portable way to empty the return stack
: QUIT 
	0 rp!
	0 SOURCE-ID!
	postpone [
	begin
		refill
	while
		space
		['] interpret catch
		?dup 0= if
			space .s ." ok" cr
		else
			dup -13 = if ."  ?" cr 0 sp! 
			else dup -2 = if 
					abort-message @ abort-message cell+ @ type space ." ?!" cr 0 sp!
				else
					." unknown error: " . cr
					0 sp!
				then
			then
		then
	repeat
;

." SLOTH (SLOw forTH) v1.0.0" cr
quit
[THEN]


