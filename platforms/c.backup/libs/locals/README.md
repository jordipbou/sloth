# Analysis

Locals are a very complicated feature in ANS Forth. Let's use
an example:

: rot {: a b c -- b c a :} b c a ;

This needs 3 values named a b c and initialized with the
stack values in the order c b a.

Each value needs to be able to be changed by the use of TO.

At compile time, the name of the value has to be findable,
although after using the semicolon, there's no need to store
the name.
