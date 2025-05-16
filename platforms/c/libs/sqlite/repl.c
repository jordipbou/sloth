#define SLOTH_IMPLEMENTATION
#include"sloth.h"
#include"sloth_sqlite.h"

/* ---------------------------------------------------- */
/* -- main -------------------------------------------- */
/* ---------------------------------------------------- */

#ifndef ROOT_PATH
#define ROOT_PATH "../../"
#endif

int main(int argc, char**argv) {
	X* x = sloth_new();

	sloth_bootstrap(x);
	sloth_bootstrap_sqlite(x);

	sloth_include(x, ROOT_PATH "4th/ans.4th");

	if (argc == 1) {
		sloth_repl(x);
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);
}



