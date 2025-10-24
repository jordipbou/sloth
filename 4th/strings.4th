REQUIRE TRANSIENT.4TH

[UNDEFINED] -TRAILING [IF]
: -TRAILING ( c-addr u1 -- c-addr u2 )
	BEGIN   
	    2DUP + CHAR- C@ BL =
	    OVER AND
	WHILE   
	    CHAR-  
	REPEAT  
;
[THEN]

\ Implementation taken from SwapForth
[UNDEFINED] CMOVE> [IF]
: CMOVE> ( ADDR1 ADDR2 U -- )
	BEGIN
		DUP
	WHILE
		1- >R
		OVER R@ + C@
		OVER R@ + C!
		R>
	REPEAT
	DROP 2DROP
;
[THEN]

\ Implementation from:
\ https://forth-standard.org/standard/string/SEARCH#reply-1489
?: SEARCH  ( caddr1 u1 caddr2 u2 -- caddr3 u3 flag )
?\		2OVER                  \ retain a copy of 1st string
?\		BEGIN
?\			2 PICK OVER <=
?\		WHILE
?\			2OVER 3 PICK OVER COMPARE
?\		WHILE
?\			1 /STRING
?\		REPEAT
?\			2NIP 2NIP TRUE EXIT  \ string found
?\		THEN
?\		2DROP 2DROP FALSE      \ string not found
?\ ;

[UNDEFINED] UNESCAPE [IF]
	\ Reference ANS Forth implementation from:
	\ https://forth-standard.org/standard/string/UNESCAPE
	: UNESCAPE ( c-addr1 len1 c-addr2 -- c-addr2 len2 )
		DUP 2SWAP OVER + SWAP ?DO
			I C@ [CHAR] % = IF
				[CHAR] % OVER C! 1+
			THEN
			I C@ OVER C! 1+
		LOOP
		OVER -
	;
[THEN]

[UNDEFINED] REPLACES [IF]
	GET-CURRENT INTERNAL-WORDLIST SET-CURRENT

		\ Wordlist used to store replacements
		WORDLIST CONSTANT REPLACEMENTS-WL

		\ Default size for replacement buffers
		32 CONSTANT REPLACEMENTS-BUFSIZE

	SET-CURRENT

	: REPLACES ( c-addr1 u1 c-addr2 u2 -- )
		2DUP REPLACEMENTS-WL SEARCH-WORDLIST IF
			NIP NIP	
			EXECUTE
			2DUP ! \ Store new length
			CELL+ SWAP CMOVE \ Store the string
		ELSE
			\ String replacement word does not exist, create
			\ a new word for it on replacements-wl
			GET-CURRENT >R REPLACEMENTS-WL SET-CURRENT 
				CREATE-NAME
				DUP , \ Store the length
				HERE
				REPLACEMENTS-BUFSIZE ALLOT
				SWAP CMOVE \ Copy the contents
			R> SET-CURRENT
		THEN
	;
	
	\ As REPLACES and SUBSTITUTE are related, I don't check if
	\ SUBSTITUTE has already been defined.

	GET-CURRENT INTERNAL-WORDLIST SET-CURRENT

		VARIABLE SOURCE-ADDRESS
		VARIABLE SOURCE-LEN
		VARIABLE DEST-ADDRESS
		VARIABLE DEST-REM
		VARIABLE DEST-LEN
		VARIABLE SUBSTITUTIONS

		: GET-CHAR ( -- char )
			SOURCE-LEN 1-!
			SOURCE-ADDRESS @ DUP CHAR+ SOURCE-ADDRESS !
			C@
		;

		: WRITE-CHAR ( char -- )
			DEST-ADDRESS @ DUP CHAR+ DEST-ADDRESS !
			C!
			DEST-REM 1-!
			DEST-LEN 1+!
		;

		: WRITE-REPLACEMENT ( c-addr u -- )
			\ Check if it can be written !!!
			DEST-REM @ OVER - DEST-REM !
			DEST-LEN @ OVER + DEST-LEN !
			SUBSTITUTIONS 1+!
			DEST-ADDRESS @ SWAP CMOVE
		;

		: PARSE-SUBSTITUTION ( -- )
			SOURCE-ADDRESS @ DUP BEGIN
				SOURCE-LEN @ 0> WHILE
				GET-CHAR [CHAR] % = IF
					DUP SOURCE-ADDRESS @ SWAP - 1-
					2DUP REPLACEMENTS-WL SEARCH-WORDLIST IF
						2DROP
						EXECUTE
						DUP @ SWAP CELL+ SWAP
						WRITE-REPLACEMENT
					ELSE
						\ Not found, just copy string
					THEN
				THEN
			REPEAT
		;

		: SUBSTITUTE ( -- c-addr u n )
			BEGIN
				DEST-REM @ 0> WHILE
				SOURCE-LEN @ 0> WHILE
				GET-CHAR DUP [CHAR] % = IF
					DROP PARSE-SUBSTITUTION
				ELSE
					WRITE-CHAR
				THEN
			REPEAT THEN
		;

	SET-CURRENT

	: SUBSTITUTE ( source slen dest rem -- dest elen substs )
		DEST-REM !
		DEST-ADDRESS !
		SOURCE-LEN !
		SOURCE-ADDRESS !
		SUBSTITUTE
	;
[THEN]
