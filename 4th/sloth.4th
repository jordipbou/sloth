: ?: parse-name find-name [: source >in ! drop ;] [: 1 >in ! ;] choose ;

?: \	source >in ! drop ; immediate

\ TODO Try to add parse earlier to allow stack comments
\ TODO Add more recognizers (string, double numbers...)
\ TODO Add numeric output
\ TODO Add tools (dump trace debug)

?: [		0 state ! ; immediate
?: ]		1 state ! ; immediate	\ This will not return to -1 if needed !!!

\ HOT (here-or-there) allows compilation to contiguous or transient
\ memory depending of value of state, allowing use of quotations and
\ control structures in interpretation state.

?: HOT		state @ 0 < [: there ;] [: here ;] choose ;

\ Control structures ---

?: AHEAD	postpone branch hot 0 , ; immediate
?: IF		postpone ?branch hot 0 , ; immediate
?: THEN		hot over - swap ! ; immediate

?: BEGIN	hot ; immediate
?: UNTIL	postpone ?branch hot - , ; immediate
?: AGAIN	postpone branch hot - , ; immediate

?: ELSE		postpone ahead swap postpone then ; immediate

?: WHILE	postpone if swap ; immediate
?: REPEAT	postpone again postpone then ; immediate

?: LITERAL	postpone lit , ; immediate 
?: 2LITERAL swap postpone literal postpone literal ; 

?: [']		' postpone literal ; immediate 

\ Stack shuffling ---

?: DUP		0 pick ;				\ ( x -- x x )
?: OVER		1 pick ;				\ ( x1 x2 -- x1 x2 x1 )
?: NIP		swap drop ;				\ ( x1 x2 -- x2 )
?: TUCK		swap over ;				\ ( x1 x2 -- x2 x1 x2 )
?: ROT		>r swap r> swap ;		\ ( x1 x2 x3 -- x2 x3 x1 )
?: -ROT		rot rot ;				\ ( x1 x2 x3 -- x3 x1 x2 ) Non ANS
?: ?DUP		dup if dup then ; \ ( x -- 0 | x x )

?: 2DUP		over over ;				\ ( x1 x2 -- x1 x2 x1 x2 )
?: 2DROP	drop drop ;				\ ( x1 x2 -- )
?: 2OVER	3 pick 3 pick ;			\ ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 )
?: 2SWAP	rot >r rot r> ; 		\ ( x1 x2 x3 x4 -- x3 x4 x1 x2 )

?: 3DROP	drop drop drop ;		\ ( x1 x2 x3 -- )

?: R@		r> r> dup >r swap >r ;	\ ( -- x ) ( R: x -- x )

\ ( x1 x2 -- ) ( R: -- x1 x2 )
?: 2>R		postpone swap postpone >r postpone >r ; immediate
\ ( -- x1 x2 ) ( R: x1 x2 -- )
?: 2R>		postpone r> postpone r> postpone swap ; immediate
\ ( -- x1 x2 ) ( R: x1 x2 -- x1 x2 )
?: 2R@		postpone r> postpone r> postpone 2dup postpone 2>r ; immediate
   
?: 2ROT		2>r 2swap 2r> 2swap ; \ ( x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2 )

\ Stack shuffling combinators ---

?: DIP		swap >r execute r> ; \ ( i*x n xt -- j*x n )
?: 2DIP		-rot 2>r execute 2r> ; \ ( n1 n2 xt -- n1 n2 )
?: 3DIP		swap >r swap >r swap >r execute r> r> r> ; \ ( n1 n2 n3 xt -- n1 n2 n3 )
?: 4DIP		swap >r swap >r swap >r swap >r execute r> r> r> r> ; \ ( n1 n2 n3 n4 xt -- n1 n2 n3 n4 )
?: KEEP		over >r execute r> ; \ ( n xt[ n -- i*x ] -- i*x n )
?: 2KEEP	2 pick 2 pick >r >r execute r> r> ; \ ( n1 n2 xt -- n1 n2 )
?: 3KEEP	3 pick 3 pick 3 pick >r >r >r execute r> r> r> ; \ ( n1 n2 n3 xt -- n1 n2 n3 )
 
?: KEEP-XT	>r r@ execute r> ; \ ( i*x xt -- j*x xt )

