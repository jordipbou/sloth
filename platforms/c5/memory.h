#include"sloth.h"

/* Dynamic memory operations */

void _allocate(X* x) { push(x, (CELL)malloc(pop(x))); }
void _free(X* x) { free((void*)pop(x)); }
void _resize(X* x) { /* TODO */ }

void bootstrap_memory_wordset(X* x) {
	code(x, "ALLOCATE", primitive(x, &_allocate));
	code(x, "FREE", primitive(x, &_free));
	code(x, "RESIZE", primitive(x, &_resize));
}
