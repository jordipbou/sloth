-1 trace!

: ?: token find [ ilen ipos! ] [ 1 ipos! ] choose ;

?: \ ilen ipos! ; immediate

\ End of line comments can be used now.

\ ?: (defined above) allows conditional compilation 
\ of one line definitions.

\ List of required primitives:
\ Stack: DROP SWAP >R R> R@
\ Comparison: < = >
\ Arithmetic: + - * / MOD
\ Bitwise: LSHIFT RSHIFT AND OR XOR INVERT
\ Memory: ! @ C! C@ CHAR CELL HERE HERE!
\ Dictionary: FIND-NAME XT
\ Parsing: TOKEN IBUF IPOS ILEN IBUF! IPOS! ILEN! REFILL

\ -- TODO Shouldn't this be directly in CORE
?: '		token find xt ; \ get xt of word by name
\ TODO ?: [']		' postpone literal ; immediate
?: ''		token find ;	\ get nt of word by name

?: ALLOT	here + here! ;
?: CELLS	cell * ;
?: CHARS	char * ;

?: ,		here ! cell allot ;
?: C,		here c! char allot ;

\ -- Stack shufflers --

?: DUP		>r r@ r> ;
?: OVER		>r dup r> swap ;
?: NIP		swap drop ;
?: 2DUP		over over ;
?: ROT		>r swap r> swap ;
?: -ROT		rot rot ;

\ -- Conditionals --

?: 0<		0 < ;
?: 1<		1 < ;
?: <		2dup xor 0< [ drop 0< ] [ - 0< ] choose ; 
?: >		swap < ;
?: <=		> invert ;
?: >=		< invert ;
?: 0=		[ 0 ] [ 0 invert ] choose ;
?: 0>		0 > ;
?: 0<>		0= 0= ;
?: NOT		0= ;
?: =		- 0= ;
?: <>		= invert ;
?: U<		2dup xor 0< [ swap drop 0< ] [ - 0< ] choose ;
?: U>		swap u< ;
?: WITHIN	over - >r - r> u< ;

?: SOURCE	ibuf ilen ;
?: /SOURCE	ibuf ipos char * + ;

\ -- Logic --

?: NOR		or invert ;

\ -- Input output --

?: CR		10 emit ;

?: PAD		256 here + ;

: READLINE 
	over >r
	0 \ ( dst max len -- )
	[
		2dup <= ?leave \ ( dst max len -- )
		key dup 10 = over 13 = or [ drop ] ?call/leave \ ( dst max len key -- )
		dup 127 = [
			drop
			dup 0 > [ \ ( dst max len -- )
				1 -
				rot 1 - -rot
				8 emit 32 emit 8 emit
			] when
		] [
			dup emit
			>r rot r> over c! 1 + -rot
			1 +
		] choose
	]
	loop
	nip nip r> swap
;
1 trace!

