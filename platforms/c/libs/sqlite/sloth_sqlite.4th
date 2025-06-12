variable (sqlite-db)

wordlist constant DB-WORDLIST

get-order DB-WORDLIST swap 1+ set-order

get-current
DB-WORDLIST set-current

: noop ;

: in-memory-db ( -- )
	s" :memory:" open (sqlite-db) !
;

: create ( "<query>;..." -- )
	(sqlite-db) @
	>in @ 7 - >in !
	[char] ; parse
	' noop
	exec
;

: insert ( "<query>;..." -- )
	(sqlite-db) @
	>in @ 7 - >in !
	[char] ; parse
	' noop
	exec
;

: select ( "<query>;..." -- )
	(sqlite-db) @ 
	>in @ 7 - >in !
	[char] ; parse
	[: 0 do type ." |" loop cr ;]
	exec
; immediate

set-current

previous

: SQL[ get-order DB-WORDLIST swap 1+ set-order ;
: ]SQL previous ;

\ Tests

SQL[
in-memory-db
create table test (id int, name text);
insert into test values (1, "Jordi");
insert into test values (2, "Natalia");
select * from test;
]SQL
