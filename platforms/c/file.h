#include"sloth.h"

void _include_file(X* x) {
	/* TODO */
}

void bootstrap_file_wordset(X* x) {
	code(x, "INCLUDE-FILE", primitive(x, &_include_file));
}
