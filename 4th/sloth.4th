0 trace!

: ?: token find [ ilen ipos! ] [ 1 ipos! ] choose ;

?: \ ilen ipos! ; immediate

\  \ End of line comments can be used now.
\  
\  \ ?: (defined above) allos conditional compilation 
\  \ of one line definitions.
\  
\  \ Let's ensure we have DUP and <>
\  
\  ?: DUP 0 pick ;
\  ?: OVER 1 pick ;
\  
\  ?: 2DROP drop drop ;
\  
\  ?: <> = invert ;
\  
\  ?: CELLS cell * ;
\  ?: CHARS char * ;
\  
\  \ /SOURCE allows easy access to current character
\  ?: /SOURCE ibuf ipos char * + ;
\  
\  ?: LITERAL postpone doLIT , ; immediate
\  
\  ?: CHR token drop c@ ;
\  ?: [CHR] chr postpone literal ; immediate
\  
\  \ Parse (advance input) to next )
\  ?: ( [chr] ) to/char 2drop ; immediate
\  
\  ( Now we can use stack comments too )
\  
\  ?: ['] ' postpone literal ; immediate
\  
\  ?: CR 10 emit ;
\  ?: TYPE [ dup c@ emit char + ] times drop ;
\  
\  \ -- Locals --
\  
\  \ TODO Change to uvwxyz (6 locals, more than enough!!)
\  
\  ?: 1L> l> drop ;
\  ?: 2L> l> l> drop drop ;
\  ?: 3L> l> l> l> drop drop drop ;
\  ?: 4L> l> l> l> l> drop drop drop drop ;
\  
\  ?: ->X >l r> ['] 1l> >r >r ;
\  ?: ->XY >l >l r> ['] 2l> >r >r ;
\  ?: ->XYZ >l >l >l r> ['] 3l> >r >r ;
\  ?: ->XYZU >l >l >l >l r> ['] 4l> >r >r ;
\  
\  ?: X 0 l@ ;
\  ?: Y 1 l@ ;
\  ?: Z 2 l@ ;
\  ?: U 3 l@ ;
\  
\  ?: X! 0 l! ;
\  ?: Y! 1 l! ;
\  ?: Z! 2 l! ;
\  ?: U! 3 l! ;
\  
\  \ TODO More than one line conditional compilation
\  
\  : ACCEPT ( c-addr u -- c-addr u )
\  	0 0 ->xyzu	( bufaddr maxlen key buflen )
\  	[ 
\  		y u < while
\  		z 13 = while
\  		z 10 = while
\  		key z!
\  		z emit
\  		z x u chars + c!
\  		u 1 + u!
\  	] loop
\  	x u
\  ;
\  
\  0 trace!
