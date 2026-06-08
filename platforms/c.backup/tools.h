#include "sloth.h"

/* Commands that change compilation & interpretation settings */

void _assembler(X* x) { /* TODO */ }
void _editor(X* x) { /* TODO */ }

/* Commands to inspect memory, debug & view code */

void _words(X* x) { /* TODO */ }
void _dot_s(X* x) { 
	CELL i;
	printf("<%ld> ", x->sp);
	for (i = 0; i < x->sp; i++) {
		printf("%ld ", x->s[i]);
	}
}
void _dump(X* x) { /* TODO */ }
void _question(X* x) { printf("%ld ", fetch(x, pop(x))); }
void _see(X* x) { 
	CELL tok, tlen, i, xt, op = 0;
	CELL q = 0;
	CELL EXIT, QUOTATION, LIT;
	push(x, 32); _word(x);
	tok = pick(x, 0) + suCHAR;
	tlen = cfetch(x, pick(x, 0));
	if (tlen == 0) { pop(x); return; }
	printf("NAME: %.*s\n", (int)tlen, (char*)tok);
	_find(x); 
	i = pop(x);
	printf("IMMEDIATE (1 = YES, -1 = NO): %ld\n", i);
	xt = pop(x);
	printf("XT: %ld\n", xt);
	if (xt > 0) {
		EXIT = get_xt(x, find_word(x, "EXIT"));
		QUOTATION = get_xt(x, find_word(x, "(QUOTATION)"));
		LIT = get_xt(x, find_word(x, "(LIT)"));
		do {
			op = fetch(x, to_abs(x, xt));
			xt += sCELL;
			printf("%ld ", op);
			if (op == EXIT && q == 0) {
				break;
			} else if (op == EXIT && q > 0) {
				q--;
			} else if (op == QUOTATION) {
				q++;
			} else if (op == LIT) {
				op = fetch(x, to_abs(x, xt));
				xt += sCELL;
				printf("%ld ", op);
			} 
		} while (1);
		printf("\n");
	} 
}

/* Source code preprocessing, interpreting & auditing commands */

void _bracket_if(X* x) { /* TODO */ }
void _bracket_else(X* x) { /* TODO */ }
void _bracket_then(X* x) { /* TODO */ }

/* Constructing compiler and interpreter system extensions */

void _ahead(X* x) { 
	compile(x, get_xt(x, find_word(x, "(BRANCH)"))); 
	_here(x); 
	comma(x, 0); 
}

void _c_s_pick(X* x) { /* TODO */ }
void _c_s_roll(X* x) { /* TODO */ }

/* More facilities for defining routines (compiling-mode only) */

void _code(X* x) { /* TODO */ }
void _semicolon_code(X* x) { /* TODO */ }

void bootstrap_tools_wordset(X* x) {
	code(x, "ASSEMBLER", primitive(x, &_assembler));
	code(x, "EDITOR", primitive(x, &_editor));
	code(x, "WORDS", primitive(x, &_words));
	code(x, "[IF]", primitive(x, &_bracket_if));
	code(x, "[ELSE]", primitive(x, &_bracket_else));
	code(x, "[THEN]", primitive(x, &_bracket_then));
	code(x, "DUMP", primitive(x, &_dump));
	code(x, "?", primitive(x, &_question));
	code(x, ".S", primitive(x, &_dot_s));
	code(x, "SEE", primitive(x, &_see));
	code(x, "AHEAD", primitive(x, &_ahead));
	code(x, "CS-PICK", primitive(x, &_c_s_pick));
	code(x, "CS-ROLL", primitive(x, &_c_s_roll));
	code(x, "CODE", primitive(x, &_code));
	code(x, ";CODE", primitive(x, &_semicolon_code));
}
