\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 05

\ Write a function that computes the running total of 
\ a list.

create list
10 , 11 , 14 , 8 , 24 , 26 , 31 , 47 , 42 , 9 , 36 ,

: running-total ( addr n -- n )
	0 -rot
	cells bounds do
		i @ +	
	1 cells +loop
;

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

list 10 print-list
list 10 running-total .
