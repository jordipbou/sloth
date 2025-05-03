\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 09

\ Write a guessing game where the user has to guess a 
\ secret number. After every guess the program tells the 
\ user whether their number was too large or too small. 
\ At the end the number of tries needed should be printed. 
\ It counts only as one try if they input the same number 
\ multiple times consecutively.

\ Random number generation taken from:
\ https://excamera.com/sphinx/article-xorshift.html

variable seed
time&date * * * * * seed !

: random    ( -- x )  \ return a 32-bit random number x
    seed @
    dup 13 lshift xor
    dup 17 rshift xor
    dup 5  lshift xor
    dup seed !
;

10 constant MAX-GUESSING-VALUE

variable SECRET-NUMBER
variable TRIES
variable GUESSED-NUMBER

: get-secret-number
	random MAX-GUESSING-VALUE mod abs 1+
	secret-number !
;

: ask-number
	." Enter your guess (1-10): " cr
	0 0 pad 25 accept cr pad swap 
	>number drop drop drop
	dup guessed-number @ <> if
		tries @ 1+ tries !	
	then
	guessed-number !
;

: check-number
	secret-number @ guessed-number @ = if
		." Congratulations, you needed " 
		tries @ . 
		." tries." cr
		bye
	else
		secret-number @ guessed-number @ < if
			." You guessed too high." cr
		else
			." You guessed too low." cr
		then
	then
;

: main
	0 tries !
	get-secret-number
	begin
		ask-number
		-1 while
		check-number
\		incorrect-guess? while
	repeat
;

main
