#define SLOTH_IMPLEMENTATION
#include "sloth.h"

int main(int argc, char**argv) {
	X* x = sloth_new();

	sloth_free(x);
}
