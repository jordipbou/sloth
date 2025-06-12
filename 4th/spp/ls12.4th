\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 12

\ Write a function that rotates a list by k elements. 
\ For example [1,2,3,4,5,6] rotated by two becomes 
\ [3,4,5,6,1,2]. Try solving this without creating a 
\ copy of the list. How many swap or move operations 
\ do you need?

variable there
here unused + there !

: tallot ( n -- )
	unused over < if -8 throw then
	there @ swap - there !
;

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

create list 1 , 2 , 3 , 4 , 5 , 6 ,

: rotate* ( addr n n -- )
;

list 6 print-list
list 6 2 rotate* print-list
