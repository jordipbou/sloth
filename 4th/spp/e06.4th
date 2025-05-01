\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 06

\ Write a program that asks the user for a number n and 
\ gives them the possibility to choose between computing 
\ the sum and computing the product of 1,…,n.

: ask-number 
	0 0
	." Please, enter a number: " 
	pad 25 accept cr
	pad swap >number
	drop drop drop
;

: ask-operation
	." Do you want me to compute:" cr
	." 1. the sum of 1..n" cr
	." 2. the product of 1..n" cr
	key
;

: main
	ask-number dup
	ask-operation
	49 = if
		0 swap 0 do
			i 1+ +	
		loop
		swap
		." The result of applying the sum to 1.." .
		." is " . cr
	else
		1 swap 0 do
			i 1+ *
		loop
		swap
		." The result of applying the product to 1.." .
		." is " . cr
	then
;

main
