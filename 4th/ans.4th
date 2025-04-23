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

\ -- Compilation wordlist ---------------------------------

\ Defining GET-CURRENT and SET-CURRENT here allows changing
\ between FORTH-WORDLIST and INTERNAL-WORDLIST from the
\ beginning and no overpopulate FORTH-WORDLIST.

?: GET-CURRENT 15 CELLS TO-ABS @ ; \ ( -- wid )
?: SET-CURRENT 15 CELLS TO-ABS ! ; \ ( wid -- )

\ -- Variables shared with the host -----------------------

\ HERE is defined in 0 CELLS TO-ABS
?: BASE					1 CELLS TO-ABS ;	\ ( -- addr )
?: FORTH-WORDLIST		2 CELLS TO-ABS ; 	\ ( -- addr )
?: INTERNAL-WORDLIST	3 CELLS TO-ABS ; 	\ ( -- addr )
?: STATE				4 CELLS TO-ABS ; 	\ ( -- addr )

INTERNAL-WORDLIST SET-CURRENT

?: (IBUF)				5 CELLS TO-ABS ;	\ ( -- addr )
?: (IPOS)				6 CELLS TO-ABS ; 	\ ( -- addr )
?: (ILEN)				7 CELLS TO-ABS ; 	\ ( -- addr )
?: (SOURCE-ID)			8 CELLS TO-ABS ; 	\ ( -- addr )
?: (HLD)				9 CELLS TO-ABS ; 	\ ( -- addr )
?: (LATESTXT)			10 CELLS TO-ABS ;	\ ( -- addr )
?: (IX)					11 CELLS TO-ABS ; 	\ ( -- addr )
?: (JX)					12 CELLS TO-ABS ; 	\ ( -- addr )
?: (KX)					13 CELLS TO-ABS ; 	\ ( -- addr )
?: (LX)					14 CELLS TO-ABS ;	\ ( -- addr )
\ CURRENT goes on 15 CELLS TO-ABS, above defined SET- GET-
?: #ORDER				16 CELLS TO-ABS ;	\ ( -- addr )
?: CONTEXT				17 CELLS TO-ABS ;	\ ( -- addr )

FORTH-WORDLIST SET-CURRENT

?: SOURCE-ID (SOURCE-ID) @ ; \ ( -- 0 | -1 | fileid )

\ -- Adjusting BASE ---------------------------------------

?: DECIMAL	10 BASE ! ; \ ( -- )
?: HEX		16 BASE ! ; \ ( -- )

\ -- Compilation ------------------------------------------

?: ,	HERE ! 1 CELLS ALLOT ; \ ( x -- )
?: C,	HERE C! 1 CHARS ALLOT ; \ ( char -- )

\ PLATFORM DEPENDENT
?: LITERAL POSTPONE (LIT) , ; IMMEDIATE \ ( C: x -- ) ( -- x )

?: ' 32 WORD FIND DROP ; \ ( "<spaces>name" -- xt ) 

