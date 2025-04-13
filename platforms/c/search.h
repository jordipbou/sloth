#include"sloth.h"

/* Commands that change compilation & interpretation settings */

void _also(X* x) { /* TODO */ }
void _definitions(X* x) { /* TODO */ }
void _forth_wordlist(X* x) { /* TODO */ }
void _forth(X* x) { /* TODO */ }
void _only(X* x) { /* TODO */ }
void _order(X* x) { /* TODO */ }
void _previous(X* x) { /* TODO */ }
void _get_current(X* x) { /* TODO */ }
void _set_current(X* x) { /* TODO */ }
void _get_order(X* x) { /* TODO */ }
void _set_order(X* x) { /* TODO */ }
void _wordlist(X* x) { /* TODO */ }
void _search_wordlist(X* x) { /* TODO */ }

void bootstrap_search_wordset(X* x) {
	code(x, "ALSO", primitive(x, &_also));
	code(x, "DEFINITIONS", primitive(x, &_definitions));
	code(x, "FORTH", primitive(x, &_forth));
	code(x, "FORTH-WORDLIST", primitive(x, &_forth_wordlist));
	code(x, "GET-CURRENT", primitive(x, &_get_current));
	code(x, "GET-ORDER", primitive(x, &_get_order));
	code(x, "ONLY", primitive(x, &_only));
	code(x, "ORDER", primitive(x, &_order));
	code(x, "PREVIOUS", primitive(x, &_previous));
	code(x, "SET-CURRENT", primitive(x, &_set_current));
	code(x, "SET-ORDER", primitive(x, &_set_order));
	code(x, "WORDLIST", primitive(x, &_wordlist));
	code(x, "SEARCH-WORDLIST", primitive(x, &_search_wordlist));
}
