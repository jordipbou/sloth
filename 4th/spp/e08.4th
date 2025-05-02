\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 08

\ Write a program that prints all prime numbers. 
\ (Note: if your programming language does not support 
\ arbitrary size numbers, printing all primes up to the 
\ largest number you can easily represent is fine too.)

0 INVERT 1 RSHIFT constant MAX-INT

: is-prime? ( n -- )
	0 over 0 ?do
		over i 1+ mod 0= if 1+ then
		dup 2 > if leave then
	loop
	2 = 
	nip
;

: main ( -- )
	MAX-INT 1+ 2 do
		i is-prime? if
			i .
		then
	loop
;

main
