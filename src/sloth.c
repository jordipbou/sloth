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
	// Stack manipulation
	add_cfunc(sloth, "dup", (FUNC)&_dup);
	add_cfunc(sloth, "swap", (FUNC)&_swap);
	add_cfunc(sloth, "drop", (FUNC)&_drop);
	add_cfunc(sloth, "2dup", (FUNC)&_2dup);
	// Arithmetics
	add_cfunc(sloth, "+", (FUNC)&_add);
	add_cfunc(sloth, "*", (FUNC)&_mul);
	add_cfunc(sloth, "-", (FUNC)&_sub);
	add_cfunc(sloth, "/", (FUNC)&_div);
	add_cfunc(sloth, "mod", (FUNC)&_mod);
	// Logic
	add_cfunc(sloth, "&", (FUNC)&_and);
	add_cfunc(sloth, "|", (FUNC)&_or);
	add_cfunc(sloth, "invert", (FUNC)&_not);
	// Memory
	add_cfunc(sloth, "@", (FUNC)&_fetch);
	add_cfunc(sloth, "!", (FUNC)&_store);
	add_cfunc(sloth, "here", (FUNC)&_here);
	add_cfunc(sloth, "allot", (FUNC)&_allot);
	add_cfunc(sloth, "align", (FUNC)&_align);

	add_cfunc(sloth, "state", (FUNC)&_state);
	add_cfunc(sloth, ".s", (FUNC)&_dump_stack);

	add_cfunc(sloth, "key", (FUNC)&_key);
	add_cfunc(sloth, ".", (FUNC)&_dot);
	add_cfunc(sloth, "emit", (FUNC)&_emit);
	add_cfunc(sloth, "accept", (FUNC)&_accept);
	add_cfunc(sloth, "type", (FUNC)&_type);
	add_cfunc(sloth, ">in", (FUNC)&_in);
	add_cfunc(sloth, "source", (FUNC)&_source);
	add_cfunc(sloth, "refill", (FUNC)&_refill);
	add_cfunc(sloth, "parse-name", (FUNC)&_parse_name);
	add_cfunc(sloth, "find", (FUNC)&_find);
	add_cfunc(sloth, "execute", (FUNC)&_execute);

	add_cfunc(sloth, "create", (FUNC)&_create);

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
			_dup(sloth);
			if (POP(sloth) != 0) {
				if (sloth->state == ST_INTERPRETING) {
					_drop(sloth);
					_execute(sloth);
				} else {
					if (POP(sloth) == 1) {
						// Immediate word
						_execute(sloth);
					} else {
						_compile(sloth);
					}
				}
			} else {
				_drop(sloth); _drop(sloth);
				if (sscanf(sloth->TSB, "%ld", &literal)) {
					PUSH(sloth, literal);
				} else {
					// ERROR: Undefined word !!
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
