: ?: parse-name find-name [: source >in ! drop ;] [: 1 >in ! ;] choose ;

?: \	source >in ! drop ; immediate

\ Now that we can do end-of-line comments, let's comment the first line on top:
\ ?: allows conditional compilation of words with one line definitions only.

\ SLOTH is a Forth implementation that tries to implement as much of words as
\ possible in Forth itself, allowing easier implementation and porting of the
\ core words.

\ The current required set of primitives is:

\ EXIT
\ LIT BLOCK STRING
\ EXECUTE THROW CATCH
\ DROP SWAP PICK
\ >R R>
\ @ ! C@ C!
\ - */MOD
\ AND OR INVERT
\ : ; POSTPONE IMMEDIATE [: ;] [ [[ ]] ] :NONAME [: ;]
\ PARSE-NAME SOURCE >IN 
\ FIND-NAME '
\ CHOOSE DOLOOP LEAVE I J K
\ STATE BASE LATEST CURRENT

\ The next section defines enough words to allow for parsing, one of the most
\ fundamental activities to have a complete Forth environment.

\ -- Basic stack shuffling ----------------------------------------------------

?: DUP			0 pick ;					\ ( x -- x x )
?: OVER			1 pick ;					\ ( x1 x2 -- x1 x2 x1 )
?: TUCK			swap over ;					\ ( x1 x2 -- x2 x1 x2 )
?: NIP			swap drop ;					\ ( x1 x2 -- x2 )

\ -- Basic memory -------------------------------------------------------------

?: CHAR			1 chars ;					\ ( -- u )

?: 1+!			dup @ 1 + swap ! ;			\ ( a-addr -- ) non ANS

\ HOT (here-or-there) allows compilation to transient regions
?: HOT			state @ 0 < [: there ;] [: here ;] choose ;	\ ( -- n )

\ -- Basic control structures -------------------------------------------------

?: AHEAD		postpone branch hot 0 , ; immediate
?: IF			postpone ?branch hot 0 , ; immediate
?: THEN			hot over - swap ! ; immediate

?: ELSE			postpone ahead swap postpone then ; immediate

?: BEGIN		hot ; immediate
?: UNTIL		postpone ?branch hot - , ; immediate
?: AGAIN		postpone branch hot - , ; immediate

?: WHILE		postpone if swap ; immediate
?: REPEAT		postpone again postpone then ; immediate

\ -- Basic comparisons --------------------------------------------------------

?: <>			= invert ;					\ ( x1 x2 -- flag )
?: 0=			if 0 else 0 invert then ;	\ ( x -- flag )
?: 0<>			0= 0= ;						\ ( n -- flag )
?: 0<			0 < ;						\ ( n -- flag )

\ -- Basic arithmetic ---------------------------------------------------------

?: +			0 swap - - ;				\ ( x1 x2 -- x3 )

\ -- Parsing (required by stack comments) -------------------------------------

?: /STRING		tuck - >r chars + r> ;		\ ( c-addr1 u1 n -- c-addr2 u2 )

\ Puts on stack the non parsed area of source
?: /SOURCE		source >in @ /string dup 0< if drop 0 then ; \ ( -- c-addr u )

