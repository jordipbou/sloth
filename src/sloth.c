#include<assert.h>
#include<stdio.h>
#include<strings.h>

#include "sloth.h"

void bye(SLOTH* sloth) {
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
	add_cfunc(sloth, "@", (FUNC)&_fetch);
	add_cfunc(sloth, "!", (FUNC)&_store);
	add_cfunc(sloth, "here", (FUNC)&_here);
	add_cfunc(sloth, "state", (FUNC)&_state);

	char buf[80];
	CELL literal;
	char *token;
	char delim[] = " \r\n";

	printf("SLOTH v0.1\n");
	dump_words(sloth);
	
	// Basic outer interpreter
	while(1) {
		fgets(buf, 80, stdin);
		// Interpret...
		token = strtok(buf, delim);
		while(token != NULL) {
			eval_word(sloth, token);
			switch(sloth->flags) {
				case ERR_UNDEFINED_WORD:
					// Maybe its a number?
					if (sscanf(token, "%ld", &literal)) {
						PUSH(sloth, literal);
						sloth->flags = ERR_OK;
					} else {
						printf(" Undefined word.\n");
						sloth->DP = sloth->RP = 0;
					}
					break;
			}
			token = strtok(NULL, delim);
		}
		printf(" ok "); dump_stack(sloth);
	}
}
