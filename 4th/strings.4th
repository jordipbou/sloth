?: -TRAILING ( c-addr u1 -- c-addr u2 )
?\		BEGIN   
?\		    2DUP + CHAR- C@ BL =
?\		    OVER AND
?\		WHILE   
?\		    CHAR-  
?\		REPEAT  
?\ ;

\ Implementation taken from SwapForth
?: CMOVE ( c-addr1 c-addr2 u -- )
?\		BOUNDS ROT >R
?\		BEGIN
?\		    2DUP XOR
?\		WHILE
?\		    R@ C@ OVER C!
?\		    R> 1+ >R
?\		    1+
?\		REPEAT
?\		R> DROP 2DROP
?\ ;

\ Implementation taken from SwapForth
?: CMOVE> ( ADDR1 ADDR2 U -- )
?\		BEGIN
?\			DUP
?\		WHILE
?\			1- >R
?\			OVER R@ + C@
?\			OVER R@ + C!
?\			R>
?\		REPEAT
?\		DROP 2DROP
?\ ;

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

\ Reference ANS Forth implementation from:
\ https://forth-standard.org/standard/string/UNESCAPE
?: UNESCAPE ( c-addr1 len1 c-addr2 -- c-addr2 len2 )
?\		DUP 2SWAP OVER + SWAP ?DO
?\			I C@ [CHAR] % = IF
?\				[CHAR] % OVER C! 1+
?\			THEN
?\			I C@ OVER C! 1+
?\		LOOP
?\		OVER -
?\ ;
