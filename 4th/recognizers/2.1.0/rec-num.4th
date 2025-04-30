\
\ Purpose: Number recognizers in Forth CORE 
\
\ Author: Matthias Trute
\ Date: Sep 14, 2017
\ License: Public Domain
\
\ not really smart but portable code, tested with old gforth's and vfxlin's
\

decimal

' noop :NONAME POSTPONE LITERAL ; DUP  RECTYPE: RECTYPE-NUM
' noop :NONAME POSTPONE 2LITERAL ; DUP RECTYPE: RECTYPE-DNUM

\
\ check for the 'c' syntax for single
\ characters.
\
: rec-char ( addr len -- n rectype-num | rectype-null )
  3 = if                       \ a three character string
    dup c@ [char] ' = if       \ that starts with a ' (tick)
      dup 2 + c@ [char] ' = if \ and ends with a ' (tick)
        1+ c@ rectype-num exit
      then
    then
  then
  drop rectype-null
;


\
\ helper words for number recognizers
\
\ set BASE according the the character at addr
\ or the 0x prefix string for HEX numbers.
\ returned string is stripped of the prefix
\ character(s) if found.
\ #: 10, $: 16, %: 2, &: 10 (again)
create num-bases 10 , 16 , 2 , 10 ,
: set-base ( addr len -- addr' len' )
  over c@ [CHAR] # - dup 0 4 within if 
    cells num-bases + @ base ! 1 /string 
  else
    drop
    \ check for a 0x prefix, requires the STRING wordset
    dup 2 >= if
       over 2 s" 0x" compare 0= if
         2 /string hex
       then
    then
  then 
;

\ check for a character. return string is 
\ without it if found. f is true if found.
: skip-char? ( addr len c -- addr' len' f)
  >r over c@ r> = dup >r
  if 1 /string then r> 
;

: -sign? ( addr len -- addr' len' f )
   [char] - skip-char?
;

: +sign  ( addr len -- addr' len' )
   [char] + skip-char? drop
;

\ allows $- and -$ combinations. skip +
\ f is true is - sign is found (and stripped off)
: base-and-sign? ( addr len -- addr' len' f)
  set-base +sign -sign? >r set-base r>
;

\ a factor the recognizers below.
: (rec-number) ( addr len -- d addr' len' f )
   base @ >r           \ save BASE
   base-and-sign? >r   \ handle prefix and sign characters
   2>r 0 0 2r> >number \ do the actual conversion
   r> r> base !        \ restore BASE and sign information
;

\
\ check for double number input
\
: rec-dnum ( addr len -- d rectype-dnum | rectype-null )
    \ simple syntax check: last character in addr/len is a dot . ?
    2dup + 1- c@ [char] . = if
      1-              \ strip trailing dot
      (rec-number) >r \ do the dirty work
      \ a number and only a number?
      nip if
        2drop r> drop rectype-null
      else 
        r> if dnegate then rectype-dnum 
      then
    else 
      2drop rectype-null  \ no, it cannot be a double cell number.
    then 
;

\
\ check for single cell number input
\
: rec-snum ( addr len -- n rectype-num | rectype-null )
    (rec-number) >r
    nip if 
      2drop r> drop rectype-null
    else
      \ a "d" with a non-empty upper cell cannot be an "n"
      if    r> drop rectype-null
      else  r> if negate then rectype-num 
      then
    then
;

\ combine them
3 STACK VALUE recstack-numbers

' rec-char ' rec-dnum ' rec-snum 3 recstack-numbers SET-STACK

: REC-NUM ( addr len -- n RECTYPE-NUM | d RECTYPE-DNUM | RECTYPE-NULL )
  recstack-numbers RECOGNIZE
;

\
\ ------------- Unit tests for all of the above ----------
\

VERBOSE ON

VARIABLE OLD-BASE BASE @ OLD-BASE !

T{ S" 'z'"  rec-char -> char z rectype-num }T
T{ S" 'z '" rec-char -> rectype-null }T

T{ S" 1234."  rec-dnum -> 1234.  rectype-dnum }T
T{ S" -1234." rec-dnum -> -1234. rectype-dnum }T
T{ S" +1234." rec-dnum -> 1234.  rectype-dnum }T

T{ S" $1234." rec-dnum -> $1234.   rectype-dnum }T
T{ S" $-1234." rec-dnum -> $-1234. rectype-dnum }T
T{ S" +$1234." rec-dnum -> $1234.  rectype-dnum }T
T{ S" -$1234." rec-dnum -> $-1234. rectype-dnum }T
T{ S" %-10010110." rec-dnum -> -150. rectype-dnum }T
T{ S" %10010110."  rec-dnum ->  150. rectype-dnum }T

T{ S" 1234"   rec-dnum -> rectype-null }T
T{ S" ABCXYZ" rec-dnum -> rectype-null }T

T{ S" 1234"   rec-snum ->  1234  rectype-num }T
T{ S" -1234"  rec-snum -> -1234  rectype-num }T
T{ S" $1234"  rec-snum -> $1234  rectype-num }T
T{ S" $-1234" rec-snum -> $-1234 rectype-num }T
T{ S" #1234"  rec-snum ->  1234  rectype-num }T
T{ S" %-10010110" rec-snum -> -150 rectype-num }T
T{ S" %10010110"  rec-snum ->  150 rectype-num }T

T{ S" #123a"  rec-snum -> rectype-null }T
T{ S" 1234."  rec-snum -> rectype-null }T
T{ S" ABCXYZ" rec-snum -> rectype-null }T

T{ S" 1234"   REC-NUM -> 1234 rectype-NUM }T
T{ S" 1234."  REC-NUM -> 1234. rectype-DNUM }T
T{ S" %-10010110" rec-num -> -150 rectype-num }T
T{ S" %10010110"  rec-num ->  150 rectype-num }T
T{ S" 'Z'"    REC-NUM -> char Z rectype-NUM }T
T{ S" ABCXYZ" REC-NUM -> rectype-null }T

T{ S" 0x123"   REC-NUM -> $123 rectype-num }T
T{ S" 0x123."  REC-NUM -> $123. rectype-dnum }T

\ check whether BASE is unchanged
T{ BASE @ OLD-BASE @ = -> -1 }T
