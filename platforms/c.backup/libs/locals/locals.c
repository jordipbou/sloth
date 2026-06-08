#include "locals.h"

/* -- Locals words ------------------------------------- */

void sloth_paren_local_paren_(X* x) {
}

/* -- Locals extension words --------------------------- */

void sloth_locals_bar_(X* x) {
}

void sloth_brace_colon_(X* x) {
}

/* == Bootstrapping ==================================== */

void sloth_bootstrap_locals_wordset(X* x) {
	
	/* -- Locals words ----------------------------------- */

	sloth_code(x, "(LOCAL)", sloth_primitive(x, &sloth_paren_local_paren_));

	/* -- Locals extension words ------------------------- */

	sloth_code(x, "LOCALS|", sloth_primitive(x, &sloth_locals_bar_));
	sloth_code(x, "{:", sloth_primitive(x, &sloth_brace_colon_));
}
