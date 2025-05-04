\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 04

\ Write a function that returns the elements on odd 
\ positions in a list.

\ Transient memory allocator

variable there
here unused + there !

: tallot ( n -- )
	unused over < if -8 throw then
	there @ swap - there !
;

create list
10 , 11 , 14 , 8 , 24 , 26 , 31 , 47 , 42 , 9 , 36 ,

variable j
: odd-elements ( addr1 n1 -- addr2 n2 )
	dup 2 / dup >r tallot
	0 j !
	0 do 
		i 2 mod if
			dup i cells + @
			there @ j @ cells + !
			j 1+!
		then
	loop drop
	there @ r>
;

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

list 11 print-list cr
list 11 odd-elements print-list cr
