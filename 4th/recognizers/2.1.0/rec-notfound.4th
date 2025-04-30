\
\ Purpose: Provide the not-found system hook
\
\ Author: Matthias Trute
\ Date:   Sep 14, 2017
\ License: Public Domain
\
\ Add the rec-notfound as the last recognizer and
\ it will call a hook. It should not return, for
\ testing purposes, the one below does nothing but
\ clean up the stack.

s" Recognizer.4th" included

\ the general system hook
defer not-found ( addr len -- ) 

\ and its default action 
:noname ( addr len -- ) 
  2DROP 
  -13 \ throw
; is not-found 

' not-found dup dup rectype: rectype-notfound

: rec-notfound ( addr len -- addr len rectype-notfound )
  rectype-notfound
;

\ ------------- Test Cases ------------

: huhu s" huhu " ;

T{ huhu rec-notfound  -> huhu rectype-notfound }T
T{ huhu rec-notfound rectype>int execute -> -13 }T
