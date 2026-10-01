#include "sloth.h"
#ifndef SLOTH_WITHOUT_FILE_WORD_SET
	#include "file.h"
#endif
#ifndef SLOTH_WITHOUT_MEMORY_WORD_SET
	#include "memory.h"
#endif

#ifndef ROOT_PATH
#define ROOT_PATH "../../"
#endif

int main(int argc, char**argv) {
	X* x;
	int ior;
	int is_test;
	CELL errors = 0;

	is_test = (argc > 1 && (strcmp(argv[1], "--test") == 0
					|| strcmp(argv[1], "-t") == 0));

	x = sloth_new();

	sloth_bootstrap(x);
#ifndef SLOTH_WITHOUT_FILE_WORD_SET
	sloth_bootstrap_file_word_set(x);
#endif
#ifndef SLOTH_WITHOUT_MEMORY_WORD_SET
	sloth_bootstrap_memory_word_set(x);
#endif

	sloth_set_root_path(x, ROOT_PATH);
	if (sloth_include(x, "ans.4th")) {
		printf("Fatal error: ans.4th can not be included.\n");
		exit(-1);
	}

	if (argc == 1) {
		sloth_run(x);
	} else if (is_test) {
		/* Standard tests */
		ior = sloth_include(x, ROOT_PATH "forth2012-test-suite/src/runtests.fth");
		if (ior) errors++;

		#ifndef SLOTH_WITHOUT_FLOATING_POINT

		/* Floating point tests */
		ior = sloth_include(x, ROOT_PATH "forth2012-test-suite/src/fp/runfptests.fth");
		if (ior) errors++;

		#endif

		/* Sentinel read by platforms/c/check_forth_tests.cmake. It is */
		/* only printed when both suites ran to completion. */
		if (errors == 0) printf("SLOTH-TEST-DONE\n");
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);

	return errors != 0;
}
