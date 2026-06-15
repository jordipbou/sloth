#include "sloth.h"
#include <stdint.h>

/* -- Locals words ------------------------------------- */

void sloth_allocate_(X* x);
void sloth_free_(X* x);
void sloth_resize_(X* x);

/* -- Special memory access words proposal ------------- */

void sloth_b_fetch_(X* x);
void sloth_b_store_(X* x);
void sloth_w_fetch_(X* x);
void sloth_w_store_(X* x);
void sloth_l_fetch_(X* x);
void sloth_l_store_(X* x);
void sloth_x_fetch_(X* x);
void sloth_x_store_(X* x);

/* -- Memory words to interface with C code ------------ */

void sloth_ints_(X* x);
void sloth_int_fetch_(X* x);
void sloth_int_store_(X* x);

/* == Bootstrapping ==================================== */

void sloth_bootstrap_memory_word_set(X* x);
