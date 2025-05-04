\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 07

\ Write three functions that compute the sum of the 
\ numbers in a list: using a for-loop, a while-loop 
\ and recursion. (Subject to availability of these 
\ constructs in your language of choice.)

create list
10 , 11 , 14 , 8 , 24 , 26 , 31 , 47 , 42 , 9 , 36 ,

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

: for-sum ( addr n -- n )
	0 -rot
	cells bounds do
		i @ +
	1 cells +loop
;

: while-sum ( addr n -- n )
	0 swap 0 begin
		2dup > while
		1+ 2>r over @ +
		swap cell+ swap
		2r>
	repeat
	drop drop nip	
;

: do-rec-sum ( n addr n -- n )
	?dup if
		over @ -rot
		1- swap cell+ swap
		recurse
		+
	else
		drop
	then
;

: rec-sum ( addr n -- n )
	0 -rot do-rec-sum
;

list 11 print-list cr
." for-loop based sum: " list 11 for-sum . cr
." while-loop based sum: " list 11 while-sum . cr
." recursion based sum: " list 11 rec-sum . cr
