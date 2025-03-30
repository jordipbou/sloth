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

?: U+D		DUP ROT + DUP ROT U< NEGATE ;

?: DNEGATE ( d1 -- d2 ) INVERT SWAP INVERT 1 U+D ROT + ;
?: DABS ( d -- ud ) DUP 0< IF DNEGATE THEN ;

?: SM/REM ( d n1 -- n2 n3 )		\ CORE
\ Symmetric divide of double by single. Return remainder n2
\ and quotient n3.
?\		2DUP XOR >R OVER >R >R DUP 0< IF DNEGATE THEN
?\		R> ABS UM/MOD
?\		R> 0< IF SWAP NEGATE SWAP THEN
?\		R> 0< IF					\ negative quotient
?\		    NEGATE 0 OVER < 0= IF EXIT THEN
?\		    -11 THROW THEN          \ result out of range
?\		DUP 0< IF -11 THROW THEN	\ result out of range
?\ ;

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
\ Not ANS
?: 1! ( a-addr -- ) 1 SWAP ! ;
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

?: CMOVE> ( c-addr1 c-addr2 u -- ) CHARS MOVE ;
?: CMOVE ( c-addr1 c-addr2 u -- ) CHARS MOVE ;

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

\ -- Strings ----------------------------------------------

\ Adapted from Minimal Forth to use CHARS instead of memory
\ units.
?: /STRING ( c-addr1 u1 n -- c-addr2 u2 )
?\		SWAP OVER - >R CHARS + R>
?\ ;

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
?\			CHAR+ R> 1- >R 
?\		REPEAT 
?\		R> DROP DROP
?\	;

\ -- Dump utility -----------------------------------------

\ Adapted from Minimal Forth

?: .HEXDIGIT ( x -- )
?\		15 AND DUP 10 < IF 
?\			'0' + 
?\		ELSE 
?\			10 - 'A' + 
?\		THEN EMIT 
?\	;

?: .HEX ( x -- )
?\		DUP  4 RSHIFT .HEXDIGIT .HEXDIGIT
?\ ;

?: .ADDR ( x -- )
\ Address 00 is not being printed correctly
?\		DUP 0= IF 
?\			DROP '0' EMIT '0' EMIT EXIT 
?\		THEN
?\		0 BEGIN ( x i ) 
?\			OVER WHILE 
?\			OVER 8 RSHIFT SWAP 1+ 
?\		REPEAT SWAP DROP
?\		BEGIN ( x i )
?\			DUP WHILE 
?\			SWAP .HEX 1- 
?\		REPEAT DROP 
?\ ;

16
?CONSTANT B/LINE

\ For all systems where char is one byte this definition
\ is enough. For the systems where char is not one byte,
\ it should be defined in host.
?: B@ ( addr -- byte ) C@ ;

?: .H ( addr len -- )
?\		B/LINE MIN DUP >R
?\		BEGIN ( addr len )
?\			DUP	WHILE ( addr len )
?\			OVER B@ .HEX SPACE
?\			1- SWAP 1+ SWAP
?\		REPEAT 2DROP
?\		B/LINE R> - 3 * SPACES
?\ ;

?: .A ( addr1 len1 -- )
?\		B/LINE MIN
?\		BEGIN ( addr len )
?\			DUP WHILE
?\			OVER B@ DUP BL < IF DROP '.' THEN EMIT
?\			1- SWAP 1+ SWAP
?\		REPEAT 2DROP 
?\ ;

?: DUMP-LINE ( addr len1 -- addr len2 )
?\		OVER .ADDR ':' EMIT SPACE 2DUP .H SPACE SPACE 2DUP .A
?\		DUP B/LINE MIN /STRING
?\ ;

?: DUMP ( addr len -- )
?\		BEGIN
?\			DUP WHILE ( addr len )
?\			CR DUMP-LINE
?\		REPEAT 2DROP 
?\		CR
?\ ;

\ -- Definitions ------------------------------------------

\ PLATFORM DEPENDENT
?: LATEST ( -- n ) 2 CELLS TO-ABS ;

\ PLATFORM DEPENDENT
?: NT>LINK ( nt -- nt ) TO-ABS @ ;

\ -- Markers ----------------------------------------------

\ MARKER creates a word that stores how to delete all
\ definitions created after it from dictionary,
\ including itself.

?: DO-MARKER ( -- )
?\		DUP CELL+ @ HERE - ALLOT
?\		@ LATEST !
?\ ;

\ PLATFORM DEPENDENT
?: MARKER ( "<spaces>name" -- )
?\		HERE LATEST @
?\		CREATE
?\		, ,
?\		DOES> DO-MARKER
?\ ;

\ -- Numeric output ---------------------------------------

\ UD/MOD is needed by #, but I don't know what it does.
?: UD/MOD	>R 0 R@ UM/MOD R> SWAP >R UM/MOD R> ;

\ This should be used with [UNDEFINED]

?VARIABLE HLD
?\ CREATE <HOLD 100 CHARS DUP ALLOT <HOLD + CONSTANT HOLD>
   
?: <# ( -- ) HOLD> HLD ! ;
?: HOLD	( c -- ) HLD @ 1- DUP HLD ! C! ;
?: # ( d1 -- d2 ) 
?\		BASE @ UD/MOD ROT 9 OVER < IF 7 + THEN 48 + HOLD 
?\ ;
?: #S ( d1 -- d2 ) BEGIN # OVER OVER OR 0= UNTIL ;
?: #> ( d -- c-addr len ) DROP DROP HLD @ HOLD> OVER - ;
   
?: SIGN	( n -- ) 0 < IF 45 HOLD THEN ;
   
