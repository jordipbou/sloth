#include "sloth.h"
#include "cpnbi.h"

#ifndef ROOT_PATH
#define ROOT_PATH "../../"
#endif

int main(int argc, char**argv) {
	cpnbi_init();

	X* x = sloth_new();

	sloth_bootstrap(x);

	sloth_include(x, ROOT_PATH "4th/ans.4th");

	/* Set ROOT PATH */
	sloth_set_root_path(x, ROOT_PATH "4th/");

	if (argc == 1) {
		sloth_repl(x);
	} else if (strcmp(argv[1], "--test") == 0 
					|| strcmp(argv[1], "-t") == 0) {
		// Standard tests
		sloth_include(x, ROOT_PATH "forth2012-test-suite/src/runtests.fth");
		#ifdef SLOTH_FLOATING_POINT

		// Floating point tests
		sloth_include(x, ROOT_PATH "forth2012-test-suite/src/fp/runfptests.fth");

		#endif
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);

	cpnbi_shutdown();
}