\ Arithmetic

?: +		0 swap - - ;			\ ( x1 x2 -- x3 )
?: *		1 */mod swap drop ;		\ ( x1 x2 -- x3 )
?: /		1 swap */mod swap drop ;	\ ( x1 x2 -- x3 )
?: */		*/mod swap drop ;		\ ( x1 x2 -- x3 )
?: MOD		1 swap */mod drop ;		\ ( x1 x2 -- x3 )
?: /MOD		1 swap */mod ;			\ ( x1 x2 -- x3 )
?: 2*		dup + ;					\ ( x1 x2 -- x3 )

?: 1-		1 - ;					\ ( x1 -- x2 )
?: 1+		1 + ;					\ ( x1 -- x2 )
?: 2+		2 + ;					\ ( x1 -- x2 )

?: NEGATE	invert 1+ ;				\ ( x1 -- x2 )

\ Bit

?: OR		invert swap invert and invert ;	\ ( x1 x2 -- x3 )
?: XOR		over over invert and >r swap invert and r> or ; \ ( x1 x2 -- x3 )

\ Comparison

?: <		2dup xor 0< if drop 0< else - 0< then ; \ ( n1 n2 -- flag )
?: >		swap < ;								\ ( n1 n2 -- flag )
?: <=		> invert ;								\ ( n1 n2 -- flag )
?: >=		< invert ;					\ ( n1 n2 -- flag )
?: 0=		if 0 else 0 invert then ;	\ ( x -- flag )
?: 0>		0 > ;						\ ( n -- flag )
?: 0<>		0= 0= ;						\ ( n -- flag )
?: NOT		0= ;						\ ( x -- flag ) Non ANS
?: =		- 0= ;						\ ( x1 x2 -- flag )
?: <>		= invert ;					\ ( x1 x2 -- flag )
?: U<		2dup xor 0< if swap drop 0< else - 0< then ; \ ( u1 u2 -- flag )
?: U>		swap u< ;					\ ( u1 u2 -- flag )
?: WITHIN	over - >r - r> u< ; 
   
?: MIN		2dup swap < if swap then drop ;
?: MAX		2dup	  < if swap then drop ;

\ -if was not defined until 0= was defined
?: -IF		postpone 0= postpone if ; immediate

\ Memory

?: CHAR		1 chars ;				\ ( -- u )
?: CELL		1 cells ;				\ ( -- u )
?: CHAR+	1 chars + ;				\ ( c-addr1 -- c-addr2 )
?: CELL+	1 cells + ; 			\ ( a-addr1 -- a-addr2 )
?: CELL-	1 cells - ; 			\ ( a-addr1 -- a-addr2 )

?: ,		here ! 1 cells allot ;	\ ( x -- )
?: C,		here c! 1 chars allot ; \ ( char -- )
 
?: +!		swap over @ + swap ! ;	\ ( n | u a-addr -- )
?: 0!		0 swap ! ;				\ ( a-addr -- ) non ANS
?: 1+!		dup @ 1+ swap ! ;		\ ( a-addr -- ) non ANS
?: 1-!		dup @ 1- swap ! ;		\ ( a-addr -- ) non ANS

?: 2!		swap over ! cell+ ! ;	\ ( x1 x2 a-addr -- )
?: 2@		dup cell+ @ swap @ ;	\ ( a-addr -- x1 x2 )

\ Loops ---

?: DO		postpone [: ; immediate
?: ?DO		postpone [: ; immediate
?: LOOP		postpone lit 1 , postpone ;] postpone doloop ; immediate
?: +LOOP	postpone ;] postpone doloop ; immediate

?: UNLOOP	; \ Unloop is a noop in this implementation

?: TIMES	swap 0 [: keep-xt 1 ;] doloop drop ; \ ( n q -- )

?: BOUNDS		over + swap ; \ ( addr u -- u u )
?: ITER			-rot cells bounds [: >r i @ r@ execute r> cell ;] doloop drop ; \ ( addr u xt -- )
?: ITER/ADDR	swap [: dup >r keep cell+ r> ;] times 2drop ; \ ( addr u xt -- )
?: -ITER/ADDR	over 2>r 1- cells + 2r> [: dup >r keep cell- r> ;] times 2drop ; \ ( addr u xt -- )
?: *ITER		-rot bounds [: i @ keep execute cell ;] doloop ; \ ( addr u xt -- )
?: CITER		-rot chars bounds [: >r i c@ r@ execute r> char ;] doloop drop ; \ ( c-addr u xt -- )

