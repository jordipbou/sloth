\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 11

\ Write a function that merges two sorted lists into a 
\ new sorted list. [1,4,6],[2,3,5] → [1,2,3,4,5,6]. 
\ You can do this quicker than concatenating them followed 
\ by a sort.

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

: sort* ( addr n -- )
	cells bounds
	over -rot do
 		dup i cell+ ?do
			j @ i @ > if
				j @ 
				i @ j ! 
				i !
			then
 		1 cells +loop
 	1 cells +loop
 	drop
;

create list1 1 , 4 , 6 ,
create list2 2 , 3 , 5 ,

\ First option, concatenating and then sorting
: merge-sorted ( addr1 n1 addr2 n2 -- addr3 n3 )
	concat 2dup sort*
;

\ TODO doing this quicker, as indicated in the problem.

list1 3 list2 3 merge-sorted print-list
