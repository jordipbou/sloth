\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 08

\ Write a function on_all that applies a function to i
\ every element of a list. Use it to print the first 
\ twenty perfect squares. The perfect squares can be 
\ found by multiplying each natural number with itself. 
\ The first few perfect squares are 1*1= 1, 2*2=4, 3*3=9, 
\ 4*4=16. Twelve for example is not a perfect square 
\ because there is no natural number m so that m*m=12. 
\ (This question is tricky if your programming language 
\ makes it difficult to pass functions as arguments.)

: on-all ( addr n xt -- )
	-rot cells bounds do
		i @ over execute	
	1 cells +loop
;

create list 20 cells allot

: range ( addr n n -- )
	dup >r + r> do
		i over !
		cell+
	loop
;

list 20 1 range

: print-list ( addr n -- )
	cells bounds do 
		i @ .
	1 cells +loop
;

list 20 print-list cr
list 20 [: dup * . ;] on-all cr
