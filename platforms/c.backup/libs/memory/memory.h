#include "sloth.h"

/* -- Memory words ------------------------------------- */

void sloth_allocate_(X* x);
void sloth_free_(X* x);
void sloth_resize_(X* x);

/* == Bootstrapping ==================================== */

void sloth_bootstrap_memory_wordset(X* x);
