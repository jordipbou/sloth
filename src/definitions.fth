Implementations collected from:

* Minimal Forth (https://github.com/uho/minimal)
* SwapForth (https://github.com/jamesbowman/swapforth)

# CORE

## "store" ! 

### Minimal Forth

Primitive

### SwapForth

Primitive


## "number-sign" #

### Minimal Forth

: # ( d -- d' )
     BASE @ ud/mod ROT 9 OVER < IF  7 + THEN  48 + HOLD ;

### SwapForth

: #
    0 base @ um/mod >r base @ um/mod swap
    9 over < [ char A char 9 1 + - ] literal and +
    [char] 0 + hold r>
;


## "number-sign-greater" #>

### Minimal Forth

: #> ( d -- c-addr len )  DROP DROP hld @ hold> over - ;

### SwapForth

: #>
    2drop hld @ BUF over -
;


## "number-sign-s" #S

### Minimal Forth

: #s  ( d -- d' )
  BEGIN  # OVER OVER or 0= UNTIL ;

### SwapForth

: #s
    begin
        #
        2dup d0=
    until
;


## "tick" ' 

### Minimal Forth

Primitive

### SwapForth

: '
    parse-name
    sfind
    0= -13 and throw
;


## "paren" (

### Minimal Forth

Primitive

### SwapForth

: (
    [char] ) parse 2drop
; immediate


## "star" *

### Minimal Forth

Primitive

### SwapForth

Primitive


## "star-slash" */

### Minimal Forth

: */ ( n1 n2 n3 -- n4 )  */MOD SWAP DROP ;

### SwapForth

: */ */mod nip ;


## "star-slash-mod" */

### Minimal Forth

Primitve

### SwapForth

: */mod     >r m* r> sm/rem ;


## "plus" +

### Minimal Forth

Primitive

### SwapForth

Primitive
