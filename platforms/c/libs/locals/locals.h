#include "sloth.h"

/* -- Locals words ------------------------------------- */

void sloth_paren_local_paren_(X* x);

/* -- Locals extension words --------------------------- */

void sloth_locals_bar_(X* x); /* Obsolescent */
void sloth_brace_colon_(X* x);

/* == Bootstrapping ==================================== */

void sloth_bootstrap_locals_wordset(X* x);
