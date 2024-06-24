\ Sloth is:
\ * (3) A typed array programming language on top of...
\ * (2) A (symbolic) string rewriting system on top of...
\ * (1) A typeless Forth implementation.

\ As this is implemented in Forth, we already have the first layer.

\ So, let's work on the next layer, a string rewriting system.

\ To start somewhere, let's investigate what a string rewriting system needs.
\ Some string rewriting system based programming languages:
\ * SNOBOL
\ * ICON
\ Another language where strings and parsing is really important:
\ * REBOL

\ Let's base our invertigations around the Icon and Rebol programming languages.

\ In both languages there's a process of parsing/scanning an string with a series of
\ rules.

\ As indicated in Icon documentation, it sets an environment
\ (saving the previous one in a stack) with the string to
\ be parsed and its position and functions operate on that
\ environment.
\ As previous environment is saved, they can be nested,
\ allowing working on substrings.

\ Let's try that:

\ variable "BUF
\ variable "POS
\ variable "LEN
\ 
\ : "SCAN
\ \ Sets a new environment to start scanning a string
\ 	"buf @ >r "pos @ >r "len @ >r
\ 	"len ! "buf ! 0 "pos !
\ ;
\ 
\ : "END
\ \ Ends the scanning environment (restoring the previous one)
\ 	r> "len !
\ 	r> "pos !
\ 	r> "buf !
\ ;

\ Ok. That was my first naive implementation. It will not
\ work interactively because its not possible to store to
\ r from the REPL.
\ That's a Forth problem for me. Because it breaks the
\ consistency.
\ Possible solutions:
\ * Create a new stack for storing data that its not the return stack
\ * Just leave it on the data stack and continue...
\ .. I think i will try this one for now.

\ variable PBUF
\ variable PPOS
\ variable PLEN
\ 
\ : S[ pbuf @ >r ppos @ >r plen @ >r plen ! pbuf ! 0 ppos ! r> r> r> ;
\ : S] plen ! ppos ! pbuf ! ;
\ 
\ : STRACE pbuf @ . ppos @ . plen @ . ;
\ 
\ : SMOVE ( n -- f )
\ \ Increments/decrements parsing pointer if 
\ \ destination position is inside the range between 0 and plen,
\ \ and returns TRUE.
\ \ If ppos+n lies outside the range between 0 and plen, 
\ \ does not modify ppos and returns FALSE.
\ 	ppos @ + dup 0 plen @ within if
\ 		ppos ! True
\ 	else
\ 		drop False
\ 	then
\ ;

\ Let's modify this, as failure (as used in the Icon
\ language) seems pretty important. Let's use exceptions
\ for that.

\ The idea searched now is:

\ <string> <rules> scan
\ where <rules> is a quotation

\ Example:
\ s" Hello" [: begin 1 smove drop 1 smove emit again ;] scan

\ Note: I see here that the use of quotations allows calling
\ words (combinators) that store data in the return stack
\ by themselves (without needing another word at the end)
\ and, as such, work perfectly well on the REPL.

variable PBUF
variable PPOS
variable PLEN

: PSCAN ( c-addr u xt -- )
	pbuf @ >r ppos @ >r plen @ >r
	>r plen ! 0 ppos ! pbuf ! r> catch drop
	r> plen ! r> ppos ! r> pbuf !
;

: PTRACE pbuf @ . ppos @ . plen @ . ;

: PMOVE ( n -- c-addr u )
	dup ppos @ + 0 plen @ ( n ppos+n 0 plen )
	within invert throw ( n | throw )
	pbuf @ ppos @ + ( n pbuf+ppos )
	over ( n pbuf+ppos n )
	rot ( pbuf+ppos n n )
	ppos @ + ( pbuf+ppos n ppos+n )
	ppos ! ( pbuf+ppos n )
;

: PMOVE\ ( n -- )
\ Just for the advancing and not printing case
	pmove drop
;

\ To have a more ergonomic quotation based Forth, it would
\ be interesting to use [ and ] as quotation markers (as in
\ Factor). That means changing ] [ from state control to
\ ]c i[

: [i [ ; immediate
: ]c ] ;

\ Now, [ and ] can be reused
\ This works in gForth

' int-[: ' comp-[: interpret/compile: [
: ] postpone ; swap execute ; immediate

\ Another helper combinator

: ENDLESS ( xt -- ) >r begin r@ execute again ;

\ Ok, this works, but as throw is used to end loops, 
\ there's no way to maintain things in stack if needed
\ for later use, let's analyze it later.
