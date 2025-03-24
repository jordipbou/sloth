#include "core.h"

/* -- Helpers to add primitives to the dictionary ------ */

CELL primitive(X* x, F f) { 
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL code(X* x, char* name, CELL xt) {
	CELL w = header(x, (CELL)name, strlen(name));
	set_xt(x, w, xt);
	return xt; 
}

/* Helper to work with absolute/relative memory addresses */

void _to_abs(X* x) { push(x, pop(x) + x->d); }
void _to_rel(X* x) { push(x, pop(x) - x->d); }

void bootstrap(X* x) {
	comma(x, 0); /* HERE */
	comma(x, 10); /* BASE */
	comma(x, 0); /* LATEST */
	comma(x, 0); /* STATE */
	comma(x, 0); /* IBUF */
	comma(x, 0); /* IPOS */
	comma(x, 0); /* ILEN */
	comma(x, 0); /* SOURCE_ID */
	comma(x, 0); /* DOES */
	comma(x, 0); /* HLD */
	comma(x, 0); /* LATESTXT */
	comma(x, 0); /* IX */
	comma(x, 0); /* JX */
	comma(x, 0); /* KX */
	comma(x, 0); /* LX */

	/* Required primitives not present in ANS Forth */

	code(x, "(LIT)", primitive(x, &_lit));
	code(x, "(RIP)", primitive(x, &_rip));
	code(x, "(COMPILE)", primitive(x, &_compile));
	code(x, "(BRANCH)", primitive(x, &_branch));
	code(x, "(?BRANCH)", primitive(x, &_zbranch));
	code(x, "(STRING)", primitive(x, &_string));
	code(x, "(QUOTATION)", primitive(x, &_quotation));
	code(x, "(DODOES)", primitive(x, &_do_does));

	/* Quotations */

	code(x, "[:", primitive(x, &_start_quotation)); _immediate(x);
	code(x, ";]", primitive(x, &_end_quotation)); _immediate(x);

	/* Loop helper */

	code(x, "DOLOOP", primitive(x, &_doloop));

	/* Commands that can help you start or end work sessions */

	code(x, "ENVIRONMENT?", primitive(x, &_environment_query));

	/* Commands to inspect memory, debug & view code */

	code(x, "DEPTH", primitive(x, &_depth));

	/* Commands that change compilation & interpretation settings */

	code(x, "BASE", primitive(x, &_base));
	code(x, "DECIMAL", primitive(x, &_decimal));

	/* Source code preprocessing, interpreting & auditing commands */

	/* Comment-introducing operations */

	code(x, "(", primitive(x, &_paren)); _immediate(x);

	/* Dynamic memory operations */

	/* String operations */

	code(x, "COUNT", primitive(x, &_count));
	code(x, "FILL", primitive(x, &_fill));
	code(x, "HOLD", primitive(x, &_hold));
	code(x, "MOVE", primitive(x, &_move));
	code(x, ">NUMBER", primitive(x, &_to_number));
	code(x, "<#", primitive(x, &_less_number_sign));
	code(x, "#>", primitive(x, &_number_sign_greater));
	code(x, "#", primitive(x, &_number_sign));
	code(x, "#S", primitive(x, &_number_sign_s));
	code(x, "SIGN", primitive(x, &_sign));

	/* More input/output operations */

	code(x, "ACCEPT", primitive(x, &_accept));
	code(x, "CR", primitive(x, &_cr));
	code(x, ".", primitive(x, &_dot));
	code(x, ".\"", primitive(x, &_dot_quote)); _immediate(x);
	code(x, "EMIT", primitive(x, &_emit));
	code(x, "KEY", primitive(x, &_key));
	code(x, "SPACE", primitive(x, &_space));
	code(x, "SPACES", primitive(x, &_spaces));
	code(x, "TYPE", primitive(x, &_type));
	code(x, "U.", primitive(x, &_u_dot));

	/* Arithmetic and logical operations */

	code(x, "ABS", primitive(x, &_abs));
	code(x, "AND", primitive(x, &_and));
	code(x, "FM/MOD", primitive(x, &_f_m_slash_mod));
	code(x, "INVERT", primitive(x, &_invert));
	code(x, "LSHIFT", primitive(x, &_l_shift));
	code(x, "M*", primitive(x, &_m_star));
	code(x, "MAX", primitive(x, &_max));
	code(x, "MIN", primitive(x, &_min));
	code(x, "-", primitive(x, &_minus));
	code(x, "MOD", primitive(x, &_mod));
	code(x, "*/MOD", primitive(x, &_star_slash_mod));
	code(x, "/MOD", primitive(x, &_slash_mod));
	code(x, "NEGATE", primitive(x, &_negate));
	code(x, "1+", primitive(x, &_one_plus));
	code(x, "1-", primitive(x, &_one_minus));
	code(x, "OR", primitive(x, &_or));
	code(x, "+", primitive(x, &_plus));
	code(x, "+!", primitive(x, &_plus_store));
	code(x, "RSHIFT", primitive(x, &_r_shift));
	code(x, "/", primitive(x, &_slash));
	code(x, "SM/REM", primitive(x, &_s_m_slash_rem));
	code(x, "*", primitive(x, &_star));
	code(x, "*/", primitive(x, &_star_slash));
	code(x, "2*", primitive(x, &_two_star));
	code(x, "2/", primitive(x, &_two_slash));
	code(x, "UM*", primitive(x, &_u_m_star));
	code(x, "UM/MOD", primitive(x, &_u_m_slash_mod));
	code(x, "XOR", primitive(x, &_xor));

	/* Number-type conversion operators */

	code(x, "S>D", primitive(x, &_s_to_d));

	/* Commands to define data structures */

	code(x, "CONSTANT", primitive(x, &_constant));
	code(x, "VARIABLE", primitive(x, &_variable));

	/* Memory-stack transfer operations */

	code(x, "C@", primitive(x, &_c_fetch));
	code(x, "C!", primitive(x, &_c_store));
	code(x, "@", primitive(x, &_fetch));
	code(x, "2@", primitive(x, &_two_fetch));
	code(x, "!", primitive(x, &_store));
	code(x, "2!", primitive(x, &_two_store));

	/* Comparison operations */

	code(x, "=", primitive(x, &_equals));
	code(x, ">", primitive(x, &_greater_than));
	code(x, "<", primitive(x, &_less_than));
	code(x, "U<", primitive(x, &_u_less_than));
	code(x, "0=", primitive(x, &_zero_equals));
	code(x, "0<", primitive(x, &_zero_less_than));

	/* System constants & facilities for generating ASCII values */

	code(x, "BL", primitive(x, &_bl));
	code(x, "CHAR", primitive(x, &_char));
	code(x, "[CHAR]", primitive(x, &_bracket_char)); _immediate(x);

	/* Forming definite loops */

	code(x, "DO", primitive(x, &_do)); _immediate(x);
	code(x, "I", primitive(x, &_i));
	code(x, "J", primitive(x, &_j));
	code(x, "LEAVE", primitive(x, &_leave));
	code(x, "UNLOOP", primitive(x, &_unloop));
	code(x, "LOOP", primitive(x, &_loop)); _immediate(x);
	code(x, "+LOOP", primitive(x, &_plus_loop)); _immediate(x);

	/* Forming indefinite loops (compiling-mode only) */

	code(x, "BEGIN", primitive(x, &_begin)); _immediate(x);
	code(x, "UNTIL", primitive(x, &_until)); _immediate(x);
	code(x, "WHILE", primitive(x, &_while)); _immediate(x);
	code(x, "REPEAT", primitive(x, &_repeat)); _immediate(x);

	/* More facilities for defining routines (compiling-mode only) */

	code(x, "ABORT", primitive(x, &_abort));
	code(x, "ABORT\"", primitive(x, &_abort_quote));
	code(x, ":", primitive(x, &_colon));
	code(x, ";", primitive(x, &_semicolon)); _immediate(x);
	code(x, "EXIT", primitive(x, &_exit_));
	code(x, "IF", primitive(x, &_if)); _immediate(x);
	code(x, "ELSE", primitive(x, &_else)); _immediate(x);
	code(x, "THEN", primitive(x, &_then)); _immediate(x);
	code(x, "[", primitive(x, &_left_bracket)); _immediate(x);
	code(x, "QUIT", primitive(x, &_quit));
	code(x, "RECURSE", primitive(x, &_recurse)); _immediate(x);
	code(x, "]", primitive(x, &_right_bracket)); _immediate(x);
	code(x, "S\"", primitive(x, &_s_quote)); _immediate(x);

	/* Manipulating stack items */

	code(x, "DROP", primitive(x, &_drop));
	code(x, "2DROP", primitive(x, &_two_drop));
	code(x, "DUP", primitive(x, &_dup));
	code(x, "2DUP", primitive(x, &_two_dup));
	code(x, "?DUP", primitive(x, &_question_dup));
	code(x, "OVER", primitive(x, &_over));
	code(x, "2OVER", primitive(x, &_two_over));
	code(x, ">R", primitive(x, &_to_r));
	code(x, "R>", primitive(x, &_r_from));
	code(x, "R@", primitive(x, &_r_fetch));
	code(x, "ROT", primitive(x, &_rot));
	code(x, "SWAP", primitive(x, &_swap));
	code(x, "2SWAP", primitive(x, &_two_swap));

	/* Constructing compiler and interpreter system extensions */

	code(x, "ALIGNED", primitive(x, &_aligned));
	code(x, "ALLOT", primitive(x, &_allot));
	code(x, ">BODY", primitive(x, &_to_body));
	code(x, "C,", primitive(x, &_c_comma));
	code(x, "CELL+", primitive(x, &_cell_plus));
	code(x, "CELLS", primitive(x, &_cells));
	code(x, "CHAR+", primitive(x, &_char_plus));
	code(x, "CHARS", primitive(x, &_chars));
	code(x, ",", primitive(x, &_comma));
	code(x, "CREATE", primitive(x, &_create));
	code(x, "DOES>", primitive(x, &_does)); _immediate(x);
	code(x, "EVALUATE", primitive(x, &_evaluate));
	code(x, "EXECUTE", primitive(x, &_execute));
	code(x, "HERE", primitive(x, &_here));
	code(x, "IMMEDIATE", primitive(x, &_immediate));
	code(x, ">IN", primitive(x, &_to_in));
	code(x, "[']", primitive(x, &_bracket_tick)); _immediate(x);
	code(x, "LITERAL", primitive(x, &_literal)); _immediate(x);
	code(x, "POSTPONE", primitive(x, &_postpone)); _immediate(x);
	code(x, "SOURCE", primitive(x, &_source));
	code(x, "STATE", primitive(x, &_state));
	code(x, "'", primitive(x, &_tick));
	code(x, "WORD", primitive(x, &_word));

	code(x, "FIND", primitive(x, &_find));

	/* Helper */

	code(x, "TO-ABS", primitive(x, &_to_abs));
	code(x, "TO-REL", primitive(x, &_to_rel));
}

/* Helpers to work with files from C */

void include(X* x, char* f) {
	push(x, (CELL)f);
	push(x, strlen(f));
	_included(x);
}

/* Helper REPL */

void repl(X* x) {
	char buf[80];
	while (1) {
		printf("[IN] >> ");
		if (fgets(buf, 80, stdin) != 0) {
			push(x, (CELL)buf);
			push(x, strlen(buf));
			_evaluate(x);
			printf("Data stack: ");	_dot_s(x); printf("\n");
		} else {
			/* TODO: Error */
		}
	}
}

/* ---------------------------------------------------- */
/* -- main -------------------------------------------- */
/* ---------------------------------------------------- */

int main(int argc, char**argv) {
	X* x = malloc(sizeof(X));
	x->p = malloc(sizeof(P));
	x->p->p = malloc(sizeof(F) * PSIZE);
	x->p->last = 0;
	x->p->sz = PSIZE;
	x->d = (CELL)malloc(DSIZE);
	init(x, x->d, DSIZE);

	bootstrap(x);

	if (argc == 1) {
		chdir("../../forth2012-test-suite/src/");
		include(x, "runtests.fth");
	} else {
		repl(x);
	}

	free((void*)x->d);
	free(x->p);
	free(x);
}
