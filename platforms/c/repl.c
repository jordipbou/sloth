#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"facility.h"
#include"file.h"
#include"locals.h"
#include"memory.h"
#include"cpnbi.h"

#include"sloth_sqlite.h"
#include"sloth_raylib.h"
#include"sloth_plibsys.h"

/* ---------------------------------------------------- */
/* -- main -------------------------------------------- */
/* ---------------------------------------------------- */

#ifndef ROOT_PATH
#define ROOT_PATH "../../"
#endif

int main(int argc, char**argv) {
	cpnbi_init();

	X* x = sloth_new();

	sloth_bootstrap(x);
	sloth_bootstrap_facility_wordset(x);
	sloth_bootstrap_file_wordset(x);
	sloth_bootstrap_locals_wordset(x);
	sloth_bootstrap_memory_wordset(x);

	sloth_include(x, ROOT_PATH "4th/ans.4th");

	/*
	sloth_bootstrap_sqlite(x);
	sloth_include(x, ROOT_PATH "4th/libs/sloth_sqlite.4th");

	sloth_bootstrap_raylib(x);
	sloth_include(x, ROOT_PATH "4th/libs/sloth_raylib.4th");

	sloth_bootstrap_plibsys(x);
	*/

	/* Set ROOT PATH */
	sloth_set_root_path(x, ROOT_PATH "4th/");
	/*
	memcpy(x->u + SLOTH_PATHS, ROOT_PATH, strlen(ROOT_PATH));
	sloth_user_area_set(x, SLOTH_ROOT_PATH_LENGTH, strlen(ROOT_PATH));
	sloth_user_area_set(x, SLOTH_PATH_START, x->u + SLOTH_PATH + strlen(ROOT_PATH));
	sloth_user_area_set(x, SLOTH_PATH_END, s->u + SLOTH_PATH + strlen(ROOT_PATH));
	*/

	if (argc == 1) {
		sloth_repl(x);
	} else if (strcmp(argv[1], "--test") == 0 
					|| strcmp(argv[1], "-t") == 0) {
		sloth_include(x, ROOT_PATH "forth2012-test-suite/src/runtests.fth");
		sloth_include(x, ROOT_PATH "forth2012-test-suite/src/fp/runfptests.fth");
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);

	cpnbi_shutdown();
}


