: 2LITERAL ( x1 x2 -- ) ( C: -- x1 x2 )
	SWAP POSTPONE LITERAL POSTPONE LITERAL
; IMMEDIATE

: 2CONSTANT ( x1 x2 "<spaces>name" -- ) ( -- x1 x2 )
	CREATE SWAP , , DOES> DUP @ SWAP CELL+ @
;

: 2VALUE ( x1 x2 "<spaces>name" -- ) ( -- x1 x2 )
	CREATE 2 , , , DOES> CELL+ 2@
;

: 2ROT ( d1 d2 d3 -- d2 d3 d1 )
	>R >R 2SWAP R> R> 2SWAP
;

: D0< ( d -- flag ) NIP 0< ;

: D0= ( d -- flag ) OR 0= ;

: D< ( d1 d2 -- flag )
	ROT SWAP
	2DUP < IF 2DROP 2DROP TRUE EXIT THEN
	= IF U< EXIT THEN
	2DROP FALSE
;

: D= ( d1 d2 -- flag ) D- OR 0= ;

: D2* ( d1 -- d2 ) 2DUP D+ ;

: D2/ ( d1 -- d2 )
	DUP 2/ >R
	SWAP 1 RSHIFT SWAP 1 AND NEGATE
	-1 1 RSHIFT 1+ AND OR
	R>
;

: DMAX ( d1 d2 -- d3 ) 2OVER 2OVER D< IF 2SWAP THEN 2DROP ;

: DMIN ( d1 d2 -- d3 ) 2OVER 2OVER D< IF 2DROP ELSE 2SWAP 2DROP THEN ;

: (UFLIP) ( ud -- ud' ) -1 1 RSHIFT 1+ XOR ;

: DU< ( ud1 ud2 -- flag )
	>R >R (UFLIP) R> R> (UFLIP) D<
;

VARIABLE (M*/N1)
VARIABLE (M*/N2)
VARIABLE (M*/SIGN)
VARIABLE (M*/P0)
VARIABLE (M*/P1)
VARIABLE (M*/P2)

: M*/ ( d1 n1 +n2 -- d2 )
	(M*/N2) !
	(M*/N1) !
	DUP 0< (M*/N1) @ 0< XOR (M*/SIGN) !
	DUP 0< IF DNEGATE THEN
	SWAP (M*/N1) @ ABS UM*
	SWAP (M*/P0) !
	SWAP (M*/N1) @ ABS UM*
	(M*/P2) !
	OVER +
	2DUP SWAP U<
	>R NIP
	(M*/P1) !
	(M*/P2) @ R> +
	(M*/P2) !
	(M*/P1) @ (M*/P2) @ (M*/N2) @ UM/MOD
	>R
	(M*/P0) @ SWAP (M*/N2) @ UM/MOD
	R> ROT DROP
	(M*/SIGN) @ IF DNEGATE THEN
;
