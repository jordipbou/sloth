#define SLOTH_IMPLEMENTATION
#include"sloth.h"

/* ---------------------------------------------------- */
/* -- main -------------------------------------------- */
/* ---------------------------------------------------- */

int main(int argc, char**argv) {
	X* x = sloth_new();

	bootstrap(x);

	include(x, "../../4th/ans.4th");

	if (argc == 1) {
		repl(x);
	} else if (strcmp(argv[1], "--test") == 0 
					|| strcmp(argv[1], "-t") == 0) {
		chdir("../../forth2012-test-suite/src/");
		include(x, "runtests.fth");
	} else {
		include(x, argv[1]);
	}

	sloth_free(x);
}


