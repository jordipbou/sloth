-1 trace!

: ?:		token find [ ilen ipos! ] [ 1 ipos! ] choose ;

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
?: REPEAT	postpone ?branch here - , ; immediate
?: AGAIN	postpone branch here - , ; immediate

?: WHILE	postpone if swap ; immediate
?: REPEAT	postpone again postpone then ; immediate

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
?: /SOURCE ibuf ipos chars + ;

?: LITERAL postpone LIT , ; immediate

?: CHR token drop c@ ;
?: [CHR] chr postpone literal ; immediate

\ Parse (advance input) to next )
?: ( [chr] ) to/char 2drop ; immediate

( Now we can use stack comments too )

?: ['] ' postpone literal ; immediate

?: CR 10 emit ;
?: TYPE [ dup c@ emit char+ ] times drop ;

?: VARIABLE	create 0 , ;
?: CONSTANT	create , does> @ ;

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

: ACCEPT
	over >r
	[
		key dup 10 = over 13 = or if drop leave then
		dup 127 = if
			drop i 0 > if
				i 1- i!
				1-
				8 emit 32 emit 8 emit
			then
		else
			dup emit
			over c!
			1+
		then
	]
	times
	r> swap over -
;

2 trace!
