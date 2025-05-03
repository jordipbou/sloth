\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 01

\ Write a function that returns the largest element in 
\ a list.

create list 10 , 11 , 14 , 8 , 24 , 26 , 31 , 47 , 42 , 9 ,

: largest ( addr n -- n )
	0 -rot \ starting point
	bounds do
		i @ 2dup < if nip else drop then	
		1 cells
	+loop
;

list 10 cells largest .
