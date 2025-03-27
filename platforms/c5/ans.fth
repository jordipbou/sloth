REFILL For now, we can only use REFILL for comments.
DROP

CREATE (?DEFFOUND) 0 HERE ! 1 CELLS ALLOT

REFILL ?: is a simple implementation of conditional
REFILL compilation.
DROP DROP

: ?: 
	32 WORD FIND 
	SWAP DROP 
	0 = 
	0 PICK	
	1 +
	SOURCE SWAP DROP 
	OVER * SWAP 1 SWAP - + 
	>IN !
	(?DEFFOUND) !
;

REFILL ?\ allows multiline definitions with 
REFILL simple conditional compilation.
DROP DROP

: ?\ (?DEFFOUND) @ INVERT 2 + >IN ! ; IMMEDIATE

?: \ SOURCE >IN ! DROP ; IMMEDIATE

\ End of line comments can be used now.

\ SLOTH v1.0

\ TODO Explain why conditional compilation is so
\ important for SLOTH.

\ TODO Make next explanation more beautiful.

\ The first definition allows for conditional 
\ compilation on any ANS Forth.
\ Let's explain it:
\ 32 WORD FIND -- parse and find ( -- c-addr|xt 1|0|-1 )
\ SWAP DROP -- ( -- 1|0|-1)
\ 0 = 1 + -- ( -- 0|1 ) 0 if word exists, 1 if not
\ 0 PICK -- ( -- 0|1 0|1 )
\ SOURCE SWAP DROP -- ( -- 0|1 0|1 ILEN )
\ Now the formula f(x, y) = (x * y) + (1 - x) is applied,
\ where x is 0|1 (existence of a word) and y is the input
\ buffer length:
\ OVER * SWAP 1 SWAP - + -- ( -- 0|1 1|ILEN )
\ >IN ! -- ( -- 0|1 )
\ (?COMPILING) ! -- ( -- )

\ TODO Explain SLOTH

\ As per ANS Forth standard, all definitions names are
\ created and used in upper case (although I prefer the
\ upper case for definition/lower case for use, I think
\ its easier to read).

\ -- Stack comments ---------------------------------------

