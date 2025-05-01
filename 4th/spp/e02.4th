\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 02

\ Write a program that asks the user for their name and 
\ greets them with their name.

: ask-name
	." What's your name?" cr
	pad 25 accept cr
	pad swap
;

: greet ." Nice to meet you, " type cr ;

ask-name greet
