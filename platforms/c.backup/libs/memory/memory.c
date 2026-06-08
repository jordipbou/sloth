#include "memory.h"

/* -- Locals words ------------------------------------- */

void sloth_allocate_(X* x) {
	void *ptr = malloc(sloth_pop(x));
	if (ptr) {
		sloth_push(x, (CELL)ptr);
		sloth_push(x, 0);
	} else {
		sloth_push(x, 0);
		sloth_push(x, -59);
	}
}

void sloth_free_(X* x) {
	free((void *)sloth_pop(x));
	sloth_push(x, 0);
}

void sloth_resize_(X* x) {
	CELL u = sloth_pop(x);
	void *addr = (void *)sloth_pop(x);
	void *ptr = realloc(addr, u);
	if (ptr) {
		sloth_push(x, (CELL)ptr);
		sloth_push(x, 0);
	} else {
		sloth_push(x, (CELL)addr);
		sloth_push(x, -61);
	}
}

/* == Bootstrapping ==================================== */

void sloth_bootstrap_memory_wordset(X* x) {
	
	/* -- Locals words ----------------------------------- */

	sloth_code(x, "ALLOCATE", sloth_primitive(x, &sloth_allocate_));
	sloth_code(x, "FREE", sloth_primitive(x, &sloth_free_));
	sloth_code(x, "RESIZE", sloth_primitive(x, &sloth_resize_));
}
