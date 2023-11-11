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

: @ $@ ;
: ! $! ;
: c@ $: ;
: c! $; ;
: s@ $" ;
: s! $' ;
: i@ $. ;
: i! $, ;

: literal $l ;

: 0branch $z ;

: cell $c ;
: cells cell * ;

: context $b ;

: sp@ context ;
: rp@ context cell + ;
: ip context 2 cells + ;
: err context 3 cells + ;
: dictionary context 4 cells + @ ;
: memory dictionary @ ;

: rel>abs memory + ;
: abs>rel memory - ;

: latest dictionary 3 cells + @ rel>abs ;

: previous @ ;

: nt>flags 2 cells + ;
: nt>length nt>flags 1 + ;
: nt>name dup nt>length c@ swap nt>length 1 + swap ;

: flag-immediate 4 ;

: immediate latest nt>flags dup c@ flag-immediate or swap c! ;

: here@ dictionary cell + ;
: here here@ @ rel>abs ;
: allot here@ @ + here@ ! ;

: , here ! cell allot ;
: c, here c! 1 allot ;
: s, here s! 2 allot ;
: i, here i! 4 allot ;

: >mark here 1 + 1024 literal ;
: >resolve here over - swap s! ;

: if >mark postpone 0branch ; immediate
: then >resolve ; immediate