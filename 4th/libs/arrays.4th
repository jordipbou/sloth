marker arrays

variable there
here unused + there !

: tallot ( n -- )
	unused over < if -8 throw then
	there @ swap - there !
;

variable tmarker

\ Compilation only
: tmark ( -- )
	r> tmarker @ >r >r
	there @ tmarker !
;

\ Compilation only
: tfree ( -- )
	tmarker @ there !
	r> r> tmarker ! >r
;

\ Array: an arrangement of items at equally spaced 
\ addresses in computer memory

\ In Forth, an array its represented by the address
\ of the first element and the number of elements.
\ The size of each element is implicit in the words
\ used (chars or cells).

\ That is a very good starting point for an array
\ library, one with words that allow both in place
\ modification and new array allocation.

\ That leaves us with four types of words:
\ 1. word -- does not modify the array
\ 2. word! -- in-place modification
\ 3. word> -- move/copy to already allocated space
\ 4. word* -- allocate new space in transient memory

\ -- Accessing and modification of elements ---------------

: get ( addr n -- n ) cells + @ ;
: cget ( c-addr n -- n ) chars + c@ ;

: set! ( n addr n -- ) cells + ! ;
: cset! ( char c-addr n -- ) chars + c! ;

\ -- Iteration over elements ------------------------------

variable iter-xt

\ Iteration over one array of cell sized elements
: iter ( addr n xt[ n -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	cells bounds do
		i @ iter-xt @ execute
	cell +loop
	r> iter-xt !
;

\ Iteration over one array of char sized elements
: citer ( c-addr n xt[ n -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	chars bounds do
		i c@ iter-xt @ execute
	1 chars +loop
	r> iter-xt !
;

variable 2iter-addr1
variable 2iter-addr2

\ Iteration over two arrays of cell sized elements
\ with the same size.
: 2iter ( addr1 addr2 n2 xt[ n1 n2 -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	-rot 2iter-addr2 @ >r 2iter-addr2 !
	2iter-addr1 @ >r 2iter-addr1 !
	0 do
		2iter-addr1 @ dup cell+ 2iter-addr1 ! @
		2iter-addr2 @ dup cell+ 2iter-addr2 ! @
		iter-xt @ execute
	loop
	r> 2iter-addr1 ! r> 2iter-addr2 ! r> iter-xt !
;

\ Iteration over two arrays of char sized elements
\ with the same size.
: 2citer ( c-addr1 c-addr2 n2 xt[ n1 n2 -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	-rot 2iter-addr2 @ >r 2iter-addr2 !
	2iter-addr1 @ >r 2iter-addr1 !
	0 do
		2iter-addr1 c@ dup char+ 2iter-addr1 ! @
		2iter-addr2 c@ dup char+ 2iter-addr2 ! @
		iter-xt @ execute
	loop
	r> 2iter-addr1 ! r> 2iter-addr2 ! r> iter-xt !
;

\ -- Iteration over addresses -----------------------------

\ Iteration over the addresses of each element of one
\ array with cell sized elements.
: addr-iter ( addr n xt[ addr -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	cells bounds do
		i iter-xt @ execute
	cell +loop
	r> iter-xt !
;

\ Iteration over the addresses of each element of one
\ array with char sized elements.
: caddr-iter ( c-addr n xt[ c-addr -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	chars bounds do
		i iter-xt @ execute
	1 chars +loop
	r> iter-xt !
;

\ Iteration over the addresses of two arrays with same
\ number of cell sized elements.
: addr-2iter ( addr1 addr2 n xt[ addr2 addr2 -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	-rot 2iter-addr2 @ >r 2iter-addr2 !
	2iter-addr1 @ >r 2iter-addr1 !
	0 do
		2iter-addr1 @ dup cell+ 2iter-addr1 !
		2iter-addr2 @ dup cell+ 2iter-addr2 !
		iter-xt @ execute
	loop
	r> 2iter-addr1 !
	r> 2iter-addr2 !
	r> iter-xt !
;

\ Iteration over the addresses of two arrays with same
\ number of char sized elements.
: caddr-2iter ( c-addr1 c-addr2 n xt[ c-addr2 c-addr2 -- i*x ] -- j*x )
	iter-xt @ >r iter-xt !
	-rot 2iter-addr2 @ >r 2iter-addr2 !
	2iter-addr1 @ >r 2iter-addr1 !
	0 do
		2iter-addr1 @ dup char+ 2iter-addr1 !
		2iter-addr2 @ dup char+ 2iter-addr2 !
		iter-xt @ execute
	loop
	r> 2iter-addr1 !
	r> 2iter-addr2 !
	r> iter-xt !
;

