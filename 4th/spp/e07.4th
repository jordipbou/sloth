\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 07

\ Write a program that prints a multiplication table for 
\ numbers up to 12.

: print-multiplication-table ( n -- )
	13 1 do
		dup . ." * " i . ." = " dup i * . cr
	loop
;

: main ( -- )
	13 1 do
		i print-multiplication-table cr
	loop
;

main
