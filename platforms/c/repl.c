#include "sloth.h"
#include "cpnbi.h"
#ifndef SLOTH_WITHOUT_FILE_WORD_SET
	#include "file.h"
#endif
#ifndef SLOTH_WITHOUT_MEMORY_WORD_SET
	#include "memory.h"
#endif

#ifndef ROOT_PATH
#define ROOT_PATH "../../"
#endif

void custom_e_key_(X* x) { sloth_push(x, cpnbi_get_event()); }

int main(int argc, char**argv) {
	cpnbi_init();

	X* x = sloth_new();
	sloth_set_e_key(custom_e_key_);

	sloth_bootstrap(x);
#ifndef SLOTH_WITHOUT_FILE_WORD_SET
	sloth_bootstrap_file_word_set(x);
#endif
#ifndef SLOTH_WITHOUT_MEMORY_WORD_SET
	sloth_bootstrap_memory_word_set(x);
#endif

	sloth_set_root_path(x, ROOT_PATH "4th/");
	sloth_include(x, "ans.4th");

	if (argc == 1) {
		sloth_repl(x);
	} else if (strcmp(argv[1], "--test") == 0 
					|| strcmp(argv[1], "-t") == 0) {
		// Standard tests
		sloth_include(x, ROOT_PATH "forth2012-test-suite/src/runtests.fth");

		#ifndef SLOTH_WITHOUT_FLOATING_POINT
		
		// Floating point tests
		sloth_include(x, ROOT_PATH "forth2012-test-suite/src/fp/runfptests.fth");

		#endif
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);
}
