\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 11

\ Write a program that computes the sum of an alternating 
\ series where each element of the series is an expression 
\ of the form ((−1)k+1)/(2*k−1) for each value of k from 
\ 1 to a million, multiplied by 4.

: main
	0E \ initial value for the sum
	1000001 1 do
		-1E i 1+ s>f f** \ -1^(k+1)
		2E i s>f f* 1E f- \ 2*k-1
		f/ \ (-1^(k+1))/(2*k-1)
		f+ \ add to the sum
	loop
	4E f*
	f. cr
;

main
