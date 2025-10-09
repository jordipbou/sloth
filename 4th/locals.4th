\ SLOTH Implementation of Locals wordset

INCLUDE THEFORTH.NET/STACK/1.0.0/STACK.4TH
REQUIRE TRANSIENT.4TH

32 CONSTANT (MAX-LOCALS)
(MAX-LOCALS) STACK CONSTANT (LOCALS-STACK)
VARIABLE (LOCALS-USED) 0 (LOCALS-USED) !

\ TODO Is not necessary to take out the safety as
\ the locals stack will be initialized at first.

: PUT-STACK ( x n stack-id -- )
   \ 2DUP DEPTH-STACK 0 SWAP WITHIN 0= IF -9 THROW THEN
   CELL+ SWAP CELLS + !
;	

: PICK-STACK ( n stack-id -- )
	CELL+ SWAP CELLS + @
;

: >L ( x -- ) ( L: -- x ) (locals-stack) >stack ;

\ : 1>L ( x -- ) ( L: -- x ) 
\ 	postpone >l 
\ 	1 (locals-used) ! 
\ ; immediate
\ 
\ : 2>L ( x1 x2 -- ) ( L: -- x2 x1 ) 
\ 	postpone >l postpone >l 
\ 	2 (locals-used) ! 
\ ; immediate
\ 
\ : 3>L ( x1 x2 x3 -- ) ( L: -- x3 x2 x1 ) 
\ 	postpone >l postpone >l postpone >l 
\ 	3 (locals-used) ! 
\ ; immediate
\ 
\ : 4>L ( x1 x2 x3 x4 -- ) ( L: -- x4 x3 x2 x1 ) 
\ 	postpone >l postpone >l postpone >l postpone >l 
\ 	4 (locals-used) ! 
\ ; immediate

: L> ( -- ) (locals-stack) stack> drop ;

\ : 1L> ( -- ) l> ;
\ : 2L> ( -- ) l> l> ;
\ : 3L> ( -- ) l> l> l> ;
\ : 4L> ( -- ) l> l> l> l> ;

\ : L1 ( -- x ) 0 (locals-stack) pick-stack ;
\ : L2 ( -- x ) 1 (locals-stack) pick-stack ;
\ : L3 ( -- x ) 2 (locals-stack) pick-stack ;
\ : L4 ( -- x ) 3 (locals-stack) pick-stack ;

\ : :NONAME DOES> and ; have to be rewritten to allow 
\ the use of locals.

VARIABLE (COLON-TMARKER)

: ALLOT-LOCALS-WL ( -- ) 
	1 CELLS TALLOT DUP
	0 SWAP !
	(LOCALS-WORDLIST) ! 
;
: SET-TMARKER ( -- ) TMARK (COLON-TMARKER) ! ;

: : ( -- TADDR ) SET-TMARKER ALLOT-LOCALS-WL : ; 
: :NONAME ( -- TADDR ) SET-TMARKER ALLOT-LOCALS-WL :NONAME ;

\ TODO I need to compile code to remove locals from local stack !!

: FREE-LOCALS-WL ( -- ) 0 (LOCALS-WORDLIST) ! 0 (LOCALS-USED) ! ;
: RESET-TMARKER ( -- ) (COLON-TMARKER) @ TFREE ;
: ; ( TADDR -- ) 
	(LOCALS-USED) @ 0 ?DO
		POSTPONE L>
	LOOP
	POSTPONE ; 
	FREE-LOCALS-WL 
	RESET-TMARKER 
; IMMEDIATE

\ DOES> acts both as a ; and then a :
: DOES> ( TADDR1 -- TADDR2 )
	(LOCALS-USED) @ 0 ?DO POSTPONE L> LOOP
	FREE-LOCALS-WL
	RESET-TMARKER
	POSTPONE DOES> 
	SET-TMARKER 
	ALLOT-LOCALS-WL
; IMMEDIATE

\ Implementation of (LOCAL)

: (LOCAL-DOES>)
	@ 
	(LOCALS-USED) @ 1- 
	SWAP - 
	POSTPONE LITERAL
	(LOCALS-STACK) POSTPONE LITERAL 
	POSTPONE PICK-STACK 
;

\ I think the local needs to be created differently,
\ not as a value with create-name but as an immediate word
\ that compiles itself what it needs.

: (LOCAL) ( c-addr u -- )
	?DUP 0<> IF
		GET-CURRENT >R (LOCALS-WORDLIST) @ SET-CURRENT
		[: CREATE-NAME (LOCALS-USED) @ , ;] ON-TRANSIENT IMMEDIATE
		(LOCALS-USED) @ 1+ (LOCALS-USED) !
		['] (LOCAL-DOES>) (DOES)
		R> SET-CURRENT
	ELSE
		(locals-used) @ 0 ?DO
			POSTPONE >L
		LOOP
		DROP
	THEN
;

\ As locals here are implemented as a stack and not as values,
\ TO needs to be modified to work correctly with the locals
\ stack.

: TO ( i*x "<spaces>name" -- )
	32 WORD DUP COUNT 
	(LOCALS-WORDLIST) @ SEARCH-WORDLIST IF
		NIP >BODY @
		(LOCALS-USED) @ 1-
		SWAP -
		STATE @ IF
 			POSTPONE LITERAL
 			(LOCALS-STACK) POSTPONE LITERAL POSTPONE PUT-STACK
 		ELSE
 			(LOCALS-STACK) PUT-STACK
		THEN
	ELSE
		FIND DROP >BODY
		STATE @ IF
			POSTPONE LITERAL POSTPONE !
		ELSE
			!
		THEN
	THEN
; IMMEDIATE

\ ---- Reference implementation of {: --------------------------

12345 CONSTANT undefined-value

: match-or-end? ( c-addr1 u1 c-addr2 u2 -- f )
   2 PICK 0= >R COMPARE 0= R> OR ;

: scan-args
   \ 0 c-addr1 u1 -- c-addr1 u1 ... c-addrn un n c-addrn+1 un+1
   BEGIN
     2DUP S" |" match-or-end? 0= WHILE
     2DUP S" --" match-or-end? 0= WHILE
     2DUP S" :}" match-or-end? 0= WHILE
     ROT 1+ PARSE-NAME
   AGAIN THEN THEN THEN ;

: scan-locals
   \ n c-addr1 u1 -- c-addr1 u1 ... c-addrn un n c-addrn+1 un+1
   2DUP S" |" COMPARE 0= 0= IF
     EXIT
   THEN
   2DROP PARSE-NAME
   BEGIN
     2DUP S" --" match-or-end? 0= WHILE
     2DUP S" :}" match-or-end? 0= WHILE
     ROT 1+ PARSE-NAME
     POSTPONE undefined-value
   AGAIN THEN THEN ;

: scan-end ( c-addr1 u1 -- c-addr2 u2 )
   BEGIN
     2DUP S" :}" match-or-end? 0= WHILE
     2DROP PARSE-NAME
   REPEAT ;

: define-locals ( c-addr1 u1 ... c-addrn un n -- )
   0 ?DO
     (LOCAL)
   LOOP
   0 0 (LOCAL) 
;

: {: ( -- )
   0 PARSE-NAME
   scan-args scan-locals scan-end
   2DROP define-locals
; IMMEDIATE

\ Reimplementation of ENVIRONMENT? adding locals

: ENVIRONMENT? ( c-addr u -- false | i*x true )
	2DUP S" #LOCALS" COMPARE 0= IF
		2DROP (MAX-LOCALS) -1
	ELSE
		ENVIRONMENT?
	THEN
;

