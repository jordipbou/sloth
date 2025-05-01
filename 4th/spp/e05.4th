\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 05

\ Modify the previous program such that only multiples of 
\ three or five are considered in the sum, 
\ e.g. 3, 5, 6, 9, 10, 12, 15 for n=17

: ask-number 
	0 0
	." Please, enter a number: " 
	pad 25 accept cr
	pad swap >number
	drop drop drop
;

: is-multiple?
	dup 3 mod 0= swap
	5 mod 0= or
;

: main
	ask-number
	0 swap 0 do
		i 1+ dup is-multiple? if + else drop then
	loop
	." The sum is " . cr
;

main
