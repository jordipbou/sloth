GET-CURRENT INTERNAL-WORDLIST SET-CURRENT

	: SEARCH-ORDER-XT>NAME ( xt -- nt -1 | 0 )
		CONTEXT CONTEXT #ORDER @ 1- CELLS + DO
			I @ @ 
			BEGIN
				DUP WHILE
				2DUP NAME>XT = IF	
					NIP -1 UNLOOP EXIT
				ELSE
					@
				THEN
			REPEAT DROP
		1 CELLS NEGATE +LOOP
		DROP 0
	;

	\ XT>NAME does not use the search-order, and it
	\ can not find NTs for primitives.
	: HEADER-XT>NAME ( xt -- nt -1 | 0 )
		DUP 0> IF
			10 0 DO 
				DUP I CELLS - @ 
				OVER = IF 
					I 1+ CELLS - 
					-1 UNLOOP EXIT
				THEN
			LOOP
			DROP 0
		ELSE
			DROP 0
		THEN
	;

	: XT>NAME ( xt -- nt -1 | 0 )
		\ First try with header-xt>name, its faster and
		\ can deduce nt from xt that are not in the search order
		DUP HEADER-XT>NAME DUP 0= IF
			\ If it can not be deduced, try with the
			\ search order
			DROP SEARCH-ORDER-XT>NAME
			\ TODO If it cannot be found, its a quotation
			\ probably
		ELSE
			ROT DROP
		THEN
	;

	: .RET ( -- )
	  	'[' EMIT RDEPTH 0 0 D.R ']' EMIT SPACE
	  	RDEPTH 0 > IF
	  		1 RDEPTH 1- DO
	  			I 1- RPICK .
	  		-1 +LOOP
	  	THEN
	;
	
	: TRACING-FUNCTION ( ip -- )
		>R
		.S ." :: " '[' EMIT RDEPTH 0 0 D.R ']' EMIT SPACE
		BASE @ HEX R@ . BASE !
		R@ @ ['] (LIT) = IF
			." LIT: " R@ CELL+ @ . 
		ELSE
			R@ @ XT>NAME IF
				NAME>STRING TYPE
			ELSE
				." NAME NOT FOUND"
			THEN
		THEN
		CR R> DROP
	;

SET-CURRENT

: TRACE: ( "<spaces>name<spaces>" -- )
	32 WORD FIND IF
		>R CR .S CR R>
		['] TRACING-FUNCTION DUP DUP DEBUG
	THEN
;