?: ['] \ ( "<spaces>name" -- ) ( R: -- xt ) 
?\		' POSTPONE LITERAL 
?\ ; IMMEDIATE

\ -- Variables and constants ------------------------------

?: VARIABLE CREATE 0 , ; \ ( "<spaces>name" -- ) 
?: CONSTANT CREATE , DOES> @ ; \ ( x "<spaces>name" -- ) 

?: 2VARIABLE CREATE 0 , 0 , ; \ ( "<spaces>name" -- )

?: BUFFER: CREATE ALLOT ; \ ( u "<spaces>name" -- ; -- a-addr ) 

?: VALUE CREATE , DOES> @ ; \ ( x "<spaces>name" -- ) 

\ -- Control structures -----------------------------------

\ This words have a good balance between simplicity and
\ usefulness. Some more complicated words (like DO/LOOP)
\ are not implemented now.

\ PLATFORM DEPENDENT (depends on implementation of BRANCH)
?: AHEAD \ ( C: -- orig ) ( -- ) 
?\		POSTPONE (BRANCH) HERE 0 , 
?\ ; IMMEDIATE

\ PLATFORM DEPENDENT (depends on implementation of ?BRANCH)
?: IF \ ( C: -- orig ) ( x -- ) 
?\		POSTPONE (?BRANCH) HERE 0 , 
?\ ; IMMEDIATE

?: THEN \ ( C: -- orig ) ( -- ) 
?\		HERE OVER - SWAP ! 
?\ ; IMMEDIATE

?: ELSE \ ( C: orig1 -- orig2 ) ( -- ) 
?\		POSTPONE AHEAD SWAP POSTPONE THEN 
?\ ; IMMEDIATE

?: BEGIN \ ( C: -- dest ) ( -- ) 
?\		HERE 
?\ ; IMMEDIATE

\ PLATFORM DEPENDENT (depends on implementation of ?BRANCH)
?: UNTIL \ ( C: dest -- ) ( x -- ) 
?\		POSTPONE (?BRANCH) HERE - , 
?\ ; IMMEDIATE

\ PLATFORM DEPENDENT (depends on implementation of BRANCH)
?: AGAIN \ ( C: dest -- ) ( -- ) 
?\		POSTPONE (BRANCH) HERE - , 
?\ ; IMMEDIATE

?: WHILE \ ( C: dest -- orig dest ) ( x -- ) 
?\		POSTPONE IF SWAP 
?\ ; IMMEDIATE

?: REPEAT \ ( C: orig dest -- ) ( -- ) 
?\		POSTPONE AGAIN POSTPONE THEN 
?\ ; IMMEDIATE

\ -- Multi-line paren -------------------------------------

?: ( \ ( "ccc<paren>" -- )
?\		BEGIN
?\			BEGIN
?\				SOURCE >IN @ SWAP < WHILE
?\				>IN @ 0 PICK 1 + >IN !
?\				CHARS + C@ ')' = IF EXIT THEN
?\			REPEAT 
?\		DROP REFILL 0 = UNTIL
?\ ; IMMEDIATE

(
	Now we can use stack comments and 
	multi-line comments!
)

\ -- Conditional compilation of variables and constants ---

?: ?VARIABLE ( "<spaces>name" -- ) ?: ;

?: ?CONSTANT ( x "<spaces>name" -- ) 
?\		32 WORD FIND SWAP DROP
?\		IF DROP SOURCE >IN ! DROP
?\		ELSE 1 >IN !
?\		THEN
?\	;

\ -- Some constants ---------------------------------------

INTERNAL-WORDLIST SET-CURRENT

64
?CONSTANT (CBUF-DISPLACEMENT)	\ Counted string buffer

128
?CONSTANT (SBUF1-DISPLACEMENT)	\ String buffer 1

256
?CONSTANT (SBUF2-DISPLACEMENT)	\ String buffer 2

384
?CONSTANT (NBUF-DISPLACEMENT)	\ Numeric output buffer

416
?CONSTANT (PAD-DISPLACEMENT)	\ PAD

FORTH-WORDLIST SET-CURRENT

-1
?CONSTANT TRUE

0
?CONSTANT FALSE

\ -- Controlling state ------------------------------------

INTERNAL-WORDLIST SET-CURRENT

?VARIABLE (PREV-STATE)

FORTH-WORDLIST SET-CURRENT

?: [ ( -- ) STATE @ (PREV-STATE) ! 0 STATE ! ; IMMEDIATE
?: ] ( -- ) (PREV-STATE) @ STATE ! ; IMMEDIATE

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

?: RDROP ( -- ) ( R: x -- ) 
?\		POSTPONE R> POSTPONE DROP 
?\ ; IMMEDIATE

?: ROT ( x1 x2 x3 -- x2 x3 x1 ) >R SWAP R> SWAP ;	

\ Not ANS
?: -ROT ( x1 x2 x3 -- x3 x1 x2 ) ROT ROT ;

?: NIP ( x1 x2 -- x2 ) SWAP DROP ;
?: TUCK ( x1 x2 -- x2 x1 x2 ) >R R@ SWAP R> ;

\ Not ANS
\ TODO Should this take into account the number of
\ items on the stack, or just do as told without
\ checking anything?
?: DISCARD ( x1 ... xn u -- )
?\		0 OVER < IF
?\			BEGIN
?\				DUP WHILE
?\				NIP
?\				1 -
?\			REPEAT
?\			DROP
?\		ELSE
?\			DROP
?\		THEN
?\ ;

?: ROLL ( xu xu-1 ... x0 u -- xu-1 ... x0 xu ) 
?\		DUP IF 
?\			SWAP >R 1 - RECURSE R> SWAP EXIT 
?\		THEN DROP 
?\ ;

?: 2DROP ( x1 x2 -- ) DROP DROP ;
?: 2DUP ( x1 x2 -- x1 x2 x1 x2 ) OVER OVER ;
?: 2SWAP ( x1 x2 x3 x4 -- x3 x4 x1 x2 ) >R -ROT R> -ROT ;
?: 2OVER ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 ) 3 PICK 3 PICK ;
\ Not ANS
?: 2NIP ( x1 x2 x3 x4 -- x3 x4 ) ROT DROP ROT DROP ;

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

?: */MOD ( n1 n2 n3 -- n4 n5 ) >R M* R> SM/REM ;

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

\ -- Platform definition ----------------------------------

?: WIN64? ( -- flag ) -1 (ENVIRONMENT) 0 = ;
?: WIN32? ( -- flag ) -1 (ENVIRONMENT) 1 = ;
?: WINDOWS? ( -- flag ) 
?\		-1 (ENVIRONMENT)
?\		DUP 0 = SWAP 1 = OR
?\ ;
?: LINUX? ( -- flag ) -1 (ENVIRONMENT) 4 = ;

\ -- More arithmetic operations ---------------------------

?: MIN ( n1 n2 -- n3 ) 2DUP > IF SWAP THEN DROP ;
?: MAX ( n1 n2 -- n3 ) 2DUP < IF SWAP THEN DROP ;

?: S>D ( n -- d ) DUP 0< ;
?: D>S ( d -- n ) DROP ;

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

\ Taken from ANS Forth Standard reference implementation

?: FM/MOD ( d n -- rem quot )
?\		DUP >R
?\		SM/REM
\ if the remainder is not zero and has a different sign 
\ than the divisor
?\		OVER DUP 0<> SWAP 0< R@ 0< XOR AND IF
?\			1- SWAP R> + SWAP
?\		ELSE
?\			RDROP
?\		THEN
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

\ -- Forming definite loops -----------------------------

\ ?: IX ( -- addr ) 11 CELLS TO-ABS ;
\ ?: JX ( -- addr ) 12 CELLS TO-ABS ;
\ ?: KX ( -- addr ) 13 CELLS TO-ABS ;
\ ?: LX ( -- addr ) 14 CELLS TO-ABS ;

?: DO ( C: -- do-sys ) ( n1 | u1 n2 | u2 -- ) ( R: -- loop-sys )
?\		1 POSTPONE LITERAL POSTPONE [:
?\ ; IMMEDIATE

?: ?DO ( C: -- do-sys ) ( n1 | u1 n2 | u2 -- ) ( R: -- loop-sys )
?\		0 POSTPONE LITERAL POSTPONE [:
?\ ; IMMEDIATE

?: I ( -- n ) (IX) @ ;
?: J ( -- n ) (JX) @ ;
\ Not ANS Forth
?: K ( -- n ) (KX) @ ;

?: LEAVE ( -- ) ( R: loop-sys -- )
?\		POSTPONE (LX) POSTPONE 1! POSTPONE EXIT
?\ ; IMMEDIATE

?: LOOP ( C: do-sys -- ) ( -- ) ( R: loop-sys1 -- | loop-sys2 )
?\		1 POSTPONE LITERAL POSTPONE ;] POSTPONE (DOLOOP)
?\ ; IMMEDIATE

?: +LOOP ( C: do-sys -- ) ( -- ) ( R: loop-sys1 -- | loop-sys2 )
?\		POSTPONE ;] POSTPONE (DOLOOP)
?\ ; IMMEDIATE

\ -- Strings ----------------------------------------------

32
?CONSTANT BL

?: CR ( -- ) 10 EMIT ;

?: SPACE ( -- ) BL EMIT ;

\ When implementing spaces, I've seen it as this:
\ begin dup while space 1- repeat drop
\ but that has the problem of doing infinite spaces
\ if, for some reason, n is negative.
?: SPACES ( n -- ) BEGIN DUP 0> WHILE SPACE 1- REPEAT DROP ;

?: COUNT ( c-addr1 -- c-addr2 u )
?\		DUP CHAR+ SWAP C@ 
?\ ;

?: BOUNDS ( c-addr1 u -- c-addr1 c-addr2 ) CHARS OVER + SWAP ;

?: PAD ( -- c-addr ) HERE (PAD-DISPLACEMENT) + ;

\ Adapted from Minimal Forth to use CHARS instead of memory
\ units.
?: /STRING ( c-addr1 u1 n -- c-addr2 u2 )
?\		SWAP OVER - >R CHARS + R>
?\ ;

?: -TRAILING ( c-addr u1 -- c-addr u2 )
?\		BEGIN   
?\		    2DUP + CHAR- C@ BL =
?\		    OVER AND
?\		WHILE   
?\		    CHAR-  
?\		REPEAT  
?\ ;

?: FILL ( c-addr u char -- ) 
?\		-ROT BEGIN 
?\			DUP 0> WHILE 
?\			>R 2DUP C! CHAR+ R> 1- 
?\		REPEAT 
?\		DROP DROP DROP 
?\ ;

?: ERASE ( c-addr n -- ) 0 FILL ;

?: BLANK ( c-addr u -- ) BL FILL ;

?: CMOVE> ( c-addr1 c-addr2 u -- ) CHARS MOVE ;
?: CMOVE ( c-addr1 c-addr2 u -- ) CHARS MOVE ;

\ COMPARE implementation taken from SwapForth

?: COMPARE-SAME? ( c-addr1 c-addr2 u -- -1|0|1 )
?\		BOUNDS ?DO
?\			I C@ OVER C@ - ?DUP IF
?\				0> 2* 1+
?\				NIP UNLOOP EXIT
?\			THEN
?\			1+
?\		LOOP
?\		DROP 0
?\ ;

?: COMPARE ( c-addr1 u1 c-addr2 u2 -- n )
?\		ROT 2DUP SWAP - >R          \ ca1 ca2 u2 u1  r: u1-u2
?\		MIN COMPARE-SAME? ?DUP
?\		IF R> DROP EXIT THEN
?\		R> DUP IF 0< 2* 1+ THEN 
?\ ;

\ Implementation taken from lbForth

\ TODO Does not return u3 == u1 when does not find anything

?: SEARCH ( c-addr1 u1 c-addr2 u2 -- c-addr3 u3 flag )   
?\		2>R BEGIN 
?\			2DUP R@ MIN 2R@ COMPARE WHILE 
?\			DUP WHILE
?\			1 /STRING 
?\		REPEAT 
?\		0 ELSE -1 
?\		THEN 
?\		2R> 2DROP 
?\ ;

\ Implementation from ANS Forth standard comment
?: TYPE ( c-addr u -- ) 0 ?DO COUNT EMIT LOOP DROP ;

?: (RETURN-KEY) ( -- n )
?\		WINDOWS? IF 13 
?\		ELSE LINUX? IF 10 THEN
?\		THEN
?\		POSTPONE LITERAL
?\ ; IMMEDIATE

?: (DELETE-KEY) ( -- n )
?\		WINDOWS? IF 8
?\		ELSE LINUX? IF 127 THEN
?\		THEN
?\		POSTPONE LITERAL
?\ ; IMMEDIATE

?: ACCEPT ( c-addr +n1 -- +n2 )
?\		BOUNDS ( c-addr2 c-addr1 )
?\		2DUP - >R
?\		BEGIN ( c-addr2 c-addr1 )
?\			2DUP <> WHILE
?\			KEY DUP (RETURN-KEY) <> WHILE
?\			DUP (DELETE-KEY) = IF
?\				2DUP - R@ <> IF
?\					DROP
?\					8 EMIT 32 EMIT 8 EMIT
?\					CHAR-	
?\				THEN
?\			ELSE
?\				DUP 31 > IF
?\					DUP EMIT
?\					OVER C!
?\					CHAR+
?\				THEN
?\			THEN
?\		REPEAT 
?\			DROP ( drop key value ) 
?\			SPACE ( add one space for clarity )
?\		THEN
?\		- R> SWAP -
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
?: U.R ( u n -- ) 0 SWAP UD.R ;
?: U. ( u -- ) 0 UD. ;
?: D.R ( d n -- )
?\		>R SWAP OVER DABS 
?\		<# #S ROT SIGN #> 
?\		R> OVER - SPACES TYPE 
?\ ;
?: D.		0 D.R SPACE ;
?: .R ( n1 n2 -- ) >R DUP 0 < R> D.R ;
?: . ( n -- ) DUP 0 < D. ;
   
?: ? ( addr -- ) @ . ;

\ Taken from SwapForth that indicates taken from standard
?: HOLDS ( addr u -- )
?\		BEGIN DUP WHILE 1- 2DUP + C@ HOLD REPEAT 2DROP
?\ ;

\ -- Definitions ------------------------------------------

1
?CONSTANT HIDDEN-FLAG

2
?CONSTANT IMMEDIATE-FLAG

\ PLATFORM DEPENDENT
?: NAME>LINK ( nt -- nt ) TO-ABS @ ;
?: NAME>XT ( nt -- xt ) CELL+ TO-ABS @ ;
?: NAME>STRING ( nt -- c-addr u )
?\		2 CELLS + CHAR+ TO-ABS COUNT
?\ ;

?: NAME>FLAGS ( nt -- n ) 2 CELLs + TO-ABS C@ ;

?: HIDDEN? ( nt -- flag ) NAME>FLAGS HIDDEN-FLAG AND ;
?: IMMEDIATE? ( nt -- flag ) NAME>FLAGS IMMEDIATE-FLAG AND ;

\ -- Search order -----------------------------------------

\ Implementation taken from reference implementation in
\ ANS Forth Standard

?: GET-ORDER ( -- wid1 ... widn n )
?\		#ORDER @ 0 ?DO
?\			#ORDER @ I - 1- CELLS CONTEXT + @
?\		LOOP
?\		#ORDER @
?\ ; 

?: SET-ORDER ( wid1 ... widn n -0 )
?\		DUP -1 = IF
?\			DROP FORTH-WORDLIST 1
?\		THEN
?\		DUP #order !
?\		0 ?DO I CELLS context + ! LOOP
?\ ;

?: WORDLIST ( -- wid ) HERE 0 , ;

?: ALSO ( -- ) GET-ORDER OVER SWAP 1+ SET-ORDER ;
?: DEFINITIONS ( -- ) GET-ORDER SWAP SET-CURRENT DISCARD ;
?: FORTH ( -- ) GET-ORDER NIP FORTH-WORDLIST SWAP SET-ORDER ;
?: ONLY ( -- ) -1 SET-ORDER ;
?: PREVIOUS ( -- ) GET-ORDER NIP 1- SET-ORDER ;

?: ORDER ( -- )
?\		CR GET-ORDER DUP . 0 ?DO . LOOP CR
?\		GET-CURRENT . CR
?\ ;

\ -- Markers ----------------------------------------------

\ MARKER creates a word that stores how to delete all
\ definitions created after it from dictionary,
\ including itself.

?: DO-MARKER ( -- )
?\		DUP CELL+ @ HERE - ALLOT
?\		@ GET-CURRENT !
?\ ;

\ PLATFORM DEPENDENT
?: MARKER ( "<spaces>name" -- )
?\		HERE GET-CURRENT @
?\		CREATE
?\		, ,
?\		DOES> DO-MARKER
?\ ;

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

\ -- Stack visualization ----------------------------------

?: .S ( -- ) 
?\		'<' EMIT DEPTH 0 0 D.R '>' EMIT SPACE
?\		DEPTH 0 > IF
?\			1 DEPTH 1- DO
?\				I 1- PICK .
?\			-1 +LOOP
?\		THEN
?\ ;

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
?\		<P ROT [: OVER OVER 10 <> >R <> R> AND ;] *P DROP P> 
?\ ;

?: CHAR ( "<spaces>name" -- char ) BL PARSE DROP C@ ; 
?: [CHAR] ( "<spaces>name" -- char ) ( -- char )
?\		CHAR POSTPONE LITERAL 
?\ ; IMMEDIATE 

?: PARSE-NAME ( "<spaces>name<space>" -- c-addr u )
?\		[: BL <= ;] *P <P [: BL > ;] *P P> 
?\ ;

\ -- Combinators ------------------------------------------

\ Removes n from the data stack, executes xt and restores n
?: DIP ( n xt -- n ) SWAP >R EXECUTE R> ;

\ Applies xt1 to n1, then applies xt2 to n2
?: BI* ( n1 n2 xt1 xt2 -- ) ['] DIP DIP EXECUTE ;

\ Applies xt to n1 and then applies xt to n2
?: BI@ ( n1 n2 xt -- ) DUP BI* ;


\ -- Finding words ----------------------------------------

?: LETTER>UPPER ( char -- char )
?\		DUP 97 123 WITHIN IF
?\			32 -
?\		THEN
?\ ;

?: CASE-INSENSITIVE-= ( char char -- ) 
?\		['] LETTER>UPPER BI@ = 
?\ ;

?: CASE-INSENSITIVE-COMPARE ( c-addr1 u1 c-addr2 u2 -- flag )
?\		ROT OVER = IF
?\			BOUNDS ( c-addr1 c-addr2 c-addr2+u ) DO
?\				I C@ OVER C@ CASE-INSENSITIVE-= 0= IF
?\					DROP FALSE UNLOOP EXIT
?\				THEN
?\				CHAR+
?\			1 CHARS +LOOP
?\			DROP TRUE
?\		ELSE
?\			DROP DROP DROP FALSE
?\		THEN
?\ ;

?: WORDLIST-LATEST ( wid -- nt ) @ ;

?: FIND-NAME-IN ( c-addr u wid -- nt | 0 )
?\		WORDLIST-LATEST >R BEGIN
?\			R@ WHILE
?\			R@ NAME>STRING
?\			2OVER CASE-INSENSITIVE-COMPARE IF
?\				DROP DROP R> EXIT
?\			THEN
?\			R> NAME>LINK >R
?\		REPEAT
?\		2DROP R>
?\ ;

?: SEARCH-WORDLIST ( c-addr u wid -- 0 | xt 1 | xt -1 )
?\		FIND-NAME-IN DUP IF
?\			DUP NAME>XT SWAP
?\			IMMEDIATE? IF 1 ELSE -1 THEN
?\		THEN

?\ ;

?: TRAVERSE-WORDLIST ( i*x xt wid -- j*x )
?\		@ 2>R BEGIN ( R: xt latest )
?\			R@ WHILE
?\			2R@ SWAP EXECUTE IF
?\				R> NAME>LINK >R
?\			ELSE
?\				2R> EXIT
?\			THEN
?\		REPEAT
?\		2R> 2DROP
?\ ;

\ \ Not ANS yet
\ ?: FIND-NAME ( c-addr u -- nt | 0 )
\ TODO I need some way to traverse the search order
\ TODO and execute FIND-NAME-IN until the word is found
\ ?\ ;

\ TODO I need to implement wordlists and search order to
\ be able to implement FIND-NAME.
\ It would be interesting to implement it in C also, as
\ find_word is used from C and it needs to work even when
\ the search order is implemented in Forth.

\ -- ANS Forth conditional compilation --------------------

\ Implementation taken from ANS reference implementation

?: [DEFINED] ( "<spaces>name ..." -- flag ) 
?\		BL WORD FIND NIP 0<>
?\ ; IMMEDIATE

?: [UNDEFINED] ( "<sapces>name ..." -- flag )
?\		BL WORD FIND NIP 0=
?\ ; IMMEDIATE

\ Implementation of [IF] [ELSE] [THEN] by ruv, as seen in:
\ https://forth-standard.org/standard/tools/BracketELSE

WORDLIST DUP 
CONSTANT BRACKET-FLOW-WL 
GET-CURRENT SWAP SET-CURRENT

?: [IF]		1+ ; \ ( level1 -- level2 )
?: [ELSE]	DUP 1 = IF 1- THEN ; \ ( level1 -- level2 )
?: [THEN]	1- ; \ ( level1 -- level2 )

SET-CURRENT

?: [ELSE] \ ( -- )
?\		1 BEGIN 
?\			BEGIN 
?\				PARSE-NAME DUP WHILE
?\				BRACKET-FLOW-WL SEARCH-WORDLIST IF 
?\					EXECUTE DUP 0= IF DROP EXIT THEN
?\				THEN
?\			REPEAT 
?\			2DROP REFILL 0= 
?\		UNTIL 
?\		DROP
?\ ; IMMEDIATE

?: [THEN]	; IMMEDIATE \ ( -- )

?: [IF]		0= IF POSTPONE [ELSE] THEN ; IMMEDIATE \ ( flag -- )
   
\ -- String literals --------------------------------------

?: SLITERAL ( c-addr1 u -- ) ( -- c-addr2 u )
?\		POSTPONE (STRING) DUP ,
?\		HERE OVER CHARS ALLOT
?\		SWAP CMOVE
?\		0 C,	\ Zero ended string literal
?\		ALIGN
?\ ; IMMEDIATE

?: CLITERAL ( c-addr1 u -- ) ( -- c-addr2 )
?\		POSTPONE (CSTRING) DUP C,
?\		HERE OVER CHARS ALLOT
?\		SWAP CMOVE
?\		0 C,	\ Zero ended string literal
?\		ALIGN
?\ ; IMMEDIATE

\ The following definitions will have an end-of-line
\ comment with a quote to not break syntax highlighting.

?: S" ( "ccc<quote>" -- ) ( -- c-addr u )	\ "
?\		34 PARSE STATE @ IF 
?\			POSTPONE SLITERAL 
?\		THEN 
?\ ; IMMEDIATE 

\ TODO: C" only works when compiling, is that correct?
?: C" ( "ccc<quote>" -- ) ( -- c-addr )		\ "
?\		34 PARSE POSTPONE CLITERAL
?\ ; IMMEDIATE

?: ." ( "ccc<quote>" -- ) ( -- ) \ "
?\		34 PARSE STATE @ IF 
?\			POSTPONE SLITERAL POSTPONE TYPE 
?\		ELSE 
?\			TYPE 
?\		THEN 
?\ ; IMMEDIATE 

?: .( ( "ccc<paren>" -- )
?\		')' PARSE TYPE
?\ ; IMMEDIATE

\ -- Number conversion ------------------------------------

\ TODO: This number conversion seems to be pretty basic.

\ DIGIT Taken from pFORTH
\ Convert a single character to a number in the given base.
?: DIGIT   ( char base -- n true | char false )
?\		>R
\ convert lower to upper
?\		DUP [CHAR] a < NOT
?\		IF
?\			[CHAR] a - [CHAR] A +
?\		THEN
?\		
?\		DUP DUP [CHAR] A 1- >
?\		IF [CHAR] A - [CHAR] 9 + 1+
?\		ELSE ( CHAR CHAR )
?\			DUP [CHAR] 9 >
?\			IF
?\				( BETWEEN 9 AND A IS BAD )
?\				DROP 0 ( TRIGGER ERROR BELOW )
?\			THEN
?\		THEN
?\		[CHAR] 0 -
?\		DUP R> <
?\		IF DUP 1+ 0>
?\			IF NIP TRUE
?\			ELSE DROP FALSE
?\			THEN
?\		ELSE DROP FALSE
?\		THEN
?\ ;

\ >NUMBER taken from pForth
?: >NUMBER ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 , convert till bad char , CORE )
?\		>R
?\		BEGIN
?\			R@ 0>    \ ANY CHARACTERS LEFT?
?\			IF
?\				DUP C@ BASE @
?\				DIGIT ( UD1 C-ADDR , N TRUE | CHAR FALSE )
?\				IF
?\					TRUE
?\				ELSE
?\					DROP FALSE
?\				THEN
?\			ELSE
?\				FALSE
?\			THEN
?\		WHILE ( -- UD1 C-ADDR N  )
?\			SWAP >R  ( -- UD1LO UD1HI N  )
?\			SWAP  BASE @ ( -- UD1LO N UD1HI BASE  )
?\			UM* DROP ( -- UD1LO N UD1HI*BASELO  )
?\			ROT  BASE @ ( -- N UD1HI*BASELO UD1LO BASE )
?\			UM* ( -- N UD1HI*BASELO UD1LO*BASELLO UD1LO*BASELHI )
?\			D+  ( -- UD2 )
?\			R> CHAR+     \ INCREMENT CHAR*
?\			R> 1- >R  \ DECREMENT COUNT
?\		REPEAT
?\		R>
?\ ;

\ -- Commands to inspect memory, debug & view code --------

\ -- Dump (inspecting memory)

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

\ -- See (inspecting words)

\ TODO This should be modified to use wordlists and the
\ search order.

\ TODO Make it better to take into account literals, rips,
\ quotations and strings.

\ PLATFORM DEPENDENT
?: SEE ( "<spaces>name" -- )
?\		BL WORD DUP FIND
?\		DUP IF
?\			ROT ." NAME: " COUNT TYPE CR
?\			0< IF ." NON " THEN ." IMMEDIATE" CR
?\			DUP ." XT: " . CR
?\			DUP 0> IF
?\				TO-ABS BEGIN
?\					DUP @ ['] EXIT <> WHILE
?\					DUP @ . 
?\					CELL+
?\				REPEAT
?\				." ;" CR
?\			ELSE
?\				DROP
?\			THEN
?\		ELSE
?\			." WORD NOT FOUND" CR DROP DROP DROP
?\		THEN
?\ ;

\ -- Words 

?: WORDS ( -- )
?\		GET-ORDER SWAP >R 1- DISCARD R>
?\		[: NAME>STRING TYPE SPACE TRUE ;] SWAP 
?\		CR TRAVERSE-WORDLIST
?\ ;

\ -- Facility -----------------------------------------------

\ Adapted from SwapForth

?: AT-XY ( u1 u2 ) \ cursor to column u1, row u2
?\		27 EMIT '[' EMIT	\ Control sequence introducer
?\		1+ 0 U.R
?\		';' EMIT
?\		1+ 0 U.R
?\		'H' EMIT
?\ ;

\ -- Structures -------------------------------------------

\ Structure implementation taken from ANS Forth standard

?: BEGIN-STRUCTURE  ( -- addr 0 ; -- size )
?\		CREATE
?\			HERE 0 0 ,      \ mark stack, lay dummy
?\		DOES> @             \ -- rec-len
;

?: +FIELD  ( n <"name"> -- ; Exec: addr -- 'addr )
?\		CREATE OVER , +
?\		DOES> @ +
?\ ;

?: FIELD: ( n1 "name" -- n2 ; addr1 -- addr2 )
?\		ALIGNED 1 CELLS +FIELD 
?\ ;
?: CFIELD: ( n1 "name" -- n2 ; addr1 -- addr2 )
\ TODO Shouldn't CHARS be also aligned?
?\		1 CHARS +FIELD 
?\ ;
?: FFIELD: ( n1 "name" -- n2 ; addr1 -- addr2 )
?\		FALIGNED 1 FLOATS +FIELD 
?\ ;
?: SFFIELD: ( n1 "name" -- n2 ; addr1 -- addr2 )
?\		SFALIGNED 1 SFLOATS +FIELD 
?\ ;
?: DFFIELD: ( n1 "name" -- n2 ; addr1 -- addr2 )
?\		DFALIGNED 1 DFLOATS +FIELD 
?\ ;

?: END-STRUCTURE ( addr n -- )
?\		SWAP !           \ set len 
?\ ;

\ -- Environment queries ----------------------------------

[UNDEFINED] ENVIRONMENT? [IF]
: ENVIRONMENT? ( c-addr u -- false | i*x true )
	2DUP S" /COUNTED-STRING" COMPARE 0= IF
		2DROP 0 (ENVIRONMENT) -1
	ELSE 2DUP S" /HOLD" COMPARE 0= IF
		2DROP 1 (ENVIRONMENT) -1
	ELSE 2DUP S" /PAD" COMPARE 0= IF
		2DROP 2 (ENVIRONMENT) -1
	ELSE 2DUP S" ADDRESS-UNIT-BITS" COMPARE 0= IF
		2DROP 3 (ENVIRONMENT) -1
	ELSE 2DUP S" FLOORED" COMPARE 0= IF
		2DROP 4 (ENVIRONMENT) -1
	ELSE 2DUP S" MAX-CHAR" COMPARE 0= IF
		2DROP 5 (ENVIRONMENT) -1
	ELSE 2DUP S" MAX-D" COMPARE 0= IF
		2DROP 6 (ENVIRONMENT) -1
	ELSE 2DUP S" MAX-N" COMPARE 0= IF
		2DROP 7 (ENVIRONMENT) -1
	ELSE 2DUP S" MAX-U" COMPARE 0= IF
		2DROP 8 (ENVIRONMENT) -1
	ELSE 2DUP S" MAX-UD" COMPARE 0= IF
		2DROP 9 (ENVIRONMENT) -1
	ELSE 2DUP S" RETURN-STACK-CELLS" COMPARE 0= IF
		2DROP 10 (ENVIRONMENT) -1
	ELSE 2DUP S" STACK-CELLS" COMPARE 0= IF
		2DROP 11 (ENVIRONMENT) -1
	ELSE 2DUP S" FLOATING-STACK" COMPARE 0= IF
		2DROP 12 (ENVIRONMENT) -1
\ Obsolescent environmental queries
	ELSE 2DUP S" FLOATING" COMPARE 0= IF
		2DROP 100 (ENVIRONMENT) -1
\ Custom environmental queries
	ELSE 2DUP S" PLATFORM" COMPARE 0= IF
		2DROP -1 (ENVIRONMENT) -1
	ELSE
		2DROP FALSE
	THEN THEN THEN THEN THEN THEN THEN THEN THEN
	THEN THEN THEN THEN THEN THEN
;
[THEN]

\ -- QUIT -------------------------------------------------

[UNDEFINED] QUIT [IF]

INTERNAL-WORDLIST SET-CURRENT

S" FLOATING-STACK" ENVIRONMENT? [IF] DROP
: PRINT-FDEPTH ( -- ) 
	FDEPTH 0 > IF SPACE ." f:" FDEPTH . THEN
;
[ELSE]
: PRINT-FDEPTH ( -- ) ;
[THEN]

FORTH-WORDLIST SET-CURRENT

: QUIT
		(EMPTY-RETURN-STACK)
		0 (SOURCE-ID) !		\ Set source to user input device
		POSTPONE [
		BEGIN
			REFILL
		WHILE
			['] INTERPRET CATCH
			CASE
			0 OF STATE @ 0= IF 
				."  OK" 
				DEPTH 0 > IF SPACE DEPTH . THEN
				PRINT-FDEPTH
		THEN CR ENDOF
		POSTPONE [
		-1 OF ( TODO Aborted )  ENDOF
		-2 OF ( TODO display message from ABORT" ) ENDOF
		( default ) DUP ." Exception # " . CR
		ENDCASE
	REPEAT BYE
;

[THEN]

\ -- Exceptions -------------------------------------------

?: ABORT ( i*x -- ) ( R: j*x -- ) -1 THROW ;

[UNDEFINED] ABORT" [IF] \ " for correct syntax highlighting

INTERNAL-WORDLIST SET-CURRENT

: (ABORT") ( n c-addr u -- )
	ROT 0= IF
		2DROP
	ELSE
		-2 THROW
	THEN
;

FORTH-WORDLIST SET-CURRENT

: ABORT"
	POSTPONE S"
	POSTPONE (ABORT")
; IMMEDIATE

[THEN]

\ -- S\" --------------------------------------------------

\ Implementation taken from 
\ http://www.forth200x.org/escaped-strings.html

DECIMAL

[UNDEFINED] S\" [IF] \ " comment added for syntax
\ TODO This one should be in memory
: c+!           \ c c-addr --
\ *G Add character C to the contents of address C-ADDR.
  tuck c@ + swap c!
;

: addchar       \ char string --
\ *G Add the character to the end of the counted string.
  tuck count + c!
  1 swap c+!
;

: append        \ c-addr u $dest --
\ *G Add the string described by C-ADDR U to the counted string at
\ ** $DEST. The strings must not overlap.
  >r
  tuck  r@ count +  swap cmove          \ add source to end
  r> c+!                                \ add length to count
;

: extract2H	\ c-addr len -- c-addr' len' u
\ *G Extract a two-digit hex number in the given base from the
\ ** start of the string, returning the remaining string
\ ** and the converted number.
  base @ >r  hex
  0 0 2over drop 2 >number 2drop drop
  >r  2 /string  r>
  r> base !
;

create EscapeTable      \ -- addr
\ *G Table of translations for \a..\z.
        7 c,	\ \a BEL (Alert)
        8 c,	\ \b BS  (Backspace)
   char c c,    \ \c
   char d c,    \ \d
       27 c,	\ \e ESC (Escape)
       12 c,	\ \f FF  (Form feed)
   char g c,    \ \g
   char h c,    \ \h
   char i c,    \ \i
   char j c,    \ \j
   char k c,    \ \k
       10 c,	\ \l LF  (Line feed)
   char m c,    \ \m
       10 c,    \ \n (Unices only)
   char o c,    \ \o
   char p c,    \ \p
   char " c,    \ \q "   (Double quote)
       13 c,	\ \r CR  (Carriage Return)
   char s c,    \ \s
        9 c,	\ \t HT  (horizontal tab}
   char u c,    \ \u
       11 c,	\ \v VT  (vertical tab)
   char w c,    \ \w
   char x c,    \ \x
   char y c,    \ \y
        0 c,	\ \z NUL (no character)

create CRLF$    \ -- addr ; CR/LF as counted string
	2 c,  13 c,  10 c,

: addEscape	\ c-addr len dest -- c-addr' len'
\ *G Add an escape sequence to the counted string at dest,
\ ** returning the remaining string.
  over 0=                               \ zero length check
  if  drop  exit  then
  >r                                    \ -- caddr len ; R: -- dest
  over c@ [char] x = if                 \ hex number?
    1 /string extract2H r> addchar  exit
  then
  over c@ [char] m = if                 \ CR/LF pair
    1 /string  13 r@ addchar  10 r> addchar  exit
  then
  over c@ [char] n = if                 \ CR/LF pair? (Windows/DOS only)
    1 /string  crlf$ count r> append  exit
  then
  over c@ [char] a [char] z 1+ within if
    over c@ [char] a - EscapeTable + c@  r> addchar
  else
    over c@ r> addchar
  then
  1 /string
;

: parse\"	\ c-addr len dest -- c-addr' len'
\ *G Parses a string up to an unescaped '"', translating '\'
\ ** escapes to characters. The translated string is a
\ ** counted string at *\i{dest}.
\ ** The supported escapes (case sensitive) are:
\ *D \a      BEL          (alert)
\ *D \b      BS           (backspace)
\ *D \e      ESC (not in C99)
\ *D \f      FF           (form feed)
\ *D \l      LF (ASCII 10)
\ *D \m      CR/LF pair - for HTML etc.
\ *D \n      newline - CRLF for Windows/DOS, LF for Unices
\ *D \q      double-quote
\ *D \r      CR (ASCII 13)
\ *D \t      HT (tab)
\ *D \v      VT
\ *D \z      NUL (ASCII 0)
\ *D \"      double-quote
\ *D \xAB    Two char Hex numerical character value
\ *D \\      backslash itself
\ *D \       before any other character represents that character
  dup >r  0 swap c!                     \ zero destination
  begin                                 \ -- caddr len ; R: -- dest
    dup
   while
    over c@ [char] " <>                 \ check for terminator
   while
    over c@ [char] \ = if               \ deal with escapes
      1 /string r@ addEscape
    else                                \ normal character
      over c@ r@ addchar  1 /string
    then
  repeat then
  dup                                   \ step over terminating "
  if 1 /string  then
  r> drop
;

create pocket  \ -- addr
\ *G A tempory buffer to hold processed string.
\    This would normally be an internal system buffer.

s" /COUNTED-STRING" environment? 0= [if] 256 [then]
1 chars + allot

: readEscaped	\ "ccc" -- c-addr
\ *G Parses an escaped string from the input stream according to
\ ** the rules of *\fo{parse\"} above, returning the address
\ ** of the translated counted string in *\fo{POCKET}.
  source >in @ /string tuck             \ -- len caddr len
  pocket parse\" nip
  - >in +!
  pocket
;

: S\"           \ "string" -- caddr u
\ *G As *\fo{S"}, but translates escaped characters using
\ ** *\fo{parse\"} above.
  readEscaped count  state @
  if  postpone sliteral  then
; IMMEDIATE
[THEN]

\ -- Input state ------------------------------------------

[UNDEFINED] SAVE-INPUT [UNDEFINED] RESTORE-INPUT AND [IF]
: SAVE-INPUT ( -- xn ... x1 n )
	SOURCE-ID SOURCE >IN @ 3
;

: RESTORE-INPUT ( xn ... x1 n -- )
	DROP (IPOS) ! (ILEN) ! (IBUF) ! (SOURCE-ID) ! FALSE
;
[THEN]

\ -- N>R NR> ----------------------------------------------

[UNDEFINED] N>R [IF]
: N>R ( i*n +n -- ) ( R: -- j*x +n )
	R> OVER >R SWAP BEGIN 
		?DUP WHILE 
		ROT R> 2>R 1 - 
	REPEAT >R 
;
[THEN]

[UNDEFINED] NR> [IF]
: NR> ( -- i*x +n ) ( R: j*x +n -- )
	R> R@ BEGIN 
		?DUP WHILE 
		2R> >R ROT ROT 1 - 
	REPEAT R> SWAP >R 
;
[THEN]

\ == FLOATING POINT WORD SET ==============================

S" FLOATING-STACK" ENVIRONMENT? [IF] DROP

INTERNAL-WORDLIST SET-CURRENT
	
?: F, (	F: r -- ) HERE F! 1 FLOATS ALLOT ;
	
FORTH-WORDLIST SET-CURRENT
	
?: FVARIABLE ( "<spaces>name" -- ) CREATE 0E F, ;
?: FCONSTANT ( "<spaces>name" -- ) ( F: r -- ) CREATE F, DOES> F@ ;

?: S>F ( n -- ) ( F: -- r ) S>D D>F ;
?: F>S ( -- n ) ( F: r -- ) F>D D>S ;

[UNDEFINED] SET-PRECISION [IF]

VARIABLE (PRECISION)

: SET-PRECISION ( u -- ) (PRECISION) ! ;

[THEN]

?: F> ( -- flag ) ( F: r1 r2 -- ) FSWAP F< ;

[THEN]
