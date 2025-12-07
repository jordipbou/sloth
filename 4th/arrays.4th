\ Creating an array with this word stores the size of the
\ items in the word itself. 
: array: ( n sz -- ) ( i -- addr )
	create dup , * allot
	does> dup @ swap cell+ -rot * +
;


