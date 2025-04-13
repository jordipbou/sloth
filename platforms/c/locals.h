#include"sloth.h"

/* More facilities for defining routines (compiling-mode only) */

void _paren_local_paren(X* x) { /* TODO */ }
void _locals(X* x) { /* TODO */ }
void _brace_colon(X* x) { /* TODO */ }

void bootstrap_locals_wordset(X* x) {
	code(x, "(LOCAL)", primitive(x, &_paren_local_paren));
	code(x, "LOCALS", primitive(x, &_locals));
	code(x, "{:", primitive(x, &_brace_colon));
}
