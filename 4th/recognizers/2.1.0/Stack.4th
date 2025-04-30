\
\ separate stacks for cell sized data
\
\ Date: Nov 13, 2016
\ Author: Matthias Trute
\ License: Public Domain

\ allocate a stack region with at most 
\ size elements
: STACK ( size -- stack-id )
  1+ ( size ) CELLS HERE SWAP ALLOT
  0 OVER ! \ empty stack
;

\ replace the stack content with data from
\ the data stack.
: SET-STACK ( rec-n .. rec-1 n recstack-id -- )
  OVER 0< IF -4 THROW THEN \ stack underflow
  2DUP ! CELL+ SWAP CELLS BOUNDS 
  ?DO I ! CELL +LOOP
;

\ read the whole stack to the data stack
: GET-STACK ( recstack-id -- rec-n .. rec-1 n )
  DUP @ >R R@ CELLS + R@ BEGIN 
    ?DUP 
  WHILE
    1- OVER ( -- a n a ) 
          @ ( -- a n r_i) 
    ROT CELL -
    ROT ( -- r_i a n )
  REPEAT
  DROP R>
;

\ execute XT for earch element of the stack
\ leave the loop if the XT returns TRUE
: MAP-STACK ( i*x XT stack-id -- j*y f )
   DUP CELL+ SWAP @ CELLS BOUNDS ?DO 
     ( -- i*x XT )
     I @ SWAP DUP >R EXECUTE
     ?DUP IF R> DROP UNLOOP EXIT THEN
     R> CELL +LOOP 
   DROP 0
;

\ add an item as new top of the stack
: >STACK ( x stack-id -- )
  2DUP 2>R NIP GET-STACK 2R> ROT 1+ SWAP SET-STACK
;

\ destructivly get Top Of Stack
: STACK> ( stack-id -- x )
  DUP >R GET-STACK 1- R> ROT >R SET-STACK R>
;

\ actual stack depth
: DEPTH-STACK ( stack-id -- n )
  @
;

\ copy a stack item
: PICK-STACK ( n stack-id -- n' )
   2DUP DEPTH-STACK 0 SWAP WITHIN 0= IF -9 THROW THEN
   CELL+ SWAP CELLS + @
;

\ add an item at the bottom of a stack
: >BACK ( x stack-id -- )
  DUP >R GET-STACK 1+ R> SET-STACK
;

\ destructivly get Bottom Of Stack
: BACK> ( stack-id -- x )
  DUP >R GET-STACK 1- R> SET-STACK
;

