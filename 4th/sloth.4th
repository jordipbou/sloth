-1 trace!

: ?: token find [ ilen ipos! ] [ 1 ipos! ] choose ;

?: \ ilen ipos! ; immediate

\ End of line comments can be used now.

\ ?: (defined above) allos conditional compilation 
\ of one line definitions.

\ Let's ensure we have DUP and <>

?: DUP 0 pick ;
?: OVER 1 pick ;

?: 2DROP drop drop ;

?: <> = invert ;

\ /SOURCE allows easy access to current character
?: /SOURCE ibuf ipos char * + ;

?: LITERAL postpone doLIT , ; immediate

?: CHR token drop c@ ;
?: [CHR] chr postpone literal ; immediate

\ Parse (advance input) to next )
?: ( [chr] ) to/char 2drop ; immediate

( Now we can use stack comments too )

?: ['] ' postpone literal ; immediate

?: CR 10 emit ;
?: TYPE [ dup c@ emit char + ] times drop ;

0 trace!
