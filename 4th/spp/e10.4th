\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 10

\ Write a program that prints the next 20 leap years.

: divisible? ( n1 n2 -- flag ) mod 0= ;

: leap-year? ( n -- flag )
	dup 4 divisible? if
		dup 100 divisible? if
			dup 400 divisible? if
				drop -1 exit
			else
				drop 0 exit
			then
		else
			drop -1 exit
		then
	else
		drop 0 exit
	then
;

: print-next-20-leap-years
	." The next 20 leap years are:" cr cr
	time&date nip nip nip nip nip
	0 begin
		dup 20 < while
		over leap-year? if over . cr 1+ then
		swap 1+ swap
	repeat
	cr
;

print-next-20-leap-years
