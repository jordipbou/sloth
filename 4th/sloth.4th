-1 trace!

: ?:		parse-name find-name [: ilen ipos! ;] [: 1 ipos! ;] choose ;

?: \		ilen ipos! ; immediate

\ End of line comments can be used now.

\ ?: (defined above) allows conditional compilation 
\ of one line definitions.

\ Basic stack shuffling (that don't need conditionals)

?: DUP		0 pick ;
?: OVER		1 pick ;

?: 2DROP	drop drop ;
?: 2DUP		over over ;

\ Basic control structures

?: AHEAD	postpone branch here 0 , ; immediate
?: IF		postpone ?branch here 0 , ; immediate
?: THEN		here over - swap ! ; immediate

?: ELSE		postpone ahead swap postpone then ; immediate

?: BEGIN	here ; immediate
?: UNTIL	postpone ?branch here - , ; immediate
?: AGAIN	postpone branch here - , ; immediate

?: WHILE	postpone if swap ; immediate
?: REPEAT	postpone again postpone then ; immediate

\ Stack shuffling with conditionals

?: ?DUP		dup if dup then ;

\ Basic comparisons

?: <=		> invert ;
?: >=		< invert ;
?: 0=		if 0 else 0 invert then ;
?: 0<		0 < ;
?: 0>		0 > ;
?: 0<>		0= 0= ;
?: NOT		0= ;
?: =		- 0= ;
?: <>		= invert ;
?: U<		2dup xor 0< if swap drop 0< else - 0< then ;
?: U>		swap u< ;
?: WITHIN	over - >r - r> u< ; 

\ Basic arithmetic

?: 1+		1 + ;
?: 1-		1 - ;

\ Basic memory access

?: CELL+	1 cells + ;
?: CHAR+	1 chars + ;

\ /SOURCE allows easy access to current character
?: /SOURCE	ibuf ipos chars + ;

?: LITERAL	postpone LIT , ; immediate

?: CHAR		parse-name drop c@ ;
?: [CHAR]	char postpone literal ; immediate

\ Parse (advance input) to next )
?: (		[char] ) parse 2drop ; immediate

( Now we can use stack comments too )

?: [']		' postpone literal ; immediate

?: CR		10 emit ;
?: TYPE		[: dup c@ emit char+ ;] times drop ;

?: VARIABLE	create 0 , ;
?: CONSTANT	create , does> @ ;

\ --

?: SEARCH-WORDLIST	find-name-in dup if dup nt>xt swap immediate? if 1 else -1 then then ;

\ -- Conditional compilation ------------------------------
 
?: [DEFINED]	parse-name find-name 0<> ;
?: [UNDEFINED]	parse-name find-name 0= ;

\ Implementation of [IF] [ELSE] [THEN] by ruv, as seen in:
\ https://forth-standard.org/standard/tools/BracketELSE
 
wordlist dup constant BRACKET-FLOW-WL get-current swap set-current
: [IF]			1+ ;
: [ELSE]		dup 1 = if 1- then ;
: [THEN]		1- ;
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
 
: [THEN]	; immediate 
 
: [IF]		0= if postpone [else] then ; immediate


 
\ variable dst variable max variable k variable len
\ : ACCEPT max ! dst ! 0 k ! 0 len !
\ 	s" On accept!" type
\ 	begin
\ 		len @ max @ < while
\ 		key dup 10 = over 13 = or if len @ exit then
\ 		dup 127 = if
\ 			len @ 0 > if
\ 				len @ 1- len !
\ 				8 emit 32 emit 8 emit
\ 			then
\ 		else
\ 			dup emit
\ 			dst @ len @ chars + c!
\ 			len @ 1+ len !
\ 		then
\ 	repeat
\ ;

\ : ACCEPT
\	over >r
\	[:
\		key dup 10 = over 13 = or if drop leave then
\		dup 127 = if
\			drop i 0 > if
\				i 1- i!
\				1-
\				8 emit 32 emit 8 emit
\			then
\		else
\			dup emit
\			over c!
\			1+
\		then
\	;]
\	times
\	r> swap over -
\ ;

2 trace!
