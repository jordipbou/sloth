s" tester.fs" INCLUDED 
s" Stack.4th" INCLUDED

\ ------------- Test Cases ------------

4 STACK constant test

: s1 1 0 ; \ 0 means continue with map-stack
: s2 2 0 ;
: s3 3 0 ;
: s4 4 -1 ; \ -1 means premature exit from map-stack

\ set and get methods
T{ -1 test ' SET-STACK CATCH -> -1 test -4 }T
T{ 0 test SET-STACK -> }T
T{ test GET-STACK -> 0 }T

T{ ' s1 1 test SET-STACK -> }T
T{ test GET-STACK -> ' s1 1 }T

T{ ' s2 ' s1 2 test SET-STACK -> }T
T{ test GET-STACK -> ' s2 ' s1 2 }T

T{ ' s1 ' s2 ' s3 3 test SET-STACK -> }T
T{ test GET-STACK -> ' s1 ' s2 ' s3 3 }T

\ testing map-stack
\ the whole stack is used for execute
T{ ' EXECUTE test MAP-STACK -> 3 2 1 0 }T

T{ ' s1 ' s2 ' s4 3 test SET-STACK -> }T
\ only the 1st element is executed
T{ ' EXECUTE test MAP-STACK -> 4 -1 }T

\ append and prepend methods
T{ ' s1 1 test SET-STACK -> }T
T{ ' s2 test >STACK -> }T
T{ test GET-STACK -> ' s1 ' s2 2 }T
T{ test STACK> -> ' s2 }T
T{ test GET-STACK -> ' s1 1 }T

T{ ' s1 1 test SET-STACK -> }T
T{ ' s2 test >BACK -> }T
T{ test GET-STACK -> ' s2 ' s1 2 }T
T{ test BACK> -> ' s2 }T
T{ test GET-STACK -> ' s1 1 }T

\ depth
T{ 2 1 0 3 test SET-STACK -> }T
T{ test DEPTH-STACK -> 3 }T 

\ pick
T{ 2 1 0 3 test SET-STACK -> }T
T{ 0 test PICK-STACK -> 0 }T 
T{ 1 test PICK-STACK -> 1 }T 

T{ -1 test ' PICK-STACK CATCH -> -1 test -9 }T
T{ 5 test ' PICK-STACK CATCH -> 5 test -9 }T
