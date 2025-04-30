\
\ an alternative implementation of recognizers. It
\ uses a separate stack module, that can be used
\ to implement the search order words independently
\
\ Date: Sep 14, 2017
\ Author: Matthias Trute
\ License: Public Domain

s" tester.fs" INCLUDED
s" Stack.4th" INCLUDED

4 STACK VALUE FORTH-RECOGNIZER

\ define a recognizer with three actions. Suggesting RECTYPE-* names
: RECTYPE: ( XT-INTERPRET XT-COMPILE XT-POSTPONE "<spaces>name" -- )
  CREATE SWAP ROT , , , 
;

: RECTYPE>POST ( RECTYPE-TOKEN -- XT-POSTPONE ) CELL+ CELL+ @ ;
: RECTYPE>COMP ( RECTYPE-TOKEN -- XT-COMPILE  )       CELL+ @ ;
: RECTYPE>INT  ( RECTYPE-TOKEN -- XT-INTERPRET)             @ ;

:NONAME -1 ABORT" :=(" ; DUP DUP RECTYPE: RECTYPE-NULL

: (recognize) ( addr len XT -- addr len 0 | i*x RECTYPE-TOKEN -1 )
   ROT ROT 2DUP 2>R ROT EXECUTE 2R> ROT
   DUP RECTYPE-NULL = ( -- i*x addr len rectype-token f )
   IF DROP 0 ELSE NIP NIP -1 THEN
;

: RECOGNIZE ( addr len stack-id -- i*x rectype-token | rectype-null )
    ['] (recognize) SWAP map-stack ( -- i*x rectype-token -1 | addr len 0 )
    0= IF \ no recognizer did the job, remove addr/len
     2DROP RECTYPE-NULL
    THEN
;

\ Now build the runtime environment needed to play with all of the above

s" rec-find.4th" INCLUDED
s" rec-num.4th" INCLUDED

' REC-NUM ' REC-FIND 2 FORTH-RECOGNIZER SET-STACK

VERBOSE ON

:noname 1 ; :noname 2 ; :noname 3 ; rectype: rectype-test

T{ rectype-test rectype>int catch -> 1 0 }T
T{ rectype-test rectype>comp catch -> 2 0 }T
T{ rectype-test rectype>post catch -> 3 0 }T

T{ S" 1234"  FORTH-RECOGNIZER RECOGNIZE -> 1234  RECTYPE-NUM   }T
T{ S" 1234." FORTH-RECOGNIZER RECOGNIZE -> 1234. RECTYPE-DNUM  }T
T{ S" DUP"   FORTH-RECOGNIZER RECOGNIZE -> ' DUP -1 RECTYPE-XT }T
T{ S" UNKOWN WORD" FORTH-RECOGNIZER RECOGNIZE  -> RECTYPE-NULL }T
