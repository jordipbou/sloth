\
\ Purpose: Use two " characters as the delimiters 
\          of a string.
\
\ Author: Matthias Trute
\ Date:   Sep 14, 2017
\ License: Public Domain
\
\ This is an example for a recognizer that depends
\ on >IN (and thus SOURCE) for input. So, no test
\ cases.
\

' noop 
:noname postpone sliteral ;
:noname -48 throw ; rectype: rectype-string

: rec-string ( addr len -- addr' len' rectype-string | rectype-null )
  over c@ [char] " <> if 2drop rectype-null exit then
  negate 1+ >in +! drop  \ adjust the  parse area in SOURCE
  [char] " parse         \ eat the trailing delimiter
  -1 /string
  rectype-string
;
