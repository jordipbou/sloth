[UNDEFINED] CS-PICK [IF]
	: CS-PICK ( C: destu ... orig0 | dest0 -- destu ... orig0 | dest0 destu ) ( S: u -- ) 
		PICK ;
[THEN]

[UNDEFINED] CS-ROLL [IF]
	: CS-ROLL ( C: origu | destu origu-1 | destu-1 ... orig0 | dest0 -- origu-1 | destu-1 ... orig0 | dest0 origu | destu ) ( S: u -- ) 
		ROLL ;
[THEN]

[UNDEFINED] DUMP [IF]
	\ Adapted from Minimal Forth
	GET-CURRENT INTERNAL-WORDLIST SET-CURRENT
	
		: .HEXDIGIT ( x -- )
			15 AND DUP 10 < IF 
				'0' + 
			ELSE 
				10 - 'A' + 
			THEN EMIT 
		;
		
		: .HEX ( x -- )
		 	DUP  4 RSHIFT .HEXDIGIT .HEXDIGIT
		;
	
		: .ADDR ( x -- )
		\ TODO Address 00 is not being printed correctly
		 	DUP 0= IF 
		 		DROP '0' EMIT '0' EMIT EXIT 
		 	THEN
		 	0 BEGIN ( x i ) 
		 		OVER WHILE 
		 		OVER 8 RSHIFT SWAP 1+ 
		 	REPEAT SWAP DROP
		 	BEGIN ( x i )
		 		DUP WHILE 
		 		SWAP .HEX 1- 
		 	REPEAT DROP 
		;
	
		16 CONSTANT B/LINE
	
		\ For all systems where char is one byte this definition
		\ is enough. For the systems where char is not one byte,
		\ it should be defined in host.
		: B@ ( addr -- byte ) C@ ;
	
		: .H ( addr len -- )
		 	B/LINE MIN DUP >R
		 	BEGIN ( addr len )
		 		DUP	WHILE ( addr len )
		 		OVER B@ .HEX SPACE
		 		1- SWAP 1+ SWAP
		 	REPEAT 2DROP
		 	B/LINE R> - 3 * SPACES
		;
		
		: .A ( addr1 len1 -- )
		 	B/LINE MIN
		 	BEGIN ( addr len )
		 		DUP WHILE
		 		OVER B@ DUP BL < IF DROP '.' THEN EMIT
		 		1- SWAP 1+ SWAP
		 	REPEAT 2DROP 
		;
	
		: DUMP-LINE ( addr len1 -- addr len2 )
		 	OVER .ADDR ':' EMIT SPACE 2DUP .H SPACE SPACE 2DUP .A
		 	DUP B/LINE MIN /STRING
		;
	
	SET-CURRENT
	
	: DUMP ( addr len -- )
	 	BEGIN
	 		DUP WHILE ( addr len )
	 		CR DUMP-LINE
	 	REPEAT 2DROP 
	 	CR
	;
[THEN]

[UNDEFINED] NAME>COMPILE [IF]
	: NAME>COMPILE ( nt -- x xt )
		DUP IMMEDIATE? IF
			NAME>XT ['] EXECUTE	
		ELSE
			NAME>XT ['] COMPILE,
		THEN
	;
[THEN]

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

[UNDEFINED] SEE [IF]
	\ TODO Make it better to take into account literals, rips,
	\ quotations and strings.
	
	\ PLATFORM DEPENDENT
	\ SEE fails to print quotations (and lots of other things)
	
	REQUIRE DEBUG.4TH
	
	: SEE ( "<spaces>name" -- )
	 	BL WORD DUP FIND
	 	DUP IF
			CR
	 		ROT ." NAME: " COUNT TYPE CR
	 		0< IF ." NON " THEN ." IMMEDIATE" CR
	 		DUP ." XT: " . CR
	 		DUP 0> IF
	 			BEGIN
	 				DUP @ ['] EXIT <> WHILE
	 				DUP @ DUP XT>NAME IF 
						NIP NAME>STRING TYPE 
					ELSE
						.
					THEN
					."  "
	 				CELL+
	 			REPEAT
	 			." ;" CR
	 		ELSE
	 			DROP
	 		THEN
	 	ELSE
	 		." WORD NOT FOUND" CR DROP DROP DROP
	 	THEN
	;
[THEN]

[UNDEFINED] SYNONYM [IF]
	: SYNONYM ( "<spaces>newname" "<spaces>oldname" -- )
		CREATE IMMEDIATE
			' ,	
		DOES>
			@ STATE @ 0= OVER IMMEDIATE? OR
			IF EXECUTE ELSE COMPILE, THEN
	;
[THEN]

[UNDEFINED] WORDS [IF]
	GET-CURRENT INTERNAL-WORDLIST SET-CURRENT
	
		: DISCARD ( x1 ... xn u -- ) 0 ?DO DROP LOOP ;
	
	SET-CURRENT
	
	: WORDS ( -- )
	 	GET-ORDER SWAP >R 1- DISCARD R>
	 	[: NAME>STRING TYPE SPACE TRUE ;] SWAP 
	 	CR TRAVERSE-WORDLIST
	;
[THEN]

[UNDEFINED] ? [IF]
	: ? ( a-addr -- ) @ . ;
[THEN]
