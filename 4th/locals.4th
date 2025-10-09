\ SLOTH Implementation of Locals wordset

\ The word LOCALS| has not been implemented as is obsolescent.

REQUIRE THEFORTH.NET/STACK/1.0.0/STACK.4TH

\ The theforth.net stacks library does not include
\ a word to update any item in the stack, so I define
\ it here.

: PUT-STACK ( x n stack-id -- )
   2DUP DEPTH-STACK 0 SWAP WITHIN 0= IF -9 THROW THEN
   CELL+ SWAP CELLS + !
;	

\ Transient memory is used to compile the code of locals
\ and free that memory on semicolon.

REQUIRE TRANSIENT.4TH

GET-CURRENT INTERNAL-WORDLIST SET-CURRENT

16 CONSTANT (MAX-LOCALS)
(MAX-LOCALS) STACK CONSTANT (LOCALS-STACK)
VARIABLE (LOCALS-USED) 0 (LOCALS-USED) !

: >L ( x -- ) ( L: -- x ) (locals-stack) >stack ;
: L> ( -- ) (locals-stack) stack> drop ;

\ (LOCAL-DOES>) is executed when a local is found without
\ a TO.
\ It compiles code to access the local stack at the index
\ that the defined local points to.

: (LOCAL-DOES>)
	@ 
	(LOCALS-USED) @ 1- 
	SWAP - 
	POSTPONE LITERAL
	(LOCALS-STACK) POSTPONE LITERAL 
	POSTPONE PICK-STACK 
;

SET-CURRENT

\ (LOCAL) creates a new local on transient memory with the
\ string passed as the name, sets its DOES> to (LOCAL-DOES>)
\ and sets the flag IMMEDIATE on the new created word to
\ allow the word to compile itself thru DOES>
\ The newly created local is added to the locals wordlist.

\ If 0 0 is passed on the stack (no more locals), then
\ this word compiles code to move from the data stack to
\ the locals stack.

: (LOCAL) ( c-addr u -- )
	?DUP 0<> IF
		GET-CURRENT >R (LOCALS-WORDLIST) @ SET-CURRENT
		[: CREATE-NAME (LOCALS-USED) @ , ;] ON-TRANSIENT IMMEDIATE
		(LOCALS-USED) @ 1+ (LOCALS-USED) !
		['] (LOCAL-DOES>) (DOES)
		R> SET-CURRENT
	ELSE
		(LOCALS-USED) @ 0 ?DO
			POSTPONE >L
		LOOP
		DROP
	THEN
;

\ As locals here are implemented as a stack and not as values,
\ TO needs to be modified to work correctly with the locals
\ stack.

\ TO now searches for the word in the locals wordlist to 
\ differentiate locals from non locals. 
\ If its a local it compiles code to put data on the stack
\ at the index indicated by the value of the local.
\ If its not a local just duplicates original TO functionality,
\ although code is rewritten because now parsing has been done
\ before to check if the word is in locals wordlist.

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

GET-CURRENT INTERNAL-WORDLIST SET-CURRENT

\ This is the reference implementation of {: as seen on:
\ https://forth-standard.org/standard/locals/bColon

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

SET-CURRENT

: {: ( -- )
   0 PARSE-NAME
   scan-args scan-locals scan-end
   2DROP define-locals
; IMMEDIATE

\ Implementation of ENVIRONMENT? as required by locals wordset

: ENVIRONMENT? ( c-addr u -- false | i*x true )
	2DUP S" #LOCALS" COMPARE 0= IF
		2DROP (MAX-LOCALS) -1
	ELSE
		ENVIRONMENT?
	THEN
;

\ : :NONAME DOES> and ; have to be rewritten to allow 
\ the use of locals.

\ At start of a compilation with : :NONAME or DOES> ,
\ transient space is reserved for the locals-wordlist and
\ also to compile a new word for each local. A transient
\ marker is set.

\ That transient memory will be freed at the end of the
\ compilation, on ; or DOES> .

\ (COLON-TMARKER) is used to store the transient marker.

GET-CURRENT INTERNAL-WORDLIST SET-CURRENT

VARIABLE (COLON-TMARKER)

: ALLOT-LOCALS-WL ( -- ) 
	1 CELLS TALLOT DUP
	0 SWAP !
	(LOCALS-WORDLIST) ! 
;
: SET-TMARKER ( -- ) TMARK (COLON-TMARKER) ! ;

: FREE-LOCALS-WL ( -- ) 0 (LOCALS-WORDLIST) ! 0 (LOCALS-USED) ! ;
: RESET-TMARKER ( -- ) (COLON-TMARKER) @ TFREE ;

SET-CURRENT

: : ( -- TADDR ) SET-TMARKER ALLOT-LOCALS-WL : ; 
: :NONAME ( -- TADDR ) SET-TMARKER ALLOT-LOCALS-WL :NONAME ;
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

