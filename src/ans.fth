\wv{ : \$w$v${$] \}u
: ; \$$$]$}$u$] \}ui

: immediate \$i ;

: parse \$\ ;
: \ 10 parse ;

: ABORT \$q ;
\ TODO: ABORT" needs compilation of a string to work, leave it for later
: ABORT" ;

: DUP \$d ;

: < \$< ;
: 0< 0 < ;

\ TODO: IF ... mark resolve and all that jazz
: IF ;

: ABS dup 0< if negate then ;

