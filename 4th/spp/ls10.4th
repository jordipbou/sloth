\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 10

\ Write a function that combines two lists by alternatingly
\ taking elements, e.g. [a,b,c], [1,2,3] → [a,1,b,2,c,3]

\ Helpers

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

\ Program

create list1 10 , 11 , 14 , 8 , 24 , 
create list2 26 , 31 , 47 , 42 , 9 ,

: combine ( addr1 addr2 n1 -- addr3 n2 )
	dup dup 2* >r cells tallot
	there @ swap
	0 do
		>r over @ r> dup cell+ >r !
		dup @ r> dup cell+ >r !
		cell+ swap cell+ swap
		r> 
	loop
	there @ r>	
;

list1 5 print-list cr
list2 5 print-list cr
list1 list2 5 combine print-list cr

