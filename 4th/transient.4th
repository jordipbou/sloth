variable there
here unused + there !

: tallot ( n -- )
	unused over < if -8 throw then
	there @ swap - there !
;

variable tmarker

\ Compilation only
: tmark ( -- )
	r> tmarker @ >r >r
	there @ tmarker !
;

\ Compilation only
: tfree ( -- )
	tmarker @ there !
	r> r> tmarker ! >r
;