\ Constants and variables

?: CONSTANT		create , does> @ ;		\ ( x "name" -- )
?: VARIABLE		create 0 , ;			\ ( "name" -- )

?: 2CONSTANT	create , , does> 2@ ;	\ ( x1 x2 "name" -- )
?: 2VARIABLE	create 0 , 0 , ;		\ ( "name" -- )

: CONSTANT?	>in @ >r parse-name find-name 0= if r> >in ! constant else drop r> drop postpone \ then ;
: VARIABLE? >in @ >r parse-name find-name 0= if r> >in ! variable else r> drop postpone \ then ;

\ true / false / on / off

0				constant? FALSE
false invert	constant? TRUE

?: ON		true swap ! ;	\ ( a-addr -- )
?: OFF		false swap ! ;	\ ( a-addr -- )

\ -- Tools for accessing the dictionary ---------------------------------------

4 constant? IMMEDIATE-FLAG
2 constant? COLON-FLAG
1 constant? HIDDEN-FLAG

?: NT>LINK		@ ;
?: NT>XT		1 cells + @ ;
?: NT>DT		2 cells + @ ;
?: NT>WORDLIST	3 cells + @ ;
?: NT>FLAGS		4 cells + c@ ;
?: NAME>STRING	4 cells + 1 chars + dup c@ swap 1 chars + swap ;

?: HAS-FLAG?	swap nt>flags over and = ;

?: IMMEDIATE?	immediate-flag has-flag? ;
?: COLON?		colon-flag has-flag? ;
?: HIDDEN?		hidden-flag has-flag? ;

\ Wordlists

variable? LAST-WORDLIST 1 last-wordlist !

?: WORDLIST		last-wordlist @ 1+ dup last-wordlist ! ;

?: GET-CURRENT	current @ ;
?: SET-CURRENT	current ! ;

?: GET-ORDER	order @ dup cell- @ dup >r ['] noop iter r> ;
?: SET-ORDER	dup order @ cell- ! order @ swap [: ! ;] -iter/addr ;
?: +ORDER		>r get-order 1+ r> swap set-order ;
?: -ORDER		get-order nip 1- set-order ;

\ TODO Search order functions FIND-NAME-IN FIND-NAME SEARCH-WORDLIST

\ -- Input/Output -------------------------------------------------------------

?: BL			32 ; 
?: CR		10 emit ;

?: SPACE		bl emit ;

\ -- Finding definitions ------------------------------------------------------

\ ( u1 -- u2 )
?: UPPER			dup dup 97 >= swap 122 <= and if 32 - then ;

?: (COMPARE-NO-CASE) 2>r -1 swap 2r> [: upper over c@ upper <> if drop 0 swap leave then char+ ;] citer drop ;
?: COMPARE-NO-CASE	rot over = if (compare-no-case) else 3drop 0 then ;

\ ( i*x xt -- j*x ) Map xt to each word, use LEAVE to exit mapping
?: TRAVERSE-DICT	1 latest @ [: i swap keep-xt i nt>link i - ;] doloop drop ;

\ ( i*x xt wid -- j*x )
?: TRAVERSE-WORDLIST [: nt>wordlist over = if >r >r i r@ execute r> r> then ;] traverse-dict 2drop ;

\ ( c-addr u wid -- nt | 0 ) 
?: FIND-NAME-IN >r 0 -rot r> [: name>string 2over compare-no-case if rot drop i -rot leave then ;] swap traverse-wordlist 2drop ;

\ ( c-addr u wid -- 0 | xt 1 | xt -1 )
?: SEARCH-WORDLIST find-name-in dup if dup nt>xt swap immediate? if 1 else -1 then then ;

\ -- Conditional compilation --------------------------------------------------

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



\ -- Tools --------------------------------------------------------------------

?: WORDS [: name>string type space ;] get-order over >r set-order r> traverse-wordlist ;