?: UD.R ( d n -- ) >R <# #S #> R> OVER - SPACES TYPE ;
?: UD.		0 UD.R SPACE ;
?: U.R		0 SWAP UD.R ;
?: U.		0 UD. ;
?: D.R ( d n -- )
?\		>R SWAP OVER DABS 
?\		<# #S ROT SIGN #> 
?\		R> OVER - SPACES TYPE 
?\ ;
?: D.		0 D.R SPACE ;
?: .R		>R DUP 0 < R> D.R ;
?: .			DUP 0 <  D. ;
   
?: ? ( addr -- ) @ . ;

\ -- CASE/OF/ENDOF ----------------------------------------

\ Copied from SwapForth

( CASE                                       JCB 09:15 07/18/14)
\ From ANS specification A.3.2.3.2

0 CONSTANT CASE IMMEDIATE  ( init count of ofs )

: OF  ( #of -- orig #of+1 / x -- )
    1+    ( count ofs )
    POSTPONE OVER  POSTPONE = ( copy and test case value)
    POSTPONE IF    ( add orig to control flow stack )
    POSTPONE DROP  ( discards case value if = )
    SWAP           ( bring count back now )
; IMMEDIATE

: ENDOF ( orig1 #of -- orig2 #of )
    >R   ( move off the stack in case the control-flow )
         ( stack is the data stack. )
    POSTPONE ELSE
    R>   ( we can bring count back now )
; IMMEDIATE

: ENDCASE  ( orig1..orign #of -- )
    POSTPONE DROP  ( discard case value )
    BEGIN
        DUP
    WHILE
        SWAP POSTPONE THEN 1-
    REPEAT DROP
; IMMEDIATE

\ -- Forming definite loops -----------------------------

?: IX ( -- addr ) 11 CELLS TO-ABS ;
?: JX ( -- addr ) 12 CELLS TO-ABS ;
?: KX ( -- addr ) 13 CELLS TO-ABS ;
?: LX ( -- addr ) 14 CELLS TO-ABS ;

?: DO ( C: -- do-sys ) ( n1 | u1 n2 | u2 -- ) ( R: -- loop-sys )
?\		1 POSTPONE LITERAL POSTPONE [:
?\ ; IMMEDIATE

?: ?DO ( C: -- do-sys ) ( n1 | u1 n2 | u2 -- ) ( R: -- loop-sys )
?\		0 POSTPONE LITERAL POSTPONE [:
?\ ; IMMEDIATE

?: I ( -- n ) IX @ ;
?: J ( -- n ) JX @ ;
\ Not ANS Forth
?: K ( -- n ) KX @ ;

?: LEAVE ( -- ) ( R: loop-sys -- )
?\		POSTPONE LX POSTPONE 1! POSTPONE EXIT
?\ ; IMMEDIATE

?: LOOP ( C: do-sys -- ) ( -- ) ( R: loop-sys1 -- | loop-sys2 )
?\		1 POSTPONE LITERAL POSTPONE ;] POSTPONE (DOLOOP)
?\ ; IMMEDIATE

?: +LOOP ( C: do-sys -- ) ( -- ) ( R: loop-sys1 -- | loop-sys2 )
?\		POSTPONE ;] POSTPONE (DOLOOP)
?\ ; IMMEDIATE

\ -- Parsing ----------------------------------------------

?: /SOURCE ( -- c-addr n )
?\		SOURCE >IN @ /STRING DUP 0< IF DROP 0 THEN 
?\ ; 

\ Here three words for parsing are implemented, resembling
\ numeric output.

?: <P ( -- c-addr n ) /SOURCE ;
?: *P ( -?- )
?\		>R BEGIN 
?\			/SOURCE NIP WHILE 
?\			/SOURCE DROP C@ R@ EXECUTE WHILE 
?\			>IN 1+! 
?\		REPEAT THEN 
?\		R> DROP 
?\ ;
?: P> ( -?- ) 
?\		/SOURCE NIP DUP >R - R> IF 
?\			>IN 1+! 
?\		THEN 
?\ ;

?: PARSE ( char "ccc<char>" -- c-addr u )
?\		<P ROT [: OVER <> ;] *P DROP P> 
?\ ;

?: CHAR ( "<spaces>name" -- char ) BL PARSE DROP C@ ; 
?: [CHAR] ( "<spaces>name" -- char ) ( -- char )
?\		CHAR POSTPONE LITERAL 
?\ ; IMMEDIATE 

?: PARSE-NAME ( "<spaces>name<space>" -- c-addr u )
?\		[: BL <= ;] *P <P [: BL > ;] *P P> 
?\ ;

\ -- String literals --------------------------------------

?: SLITERAL ( c-addr1 u -- ) ( -- c-addr2 u )
?\		POSTPONE (STRING) DUP ,
?\		HERE OVER CHARS ALLOT
?\		SWAP CMOVE
?\		ALIGN
?\ ; IMMEDIATE

?: S" ( "ccc<quote>" -- ) ( -- c-addr u )
?\		34 PARSE STATE @ IF 
?\			POSTPONE SLITERAL 
?\		THEN 
?\ ; IMMEDIATE 

?: CLITERAL ( c-addr1 u -- ) ( -- c-addr2 )
?\		POSTPONE (CSTRING) DUP C,
?\		HERE OVER CHARS ALLOT
?\		SWAP CMOVE
?\		ALIGN
?\ ; IMMEDIATE

\ TODO: C" only works when compiling
?: C" ( "ccc<quote>" -- ) ( -- c-addr )
?\		34 PARSE POSTPONE CLITERAL
?\ ; IMMEDIATE

