\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 04

\ Write a program that asks the user for a number n and 
\ prints the sum of the numbers 1 to n.

: ask-number 
	0 0
	." Please, enter a number: " 
	pad 25 accept cr
	pad swap >number
	drop drop drop
;

: main
	ask-number
	dup 0 swap 0 do
		i 1+ +		
	loop
	swap
	." The sum of 1.." . ." is " . cr
;

main
