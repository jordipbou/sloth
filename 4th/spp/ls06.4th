\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Lists, Strings 06

\ Write a function that tests whether a string is a 
\ palindrome.

: is-palindrome? ( addr n -- )
	2dup 1- chars + swap 
	2 / 0 do 
		over c@ over c@ <> if
			0 unloop exit
		then
		1 chars - swap
		1 chars + swap
	loop
	-1
;

s" abba" 2dup type is-palindrome? space . cr
s" abbo" 2dup type is-palindrome? space . cr
s" abcba" 2dup type is-palindrome? space . cr
