#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"sloth_raylib.c"

int main(int argc, char** argv) {
	X* x = sloth_new();

	sloth_bootstrap(x);

	/* It's important to include ans.4th before */
	/* bootstrapping raylib to allow evaluating */
	/* forth code in bootstrap_raylib function. */
	sloth_include(x, FORTH_PATH "ans.4th");

	bootstrap_raylib(x);

	sloth_include(x, ROOT_PATH "raylib.4th");

	if (argc == 1) {
		sloth_repl(x);
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);
}
