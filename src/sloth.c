#include<assert.h>
#include<stdio.h>
#include<strings.h>

#include "sloth.h"

void bye(SLOTH* sloth) {
	printf("\n");
	deinit_sloth(sloth);
	exit(0);
}

void main() {
	SLOTH* sloth = init_sloth();

	if (!sloth) {
		printf("Can't create SLOTH context.\n");
		return;
	}

	add_cfunc(sloth, "bye", (FUNC)&bye);
	add_cfunc(sloth, "words", (FUNC)&dump_words);
	add_cfunc(sloth, "dup", (FUNC)&_dup);
	add_cfunc(sloth, "swap", (FUNC)&_swap);
	add_cfunc(sloth, "+", (FUNC)&_add);
	add_cfunc(sloth, "*", (FUNC)&_mul);
	add_cfunc(sloth, "-", (FUNC)&_sub);
	add_cfunc(sloth, "/", (FUNC)&_div);
	add_cfunc(sloth, "@", (FUNC)&_fetch);
	add_cfunc(sloth, "!", (FUNC)&_store);
	add_cfunc(sloth, "here", (FUNC)&_here);
	add_cfunc(sloth, "state", (FUNC)&_state);
	add_cfunc(sloth, ".s", (FUNC)&_dump_stack);
	add_cfunc(sloth, ".", (FUNC)&_dot);

	char buf[80];
	CELL literal;
	char *token;
	char delim[] = " \r\n";

	printf("SLOTH v0.1\n");
	dump_words(sloth);
	
	// Basic outer interpreter
	while(1) {
		_refill(sloth); _drop(sloth);
		_parse_name(sloth);
		_dup(sloth);
		while(POP(sloth) != 0) {
			_to_counted_string(sloth);
			_find(sloth);
			if (POP(sloth) != 0) {
				_execute(sloth);
			} else {
				_drop(sloth);
				if (sscanf(sloth->TSB, "%ld", &literal)) {
					PUSH(sloth, literal);
				} else {
					sloth->DP = sloth->RP = 0;
				}
			}
			_parse_name(sloth);
			_dup(sloth);
		}
		_drop(sloth); _drop(sloth);
		printf(" ok\n");
	}
}
