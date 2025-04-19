#include"sloth.h"

/* String operations */

void _slash_string(X* x) { /* TODO */ }
void _dash_trailing(X* x) { /* TODO */ }
void _search(X* x) { /* TODO */ }
void _compare(X* x) { /* TODO */ }
void _blank(X* x) { /* TODO */ }

/* Constructing compiler and interpreter system extensions */

void _s_literal(X* x) { /* TODO */ }

void bootstrap_string_wordset(X* x) {
	code(x, "/STRING", primitive(x, &_slash_string));
	code(x, "BLANK", primitive(x, &_blank));
	code(x, "COMPARE", primitive(x, &_compare));
	code(x, "SEARCH", primitive(x, &_search));
	code(x, "-TRAILING", primitive(x, &_dash_trailing));
	code(x, "SLITERAL", primitive(x, &_s_literal));
}
