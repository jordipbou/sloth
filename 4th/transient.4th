variable THERE
here unused + there !

: (CHECK-TRANSIENT-SPACE) ( n -- n ) unused over < if -8 throw then ;
: (TALLOT) ( n -- taddr ) there @ swap - dup there ! ;

: TALLOT ( n -- taddr ) (check-transient-space) (tallot) ;
: TMARK ( -- addr ) there @ ;
: TFREE ( addr -- ) there ! ;

\ TODO Maybe just indicate how much space do you want
\ to use on transient?

\ Allocates 512 bytes on transient and sets HERE there to
\ allow any compilation done in the transient region.
: ON-TRANSIENT ( xt -- ) 
	HERE >R 
	512 TALLOT 
	HERE - ALLOT 
	CATCH 
	R> HERE - ALLOT \ Restore HERE even on exception
	THROW
;