?: <P			/source ; \ Start parsing (similar to <# for numberic output)
?: *P			>r begin /source nip while /source drop c@ r@ execute while >in 1+! repeat then r> drop ;
?: P>			/source nip dup >r - r> if >in 1+! then ; \ End parsing

?: PARSE		<p rot [: over <> ;] *p drop p> ;

\ ( "ccc<paren>" -- )
?: (			41 parse drop drop ; immediate 

\ Now we can use stack comments too !!

\ -- Conditional compilation --------------------------------------------------

?: [DEFINED] ( "<spaces>name ... " -- flag ) parse-name find-name 0<> ;
?: [UNDEFINED] ( "<spaces>name ... " -- flag ) parse-name find-name 0= ;

\ -- More stack shuffling words -----------------------------------------------

?: ROT ( x1 x2 x3 -- x2 x3 x1 ) >r swap r> swap ;
?: -ROT ( x1 x2 x3 -- x3 x1 x2 ) rot rot ;
?: ?DUP ( x -- 0 | x x ) dup if dup then ;

?: 2DUP ( x1 x2 -- x1 x2 x1 x2 ) over over ;
?: 2DROP ( x1 x2 -- ) drop drop ;
?: 2OVER ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 ) 3 pick 3 pick ;
?: 2SWAP ( x1 x2 x3 x4 -- x3 x4 x1 x2 ) rot >r rot r> ;

?: 3DROP ( x x x -- ) drop drop drop ;

?: R@ ( -- x ) ( R: x -- x )  r> r> dup >r swap >r ;

?: 2>R ( x1 x2 -- ) ( R: -- x1 x2 ) ]] swap >r >r [[ ; immediate
?: 2R> ( -- x1 x2 ) ( R: x1 x2 -- ) ]] r> r> swap [[ ; immediate
?: 2R@ ( -- x1 x2 ) ( R: x1 x2 -- x1 x2 ) ]] 2r> 2dup 2>r [[ ; immediate

?: 2ROT ( x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2 ) 2>r 2swap 2r> 2swap ;

\ -- More arithmetic words ----------------------------------------------------

?: + ( x1 x2 -- x3 ) 0 swap - - ;
?: * ( x1 x2 -- x3 ) 1 */mod swap drop ;
?: / ( x1 x2 -- x3 ) 1 swap */mod swap drop ;
?: */ ( x1 x2 -- x3 ) */mod swap drop ;
?: MOD ( x1 x2 -- x3 ) 1 swap */mod drop ;
?: /MOD ( x1 x2 -- x3 ) 1 swap */mod ;
?: 2* ( x1 x2 -- x3 ) dup + ;

?: 1- ( x1 -- x2 ) 1 - ;
?: 1+ ( x1 -- x2 ) 1 + ;
?: 2+ ( x1 -- x2 ) 2 + ;

?: NEGATE ( x1 -- x2 ) invert 1+ ;

\ -- Bit ----------------------------------------------------------------------
\ 
?: OR ( x1 x2 -- x3 ) invert swap invert and invert ;
?: XOR ( x1 x2 -- x3 ) over over invert and >r swap invert and r> or ;

\ -- More comparisons ---------------------------------------------------------

?: < ( n1 n2 -- flag ) 2dup xor 0< if drop 0< else - 0< then ;
?: > ( n1 n2 -- flag ) swap < ;
?: <= ( n1 n2 -- flag ) > invert ;
?: >= ( n1 n2 -- flag ) < invert ;
?: 0> ( n -- flag ) 0 > ;
?: = ( x1 x2 -- flag ) - 0= ;
?: <> ( x1 x2 -- flag ) = invert ;
?: U< ( u1 u2 -- flag ) 2dup xor 0< if swap drop 0< else - 0< then ;
?: U> ( u1 u2 -- flag ) swap u< ;
?: WITHIN ( n1 | u1 n2 | u2 n3 | u3 -- flag ) over - >r - r> u< ; 
   
?: MIN ( n1 n2 -- n3 ) 2dup swap < if swap then drop ;
?: MAX ( n1 n2 -- n3 ) 2dup < if swap then drop ;

\ -- More memory words --------------------------------------------------------

?: CHAR ( -- u ) 1 chars ;
?: CELL ( -- u ) 1 cells ;
?: CHAR+ ( c-addr1 -- c-addr2 ) 1 chars + ;
?: CELL+ ( a-addr1 -- a-addr2 ) 1 cells + ;
?: CELL- ( a-addr1 -- a-addr2 ) 1 cells - ;

\ TODO , and C, are not taking into account when state < 0
?: , ( x -- ) here ! 1 cells allot ;
?: C, ( char -- ) here c! 1 chars allot ;
 
?: +! ( n | u a-addr -- ) swap over @ + swap ! ;
?: 0! ( a-addr -- ) 0 swap ! ;
?: 1+! ( a-addr -- ) dup @ 1+ swap ! ;
?: 1-! ( a-addr -- ) dup @ 1- swap ! ;

?: 2! ( x1 x2 a-addr -- ) swap over ! cell+ ! ;
?: 2@ ( a-addr -- x1 x2 ) dup cell+ @ swap @ ;

?: BOUNDS ( addr u -- u u ) over + swap ;

?: MOVE ( addr1 addr2 u -- ) rot swap chars bounds [: i c@ over c! char+ char ;] doloop drop ;

\ -- Constants and variables --------------------------------------------------

?: CONSTANT ( x "name" -- ) create , does> @ ;
?: VARIABLE ( "name" -- ) create 0 , ;

?: 2CONSTANT ( x1 x2 "name" -- ) create , , does> 2@ ;
?: 2VARIABLE ( "name" -- ) create 0 , 0 , ;

?: CONSTANT? >in @ >r [undefined] if r> >in ! constant else r> 2drop postpone \ then ;
?: VARIABLE? >in @ >r [undefined] if r> >in ! variable else r> drop postpone \ then ;

\ -- true / false / on / off --------------------------------------------------

0				constant? FALSE
false invert	constant? TRUE

?: ON ( a-addr -- ) true swap ! ;
?: OFF ( a-addr -- ) false swap ! ;

\ -- Compilation --------------------------------------------------------------

variable? LAST-STATE

?: [ ( -- ) state @ last-state ! 0 state ! ; immediate
?: ] ( -- ) last-state @  state ! ; immediate

?: LITERAL ( C: x -- ) ( -- x ) postpone lit , ; immediate 
?: 2LITERAL ( C: x1 x2 -- ) ( -- x1 x2 ) swap postpone literal postpone literal ; 

?: ['] ( C: "<spaces>name" -- ) ( -- xt ) ' postpone literal ; immediate 

?: SLITERAL ( C: c-addr u -- ) ( -- c-addr u ) postpone string dup , here swap move align ; immediate

\ -- Do/Loop ------------------------------------------------------------------

?: DO		postpone [: ; immediate
?: ?DO		postpone [: ; immediate
?: LOOP		postpone lit 1 , postpone ;] postpone doloop ; immediate
?: +LOOP	postpone ;] postpone doloop ; immediate

?: UNLOOP	; \ Unloop is a noop in this implementation

\ -- Stack shuffling combinators ----------------------------------------------

?: DIP ( i*x n xt -- j*x n ) swap >r execute r> ;
?: 2DIP ( n1 n2 xt -- n1 n2 ) swap >r swap >r execute r> r> ;
?: 3DIP ( n1 n2 n3 xt -- n1 n2 n3 ) swap >r swap >r swap >r execute r> r> r> ;
?: 4DIP ( n1 n2 n3 n4 xt -- n1 n2 n3 n4 ) swap >r swap >r swap >r swap >r execute r> r> r> r> ;

?: KEEP ( n xt[ n -- i*x ] -- i*x n ) over >r execute r> ;
?: 2KEEP ( n1 n2 xt[ n1 n2 -- i*x ] -- i*x n1 n2 ) 2 pick 2 pick >r >r execute r> r> ;
?: 3KEEP ( n1 n2 n3 xt[ n1 n2 n3 -- i*x ] -- i*x n1 n2 n3 ) 3 pick 3 pick 3 pick >r >r >r execute r> r> r> ;
 
?: KEEP-XT ( i*x xt[ i*x -- j*x]  -- j*x xt ) >r r@ execute r> ;

?: BI@ ( x1 x2 xt[ x -- i*x ] -- i*x1 i*x2 ) swap >r >r r@ execute r> r> swap execute ;

\ -- Looping combinators ------------------------------------------------------

?: TIMES ( n xt -- ) swap dup 0> if 0 [: keep-xt 1 ;] doloop else drop then drop ;
 
?: ITER ( addr u xt -- ) -rot cells bounds [: >r i @ r@ execute r> cell ;] doloop drop ;
?: ITER/ADDR ( addr u xt -- ) swap [: dup >r keep cell+ r> ;] times 2drop ;
?: -ITER/ADDR ( addr u xt -- ) over >r >r 1- cells + r> r> [: dup >r keep cell- r> ;] times 2drop ;
?: CITER ( c-addr u xt -- ) -rot chars bounds [: >r i c@ r@ execute r> char ;] doloop drop ;

\ TODO Implement:
\ 
\ ZIP 
\ *ZIP
\ CZIP
\ *CZIP
 
\ -- Tools for accessing the dictionary ---------------------------------------

4 constant? IMMEDIATE-FLAG
2 constant? COLON-FLAG
1 constant? HIDDEN-FLAG

?: NT>LINK ( nt1 -- nt2 ) @ ;
?: NT>XT ( nt -- xt ) 1 cells + @ ;
?: NT>DT ( nt -- a-addr ) 2 cells + @ ;
?: NT>WORDLIST ( nt -- wid ) 3 cells + @ ;
?: NT>FLAGS ( nt -- n ) 4 cells + c@ ;
?: NT>NAME ( nt -- c-addr u ) 4 cells + 1 chars + dup c@ swap 1 chars + swap ;

?: HAS-FLAG? ( nt n -- flag ) swap nt>flags over and = ;

?: IMMEDIATE? ( nt -- flag ) immediate-flag has-flag? ;
?: COLON? ( nt -- flag ) colon-flag has-flag? ;
?: HIDDEN? ( nt -- flag ) hidden-flag has-flag? ;

\ -- Wordlists ----------------------------------------------------------------

variable? LAST-WORDLIST 1 last-wordlist !

?: WORDLIST ( -- wid ) last-wordlist @ 1+ dup last-wordlist ! ;

?: GET-CURRENT ( -- wid ) current @ ;
?: SET-CURRENT ( wid -- ) current ! ;

?: GET-ORDER ( -- widn ... wid1 n ) order @ dup cell- @ dup >r ['] noop iter r> ;
?: SET-ORDER ( widn ... wid1 n -- ) dup order @ cell- ! order @ swap [: ! ;] -iter/addr ;
?: +ORDER ( wid -- ) >r get-order 1+ r> swap set-order ;
?: -ORDER ( -- ) get-order nip 1- set-order ;

\ -- Finding definitions ------------------------------------------------------

\ TODO The traverse functions must take a return argument 0/-1 to continue
\ traversing or stop it

\ TODO Use a map (instead of traverse) for not stopping (like iter and *iter)

?: UPPER ( u1 -- u2 ) dup dup 97 >= swap 122 <= and if 32 - then ;

\ TODO Implement better compare-no-case with *czip 

\ ?: (COMPARE-NO-CASE) 2>r -1 swap 2r> [: upper over c@ upper <> if drop 0 swap leave then char+ ;] citer drop ;
?: (COMPARE-NO-CASE) >r >r -1 swap r> r> [: upper over c@ upper <> if drop 0 swap leave then char+ ;] citer drop ;
?: COMPARE-NO-CASE	rot over = if (compare-no-case) else 3drop 0 then ;

\ ( i*x xt -- j*x ) Map xt to each word, use LEAVE to exit mapping
?: TRAVERSE-DICT	1 latest @ [: i swap keep-xt i nt>link i - ;] doloop drop ;

\ ( i*x xt wid -- j*x )
?: TRAVERSE-WORDLIST [: nt>wordlist over = if >r >r i r@ execute r> r> then ;] traverse-dict 2drop ;

\ ( c-addr u wid -- nt | 0 ) 
?: FIND-NAME-IN >r 0 -rot r> [: nt>name 2over compare-no-case if rot drop i -rot leave then ;] swap traverse-wordlist 2drop ;

\ ( c-addr u wid -- 0 | xt 1 | xt -1 )
?: SEARCH-WORDLIST find-name-in dup if dup nt>xt swap immediate? if 1 else -1 then then ;
 
\ -- More conditional compilation ---------------------------------------------

\ Implementation of [IF] [ELSE] [THEN] by ruv, as seen in:
\ https://forth-standard.org/standard/tools/BracketELSE

wordlist dup constant BRACKET-FLOW-WL get-current swap set-current
: [IF]				1+ ;					\ ( level1 -- level2 )
: [ELSE]			dup 1 = if 1- then ;	\ ( level1 -- level2 )
: [THEN]			1- ;					\ ( level1 -- level2 )
set-current

: [ELSE] \ ( -- )
	1 begin 
		begin 
			parse-name dup while
			bracket-flow-wl search-wordlist if 
				execute dup 0= if drop exit then
			then
		repeat 
		2drop refill 0= 
	until 
	drop
; immediate

: [THEN]	; immediate \ ( -- )

: [IF]		0= if postpone [else] then ; immediate \ ( flag -- )
 
 
\ \ TODO With current implementation it's not possible to do multiline quotations
\ \ on interpret mode because the input source on refill is put in the middle 
\ \ of the quotation !!!!!
\ \ TODO Try to add CZIP and ZIP to iterate over two arrays at once
\ \ TODO Try to add parse earlier to allow stack comments
\ \ TODO Add more recognizers (string, double numbers, quote for xts...)
\ \ TODO Add numeric output
\ \ TODO Add tools (dump trace debug)
\ 

\ -- Tools for recognizers ----------------------------------------------------

?: SET-RECOGNIZERS ( xtn ... xt1 n -- )
	dup forth-recognizer @ cell- ! forth-recognizer @ swap ['] ! -iter/addr ;

?: GET-RECOGNIZERS ( -- xtn ... xt1 n ) 
	forth-recognizer @ dup cell- @ dup >r ['] noop iter r> ;

\ -- Characters ---------------------------------------------------------------

?: BL ( -- n ) 32 ; 
?: CR ( -- ) 10 emit ;
?: SPACE ( -- ) bl emit ;
?: SPACES ( n -- ) [: space ;] times ;

?: [CHAR] ( C: "<spaces>name" -- ) ( -- char ) parse-name drop c@ postpone literal ; immediate

\ -- String recognizer --------------------------------------------------------

[UNDEFINED] rectype-string [IF]

' noop
:noname postpone sliteral ;
:noname -48 throw ; 
rectype: rectype-string

: rec-string ( addr len -- addr' len' rectype-string | rectype-null )
	over c@ [char] " <> if 2drop rectype-null exit then
	negate /source nip 0= if 2+ else 1+ then >in +! drop
	[char] " parse
	-1 /string
	rectype-string
;

get-recognizers 1+ ' rec-string swap set-recognizers

[THEN]

\ -- Tools --------------------------------------------------------------------

?: WORDS [: nt>name type space ;] get-order over >r set-order r> traverse-wordlist ;

\ -- Doubles (Taken mostly from SwapForth) ------------------------------------

?: D+		swap >r + swap r@ + swap over r> u< - ;
?: DNEGATE	invert swap negate tuck 0= - ;

\ -- Numeric output -----------------------------------------------------------

?: DABS ( d1 -- d2 ) dup 0 < if dnegate then ;
?: UD/MOD ( -?- ) >r 0 r@ um/mod r> swap >r um/mod r> ;

[UNDEFINED] . [IF]
variable hld
create <HOLD 100 chars dup allot <hold + constant HOLD>

: <# ( -- ) hold> hld ! ;
: HOLD ( c -- ) hld @ char - dup hld ! c! ;
: # ( d1 -- d2 ) base @ ud/mod rot 9 over < if 7 + then 48 + hold ;
: #S ( d1 -- d2 ) begin # over over or 0= until ;
: #> ( d -- c-addr u ) drop drop hld @ hold> over - ;

: SIGN ( n -- ) 0 < if 45 hold then ;

: UD.R ( ud n -- ) >r <# #s #> r> over - spaces type ;
: UD. ( ud -- ) 0 ud.r space ;
: U.R ( -?- ) 0 swap ud.r ;
: U. ( u -- ) 0 ud. ;
: D.R ( d n -- ) >r swap over dabs <# #s rot sign #> char / r> over - spaces type ;
: D. ( d -- ) 0 d.r space ;
: .R ( -?- ) >r dup 0 < r> d.r ;
: . ( n -- ) dup 0 < d. ;
[THEN]

?: ? ( addr -- ) @ . ; ( addr -- )
