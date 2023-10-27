: context $b ;

: drop $_ ;
: dup $d ;
: over $o ;
: swap $s ;
: rot $r ;
: nip $n ;

: + $+ ;
: - $- ;
: * $* ;
: / $/ ;
: mod $% ;

: or $| ;

: @ $. ;
: ! $, ;
: c@ $: ;
: c! $; ;

: cell $c ;
: cells cell * ;

: sp@ context ;
: rp@ context cell + ;
: ip context 2 cells + ;
: err context 3 cells + ;
: dictionary context 4 cells + @ ;

: rel>abs dictionary + ;
: abs>rel dictionary - ;

: latest dictionary 3 cells + @ rel>abs ;

: nt>flags 2 cells + ;
: nt>length name>flags 1 + ;
: nt>name dup name>length c@ swap name>length 1 + swap ;

: flag-immediate 4 ;

: immediate latest name>flags dup c@ flag-immediate or swap c! ;
