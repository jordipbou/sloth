\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 03

\ Write a function that checks whether an element occurs 
\ in a list.

create list 
10 , 11 , 14 , 8 , 24 , 26 , 31 , 47 , 42 , 9 ,

: find ( addr n n -- flag )
	-rot cells bounds do
		i @ over = if -1 unloop exit then
	1 cells +loop
	0
;

list 10 5 find .
list 10 24 find .
