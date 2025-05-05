\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 09

\ Write a function that concatenates two lists. 
\ [a,b,c], [1,2,3] → [a,b,c,1,2,3]

variable there
here unused + there !

: tallot ( n -- )
	unused over < if -8 throw then
	there @ swap - there !
;

: concat ( addr1 n1 addr2 n2 -- addr3 n3 )
	rot 2dup + dup >r cells tallot -rot
	2swap there @ -rot cells bounds do
		i @ over !
		cell+
	1 cells +loop
	-rot cells bounds do
		i @ over !
		cell+
	1 cells +loop
	drop there @ r>
;

create list1 10 , 11 , 14 , 8 , 24 , 
create list2 26 , 31 , 47 , 42 , 9 , 36 ,

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

list1 5 print-list cr
list2 6 print-list cr
list1 5 list2 6 concat print-list cr
 
