\
\ Purpose: temporarly switch off all actions until
\          a delimiting word is found and executed.
\          Useful to comment larger text parts.
\
\          (( switches to a limited command set and
\          makes all words no-operations. Only words
\          in a special wordlist are allowed for
\          execution and )) switches back to normal.
\          The recognizer switch survives REFILL's so
\          multi line comments are included.
\ Typical use
\
\ <forth code> (( now comments
\  and more comments like foo
\  finish comment )) <next forth code>
\
\ adding "foo" to the comment-actions wordlist
\ makes "foo" a valid command inside of (( ))
\ that may be executed or compiled, depending
\ on STATE and immediacy.
\
\ Author: Matthias Trute
\ Date: Sep 14, 2017
\ License: Public Domain
\

S" Recognizer.4th" INCLUDED

\ keep the previously active forth-recognizer stack
variable old-frs
wordlist constant comment-actions

get-current
comment-actions set-current

\ every word in this wordlist is executed during comments
\ at least the )) is needed.

\ switch back to the saved recognizer stack
: ))
  old-frs @ to forth-recognizer 
; immediate \ just in case

\ that's all for the comment actions
set-current

\ every word is fine. Even the ones that are not found
' noop dup dup rectype: rectype-skip
: rec-skip ( addr len -- rectype-skip ) 2drop rectype-skip ;

\ search only in the comment-actions wordlist
: rec-comment-actions ( addr len -- xt +/-1 rectype-xt | rectype-null )
  comment-actions search-wordlist ( xt +/-1 | 0 )
  ?dup if rectype-xt else rectype-null then 
;

\ a simple two-element recognizer stack
2 stack value rs-comment
' rec-skip ' rec-comment-actions 2 rs-comment set-stack

\ save the current recognizer stack and
\ switch over to the limited one
: (( ( -- )
  forth-recognizer old-frs !
  rs-comment to forth-recognizer
;

\ ------------- Test Cases ----------------

: rec-comment rs-comment recognize ;
T{ S" DUP"  rec-comment -> rectype-skip }T
T{ S" 1234" rec-comment -> rectype-skip }T

\ the XT of )) is not important
T{ S" ))"   rec-comment rot drop -> 1 rectype-xt }T
