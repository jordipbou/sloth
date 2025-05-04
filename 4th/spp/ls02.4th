\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 02

\ Write function that reverses a list, preferably in place.

create list
10 , 11 , 14 , 8 , 24 , 26 , 31 , 47 , 42 , 9 , 36 ,

: reverse ( addr n -- )
	2dup 1- cells + swap
	2 / 0 do 
		over @ over @ >r >r
		dup r> swap !
		over r> swap !
		1 cells - swap 1 cells + swap
	loop
;

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

list 11 print-list cr
list 11 reverse
list 11 print-list cr