?: ( 41 WORD DROP ; IMMEDIATE

\ -- Compilation ------------------------------------------

?: , ( x -- ) HERE ! 1 CELLS ALLOT ;
?: C, ( char -- ) HERE C! 1 CHARS ALLOT ;

\ PLATFORM DEPENDENT
?: LITERAL ( C: x -- ) ( -- x ) 
?\		POSTPONE (LIT) , 
?\ ; IMMEDIATE

?: ' ( "<spaces>name" -- xt ) 32 WORD FIND DROP ; 

?: ['] ( "<spaces>name" -- ) ( R: -- xt ) 
?\		' POSTPONE LITERAL 
?\ ; IMMEDIATE

\ -- Variables and constants ------------------------------

?: VARIABLE ( "<spaces>name" -- ) CREATE 0 , ;
?: CONSTANT ( x "<spaces>name" -- ) CREATE , DOES> @ ;

?: BUFFER: ( u "<spaces>name" -- ; -- a-addr ) CREATE ALLOT ;

?: VALUE ( x "<spaces>name" -- ) CREATE , DOES> @ ;

\ -- Control structures -----------------------------------

\ This words have a good balance between simplicity and
\ usefulness. Some more complicated words (like DO/LOOP)
\ are not implemented now.

\ PLATFORM DEPENDENT (depends on implementation of BRANCH)
?: AHEAD ( C: -- orig ) ( -- ) 
?\		POSTPONE (BRANCH) HERE 0 , 
?\ ; IMMEDIATE

\ PLATFORM DEPENDENT (depends on implementation of ?BRANCH)
?: IF ( C: -- orig ) ( x -- ) 
?\		POSTPONE (?BRANCH) HERE 0 , 
?\ ; IMMEDIATE

?: THEN ( C: -- orig ) ( -- ) HERE OVER - SWAP ! ; IMMEDIATE

?: ELSE ( C: orig1 -- orig2 ) ( -- ) 
?\		POSTPONE AHEAD SWAP POSTPONE THEN 
?\ ; IMMEDIATE

?: BEGIN ( C: -- dest ) ( -- ) HERE ; IMMEDIATE

\ PLATFORM DEPENDENT (depends on implementation of ?BRANCH)
?: UNTIL ( C: dest -- ) ( x -- ) 
?\		POSTPONE (?BRANCH) HERE - , 
?\ ; IMMEDIATE

\ PLATFORM DEPENDENT (depends on implementation of BRANCH)
?: AGAIN ( C: dest -- ) ( -- ) 
?\		POSTPONE (BRANCH) HERE - , 
?\ ; IMMEDIATE

?: WHILE ( C: dest -- orig dest ) ( x -- ) 
?\		POSTPONE IF SWAP 
?\ ; IMMEDIATE

?: REPEAT	( C: orig dest -- ) ( -- ) 
?\		POSTPONE AGAIN POSTPONE THEN 
?\ ; IMMEDIATE

\ -- Conditional compilation of variables and constants ---

?: ?VARIABLE ( "<spaces>name" -- ) ?: ;

?: ?CONSTANT ( x "<spaces>name" -- ) 
?\		32 WORD FIND SWAP DROP
?\		IF DROP SOURCE >IN ! DROP
?\		ELSE 1 >IN !
?\		THEN
?\	;

\ ---------------------------------------------------------

\ Now that we have conditional compilation and basic 
\ control structures, we can implement a lot of fundamental 
\ words.

\ -- Stack shuffling --------------------------------------

?: DUP ( x -- x x ) 0 PICK ;
?: ?DUP	( x -- 0 | x x ) DUP IF DUP THEN ;

?: R@ ( -- x ) ( R: x -- x ) 
?\		POSTPONE R> POSTPONE DUP POSTPONE >R 
?\ ; IMMEDIATE

?: ROT ( x1 x2 x3 -- x2 x3 x1 ) >R SWAP R> SWAP ;	

\ Not ANS
?: -ROT ( x1 x2 x3 -- x3 x1 x2 ) ROT ROT ;

?: NIP ( x1 x2 -- x2 ) SWAP DROP ;
?: TUCK ( x1 x2 -- x2 x1 x2 ) >R R@ SWAP R> ;

?: ROLL ( xu xu-1 ... x0 u -- xu-1 ... x0 xu ) 
?\		DUP IF 
?\			SWAP >R 1 - RECURSE R> SWAP EXIT 
?\		THEN DROP 
?\ ;

?: 2DROP ( x1 x2 -- ) DROP DROP ;
?: 2DUP ( x1 x2 -- x1 x2 x1 x2 ) OVER OVER ;
?: 2SWAP ( x1 x2 x3 x4 -- x3 x4 x1 x2 ) >R -ROT R> -ROT ;
?: 2OVER ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 ) 3 PICK 3 PICK ;

?: 2>R ( x1 x2 -- ) ( R: -- x1 x2 ) 
?\		POSTPONE SWAP POSTPONE >R POSTPONE >R 
?\ ; IMMEDIATE

?: 2R@ ( -- x1 x2 ) ( R: x1 x2 -- x1 x2 ) 
?\		POSTPONE R> POSTPONE R> 
?\		POSTPONE SWAP POSTPONE 2DUP POSTPONE SWAP 
?\		POSTPONE >R POSTPONE >R 
?\ ; IMMEDIATE

?: 2R> ( -- x1 x2 ) ( R: x1 x2 -- ) 
?\		POSTPONE R> POSTPONE R> POSTPONE SWAP 
?\ ; IMMEDIATE

\ Not ANS
?: 2RDROP ( x1 x2 -- ) 
?\		POSTPONE R> POSTPONE R> 
?\		POSTPONE DROP POSTPONE DROP 
?\ ; IMMEDIATE

\ -- Arithmetic -------------------------------------------

?: / ( n1 n2 -- n3 ) 1 SWAP */MOD SWAP DROP ;
?: */ ( n1 n2 n3 -- n4 ) */MOD SWAP DROP ;
?: MOD ( n1 n2 -- n3 ) 1 SWAP */MOD DROP ;
?: /MOD	( n1 n2 -- n3 n4 ) 1 SWAP */MOD ;

?: 1+ ( n1 | u1 -- n2 | u2 ) 1 + ;
?: 1- ( n1 | u1 -- n2 | u2 ) 1 - ;

?: 2* ( x1 -- x2 ) 2 * ;

?: ABS ( n -- u ) DUP 0 < IF INVERT 1+ THEN ;

\ -- Bit\Logic --------------------------------------------

?: NEGATE ( n1 -- n2 ) INVERT 1+ ;

?: OR ( x1 x2 -- x3 ) INVERT SWAP INVERT AND INVERT ;
?: XOR ( x1 x2 -- x3 ) 
?\		OVER OVER INVERT AND >R SWAP INVERT AND R> OR 
?\ ;

\ -- Comparisons ------------------------------------------

?: > ( n1 n2 -- flag ) SWAP < ;
\ Not ANS
?: <= ( n1 n2 -- flag ) > INVERT ;
\ Not ANS
?: >= ( n1 n2 -- flag ) < INVERT ;
?: 0= ( x -- flag ) IF 0 ELSE 0 INVERT THEN ;
?: 0> ( x -- flag ) 0 > ;
?: 0<> ( x -- flag ) 0= 0= ;
?: 0< ( x -- flag ) 0 < ;
\ Not ANS
?: NOT ( x1 -- x2 ) 0= ;
?: <> ( x1 x2 -- flag ) = INVERT ;
?: U< ( u1 u2 -- flag ) 
?\		2DUP XOR 0< IF SWAP DROP 0< ELSE - 0< THEN 
?\ ;
?: U> ( u1 u2 -- flag ) SWAP U< ;
?: WITHIN ( n1 | u1 n2 | u2 n3 | u3 -- flag ) 
?\		OVER - >R - R> U< 
?\ ; 

\ -- More arithmetic operations ---------------------------

?: MIN ( n1 n2 -- n3 ) 2DUP > IF SWAP THEN DROP ;
?: MAX ( n1 n2 -- n3 ) 2DUP < IF SWAP THEN DROP ;

\ -- Memory -----------------------------------------------

\ Not ANS
?: CELL ( -- u ) 1 CELLS ;

?: CELL+ ( a-addr1 -- a-addr2 ) 1 CELLS + ;
?: CHAR+ ( c-addr1 -- c-addr2 ) 1 CHARS + ;
\ Not ANS
?: CELL- ( a-addr1 -- a-addr2 ) 1 CELLS - ;
\ Not ANS
?: CHAR- ( c-addr1 -- c-addr2 ) 1 CHARS - ;

?: ALIGNED ( addr -- a-addr ) 
?\		CELL+ 1- 1 CELLS 1- INVERT AND 
?\ ;
?: ALIGN ( -- ) HERE ALIGNED HERE - ALLOT ;

\ PLATFORM DEPENDENT - Not ANS
?: HERE, ( x -- ) HERE 2 CELLS + POSTPONE LITERAL ;
\ Not ANS
?: 0! ( a-addr -- ) 0 SWAP ! ;
?: +! ( n | u a-addr -- ) SWAP OVER @ + SWAP ! ;
\ Not ANS
?: 1+! ( a-addr -- ) DUP @ 1 + SWAP ! ;
\ Not ANS
?: 1-! ( a-addr -- ) DUP @ 1 - SWAP ! ;

?: 2! ( x1 x2 a-addr -- ) SWAP OVER ! CELL+ ! ;
?: 2@ ( a-addr -- x1 x2 ) DUP CELL+ @ SWAP @ ;

?: FILL ( c-addr u char -- ) 
?\		-ROT BEGIN 
?\			DUP 0> WHILE 
?\			>R 2DUP C! CHAR+ R> 1- 
?\		REPEAT 
?\		DROP DROP DROP 
?\ ;

\ -- Deferred words ---------------------------------------

\ PLATFORM DEPENDENT
?: >BODY ( xt -- a-addr ) 4 CELLS + TO-ABS ;

?: DEFER ( "<spaces>name" -- ) ( EX: i*x -- j*x )
?\		CREATE 0 , DOES> @ EXECUTE ;

?: TO 
?\ ( i*x "<spaces>name" -- ) 
?\ ( C: "<spaces>name" -- )
?\		STATE @ IF
?\			POSTPONE ['] POSTPONE >BODY POSTPONE !
?\		ELSE
?\			' >BODY !
?\		THEN 
?\	; IMMEDIATE

?: IS 
?\ ( xt "<spaces>name" -- ) 
?\ ( C: "<spaces>name" -- )
?\		STATE @ IF
?\			POSTPONE TO
?\		ELSE
?\			['] TO EXECUTE
?\		THEN
?\	; IMMEDIATE

?: DEFER@	( xt1 -- xt2 ) >BODY @ ;

?: DEFER!	( xt2 xt1 -- ) >BODY ! ;

?: ACTION-OF 
?\ ( "<spaces>name" -- xt ) 
?\ ( C: "<spaces>name -- ) 
?\ ( RT: -- xt )
?\		STATE @ IF
?\			POSTPONE ['] POSTPONE DEFER@
?\		ELSE
?\			' DEFER@
?\		THEN
?\	; IMMEDIATE

\ -- Input/output -----------------------------------------

32
?CONSTANT BL

?: CR ( -- ) 10 EMIT ;

?: SPACE ( -- ) BL EMIT ;

\ When implementing spaces, I've seen it as this:
\ begin dup while space 1- repeat drop
\ but that has the problem of doing infinite spaces
\ if, for some reason, n is negative.
?: SPACES ( n -- ) BEGIN DUP 0> WHILE SPACE 1- REPEAT DROP ;

?: TYPE ( c-addr u -- ) 
?\		>R BEGIN 
?\			R@ 0> WHILE 
?\			DUP C@ DUP 32 127 WITHIN IF
?\				EMIT
?\			ELSE
?\				DROP
?\			THEN
?\			1+ R> 1- >R 
?\		REPEAT 
?\		R> DROP DROP
?\	;


