#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"facility.h"

/* ---------------------------------------------------- */
/* -- main -------------------------------------------- */
/* ---------------------------------------------------- */

int main(int argc, char**argv) {
	X* x = sloth_new();

	sloth_bootstrap(x);
	sloth_bootstrap_facility_wordset(x);

	sloth_include(x, "../../4th/ans.4th");

	if (argc == 1) {
		sloth_repl(x);
	} else if (strcmp(argv[1], "--test") == 0 
					|| strcmp(argv[1], "-t") == 0) {
		chdir("../../forth2012-test-suite/src/");
		sloth_include(x, "runtests.fth");

		chdir("fp");
		sloth_include(x, "runfptests.fth");
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);
}


