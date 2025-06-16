\ Based on:
\ https://forth-standard.org/proposals/special-memory-access-words
\ (but not accepting that 1 CHARS = 1 BYTE as my own
\ Java implementation will break that)

1 CHARS 1 = [IF]
?: BYTES ( n -- n ) ;
?: B@ ( b-addr -- x ) C@ ;
?: B! ( x b-addr -- ) C! ;
[ELSE]
\ Implement it using C@/C! or @/!
[THEN]

\ TODO Implement (16 bits) W@ W! (32 bits) L@ L! (64 bits) W@ W!
