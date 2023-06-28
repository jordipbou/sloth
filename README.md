# SLOTH

A small, simple, relatively fast, double stack based virtual machine with human readable/writeable bytecode.

Inspired by STABLE Forth, RetroForth/ilo, XY, Joy/Factor.

Features:

* One header only. Very easily embedabble in a C/C++ application.
* Human readable bytecode.
* Quotations at bytecode level.
* Relatively fast interpreter.
* Ability to add C function extensions.

## Bytecode

* (SPACE)	noop
* !				store cell sized value at address ( n addr -- )
* "
* #				(TODO: number literal)
* $				swap ( n2 n1 -- n1 n2 )
* %				mod ( n2 n1 -- n2 % n1 )
* &				and ( n2 n1 -- n2 & n1 )
* '
* (
* )
* *				multiplication ( n2 n1 -- n2 * n1 )
* +				addition ( n2 n1 -- n2 + n1 )
* ,				compile cell sized value to here, incrementing here ( n -- )
* /				division ( n2 n1 -- n2 / n1 )
* 0-9			parsed number literal
* :
* ;
* <				less than ( n2 n1 -- n2 < n1 )
* =				equal ( n2 n1 -- n2 == n1 )
* >				greater than ( n2 n1 -- n2 > n1 )
* ?				if ( b q2 q1 -- [q2 if b else q1] )
* @				fetch cell sized value from address ( addr -- n )
* A-Z			extensions
* [				start of quotation ( -- q )
* \				
* ]				return/end of quotation
* ^
* _				drop item from stack ( a -- )	
* `
* a				alloc
* b		
* c				char	
* d				dup
* e				emit
* f				
* g				
* h				here
* i				exec
* j				
* k				key
* l				long
* m				
* n				
* o				over
* p				
* q				exit
* r				rot
* s				short
* t				
* u		
* v		
* w		
* x		
* y		
* z		
* {
* |				or ( n2 n1 -- n2 | n1 )
* }
* ~				not ( n -- not n )

