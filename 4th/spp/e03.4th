\ Simple Programming Problems
\ https://adriann.github.io/programming_problems.html

\ Elementary 03

\ Modify the previous program such that only the users 
\ Alice and Bob are greeted with their names.

: ask-name
	." What's your name?" cr
	pad 25 accept cr
	pad swap
;

: greet ." Nice to meet you, " type cr ;

\ In the previous programs there was no main word,
\ but as we use IF THEN here, and those are 
\ compile only words, we need to put this code
\ inside a word.

: main
	ask-name 
	2dup s" Alice" compare 0= if greet exit then
	2dup s" Bob" compare 0= if greet exit then
	2drop ." Who the hell are you?" cr
;

main
