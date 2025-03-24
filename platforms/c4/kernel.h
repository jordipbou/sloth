/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Constants --------------------------------------- */

/* Displacement of counted string buffer from here */
#define CBUF					64	/* Counted string buffer */
#define SBUF1					128	/* First string buffer */
#define SBUF2					256	/* Second string buffer */
#define NBUF					384	/* Pictured numeric output buffer */

/* Relative addresses of variables accessed both from C */
/* and Forth. */

#define HERE					0	
#define BASE					sCELL
#define LATEST				2*sCELL
#define STATE					3*sCELL
#define IBUF					4*sCELL
#define IPOS					5*sCELL
#define ILEN					6*sCELL
#define SOURCE_ID			7*sCELL
#define DOES					8*sCELL
#define HLD						9*sCELL
#define LATESTXT			10*sCELL
#define IX						11*sCELL
#define JX						12*sCELL
#define KX						13*sCELL
#define LX						14*sCELL

/* XTs of primitives that need to be called from C and  */
/* Forth code. */

/* TODO: If using find_word, there's no need to define this */
/* as constants in C. */

#define EXIT					-1
#define LIT						-2
#define RIP						-3
#define COMPILE				-4
#define BRANCH				-5
#define ZBRANCH				-6
#define STRING				-7
#define QUOTATION			-8
#define SQUOTATION		-9
#define EQUOTATION		-10
#define DOLOOP				-11

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */
/* This functions convert from relative to absolute */
/* addresses. */

void set(X* x, CELL a, CELL v) { 
	store(x, to_abs(x, a), v); 
}

CELL get(X* x, CELL a) { 
	return fetch(x, to_abs(x, a)); 
}

void cset(X* x, CELL a, CHAR v) { 
	cstore(x, to_abs(x, a), v); 
}

CHAR cget(X* x, CELL a) { 
	return cfetch(x, to_abs(x, a)); 
}

/* Memory management */

CELL here(X* x) { 
	return get(x, HERE); 
}

void allot(X* x, CELL v) { 
	set(x, HERE, get(x, HERE) + v); 
}

CELL aligned(CELL a) { 
	return (a + (sCELL - 1)) & ~(sCELL - 1); 
}

void align(X* x) { 
	set(x, HERE, (get(x, HERE) + (sCELL - 1)) & ~(sCELL - 1)); 
}

/* Compilation */

void comma(X* x, CELL v) { 
	set(x, here(x), v); allot(x, sCELL); 
}

void ccomma(X* x, CHAR v) { 
	set(x, here(x), v); allot(x, sCHAR); 
}

void compile(X* x, CELL xt) { 
	comma(x, xt); 
}

void literal(X* x, CELL n) { 
	comma(x, LIT); comma(x, n); 
}

/* -- Headers ------------------------------------------ */

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags CHAR					@ NT + 3*sCELL */
/* Namelen CHAR				@ NT + 3*sCELL + sCHAR */
/* Name CHAR*namelen	@ NT + 3*sCELL + 2*sCHAR */

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = here(x); /* NT address */
	comma(x, get(x, LATEST)); /* Store link to latest */
	set(x, LATEST, w); /* Set NT as latest */
	comma(x, 0); /* Reserve space for XT */
	ccomma(x, 0); /* Flags (default flags: 0) */
	ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) ccomma(x, fetch(x, n + i)); /* Name */
	align(x); /* Align XT address */
	set(x, w + sCELL, here(x));
	return w;
}

CELL get_link(X* x, CELL w) { 
	return get(x, w); 
}

CELL get_xt(X* x, CELL w) { 
	return get(x, w + sCELL); 
}

void set_xt(X* x, CELL w, CELL xt) { 
	set(x, w + sCELL, xt); 
}

CHAR get_flags(X* x, CELL w) { 
	return cget(x, w + 2*sCELL); 
}

CELL has_flag(X* x, CELL w, CELL v) { 
	return get_flags(x, w) & v; 
}

CHAR get_namelen(X* x, CELL w) { 
	return cget(x, w + 2*sCELL + sCHAR); 
}
CELL get_name_addr(X* x, CELL w) { 
	return to_abs(x, w + 2*sCELL + 2*sCHAR); 
}

/* Setting flags */

#define HIDDEN				1
#define IMMEDIATE			2

void set_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) | v); 
}
void unset_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) & ~v); 
}

/* -- Primitives -------------------------------------- */

/* Primitives are defined as p_<name> */

void p_exit(X* x) { x->ip = (x->rp > 0) ? rpop(x) : -1; }
void p_lit(X* x) { push(x, op(x)); }
void _rip(X* x) { push(x, to_abs(x, x->ip) + op(x) - sCELL); }

void _compile(X* x) { compile(x, pop(x)); }

void _branch(X* x) { x->ip += op(x) - sCELL; }
void _zbranch(X* x) { x->ip += pop(x) == 0 ? (op(x) - sCELL) : sCELL; }

/* TODO: Shouldn't be aligned? */
void _string(X* x) { CELL l = op(x); push(x, to_abs(x, x->ip)); push(x, l); x->ip = aligned(x->ip + l); }

/* Quotations (not in ANS Forth yet) */

void _quotation(X* x) { CELL d = op(x); push(x, x->ip); x->ip += d; }
void _start_quotation(X* x) {
	CELL s = get(x, STATE);
	set(x, STATE, s <= 0 ? s - 1 : s + 1);
	if (get(x, STATE) == -1) push(x, here(x) + 2*sCELL);
	push(x, get(x, LATESTXT));
	compile(x, QUOTATION);
	push(x, to_abs(x, here(x)));
	comma(x, 0);
	set(x, LATESTXT, here(x));
}

void _end_quotation(X* x) {
	CELL s = get(x, STATE), a = pop(x);
	compile(x, EXIT);
	store(x, a, to_abs(x, here(x)) - a - sCELL);
	set(x, LATESTXT, pop(x));
	set(x, STATE, s < 0 ? s + 1 : s - 1);
}

/* Loop helpers */

void ipush(X* x) { 
	rpush(x, get(x, KX));
	set(x, KX, get(x, JX));
	set(x, JX, get(x, IX));
	set(x, LX, 0);
}
void ipop(X* x) { 
	set(x, LX, 0);
	set(x, IX, get(x, JX));
	set(x, JX, get(x, KX));
	set(x, KX, rpop(x));
}

void _leave(X* x) { set(x, LX, 1); p_exit(x); }
void _unloop(X* x) { 
	set(x, LX, get(x, LX) - 1);
	if (get(x, LX) == -1) {
		set(x, IX, get(x, JX));
		set(x, JX, get(x, KX));
		set(x, KX, rpick(x, 1));
	} else if (get(x, LX) == -2) {
		set(x, IX, get(x, JX));
		set(x, JX, get(x, KX));
		set(x, KX, rpick(x, 3));
	}
}

/* Algorithm for doloop taken from pForth */
/* (pf_inner.c case ID_PLUS_LOOP) */
void _doloop(X* x) {
	CELL q, do_first_loop, l, o, d;
	ipush(x);
	q = pop(x);
	do_first_loop = pop(x);
	set(x, IX, pop(x));
	l = pop(x);

	o = get(x, IX) - l;
	d = 0;

	/* First iteration is executed always on a DO */
	if (do_first_loop == 1) {
		eval(x, q);
		if (get(x, LX) == 0) {
			d = pop(x);
			o = get(x, IX) - l;
			set(x, IX, get(x, IX) + d);
			/* printf("LX == 0 l %ld o %ld d %ld\n", l, o, d); */
		}
	}

	if (!(do_first_loop == 0 && o == 0)) {
		while (((o ^ (o + d)) & (o ^ d)) >= 0 && get(x, LX) == 0) {
			eval(x, q);
			if (get(x, LX) == 0) { /* Avoid pop if we're leaving */
				d = pop(x);
				o = get(x, IX) - l;
				set(x, IX, get(x, IX) + d);
			}
		}
	}

	if (get(x, LX) == 0 || get(x, LX) == 1) { 
		/* Leave case */
		ipop(x);
	} else if (get(x, LX) < 0) {
		/* Unloop case */
		set(x, LX, get(x, LX) + 1);
		rpop(x);
		p_exit(x);
	}
}

/* Parsing input */

/* I would prefer using PARSE-NAME but its not yet */
/* standarized and there's no need to implement two */
/* words with almost the same functionality. */
void _word(X* x) {
	/* The region to store WORD counted strings starts */
	/* at here + CBUF. */
	CHAR c = (CHAR)pop(x);
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	CELL start, end, i;
	/* First, ignore c until not c is found */
	/* The Forth Standard says that if the control character is */
	/* the space (hex 20) then control characters may be treated */
	/* as delimiters. */
	if (c == 32) {
		while (ipos < ilen && cfetch(x, ibuf + ipos) <= c) ipos++;
	} else {
		while (ipos < ilen && cfetch(x, ibuf + ipos) == c) ipos++;
	}
	start = ibuf + ipos;
	/* Next, continue parsing until c is found again */
	if (c == 32) {
		while (ipos < ilen && cfetch(x, ibuf + ipos) > c) ipos++;
	} else {
		while (ipos < ilen && cfetch(x, ibuf + ipos) != c) ipos++;
	}
	end = ibuf + ipos;	
	/* Now, copy it to the counted string buffer */
	cstore(x, to_abs(x, here(x) + CBUF), end - start);
	for (i = 0; i < (end - start); i++) {
		cstore(x, to_abs(x, here(x) + CBUF + sCHAR + i*sCHAR), cfetch(x, start + i*sCHAR));
	}
	push(x, to_abs(x, here(x) + CBUF));
	/* If we are not at the end of the input buffer, */
	/* skip c after the word, but its not part of the counted */
	/* string */
	if (ipos < ilen) ipos++;
	set(x, IPOS, ipos);
}

/* Finding words */

/* Helper function to compare a string and a word's name */
/* without case sensitivity. */
int compare_without_case(X* x, CELL w, CELL t, CELL l) {
	int i;
	if (get_namelen(x, w) != l) return 0;
	for (i = 0; i < l; i++) {
		CHAR a = cfetch(x, t + i);
		CHAR b = cfetch(x, get_name_addr(x, w) + i);
		if (a >= 97 && a <= 122) a -= 32;
		if (b >= 97 && b <= 122) b -= 32;
		if (a != b) return 0;
	}
	return 1;
}

void _find(X* x) {
	/* Let's get the address and length from the counted */
	/* string on the stack. */
	CELL cstring = pop(x);
	CHAR l = cfetch(x, cstring);
	CELL a = cstring + sCHAR;
	/* Let's find the word, starting from LATEST */
	CELL w = get(x, LATEST);
	while (w != 0) {
		if (!has_flag(x, w, HIDDEN) && compare_without_case(x, w, a, l)) break;
		w = get_link(x, w);
	}
	if (w == 0) {
		push(x, cstring);
		push(x, 0);
	} else if (has_flag(x, w, IMMEDIATE)) {
		push(x, get_xt(x, w));
		push(x, 1);
	} else {
		push(x, get_xt(x, w));
		push(x, -1);
	}
}

/* Helper to find words from C */
CELL find_word(X* x, char* name) {
	CHAR l = strlen(name);
	CELL a = (CELL)name;
	CELL w = get(x, LATEST);
	while (w != 0) {
		if (!has_flag(x, w, HIDDEN) && compare_without_case(x, w, a, l)) break;
		w = get_link(x, w);
	}
	return w;
}

/* Outer interpreter */

/* INTERPRET is not an ANS word ??!! */
void _interpret(X* x) {
	CELL nt, flag, n;
	char* tok;
	int tlen;
	char buf[15]; char *endptr;
	while (get(x, IPOS) < get(x, ILEN)) {
		push(x, 32); _word(x);
		tok = (char*)(pick(x, 0) + sCHAR);
		tlen = cfetch(x, pick(x, 0));
		if (tlen == 0) { pop(x); return; }
		_find(x);
		if ((flag = pop(x)) != 0) {
			if (get(x, STATE) == 0
			|| (get(x, STATE) != 0 && flag == 1)) {
				eval(x, pop(x));	
			} else {
				compile(x, pop(x));
			}
		} else {
			CELL temp_base = get(x, BASE);
			pop(x);
			if (tlen == 3 && *tok == '\'' && (*(tok + 2)) == '\'') {
				/* Character literal */
				if (get(x, STATE) == 0)	push(x, *(tok + 1));
				else literal(x, *(tok + 1));
			} else {
				if (*tok == '#') {
					temp_base = 10;
					tlen--;
					tok++;
				}	else if (*tok == '$') {
					temp_base = 16;
					tlen--;
					tok++;
				} else if (*tok == '%') {
					temp_base = 2;
					tlen--;
					tok++;
				}
				strncpy(buf, tok, tlen);
				buf[tlen] = 0;
				n = strtol(buf, &endptr, temp_base);	
				if (*endptr == '\0') {
					if (get(x, STATE) == 0) push(x, n);
					else literal(x, n);
				} else {
					/* TODO Word not found, throw an exception? */
					printf("%.*s ?\n", tlen, tok);
				}
			}
		}
	}
}

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

	/* Required order primitives */

	code(x, "EXIT", primitive(x, &p_exit));
	code(x, "LIT", primitive(x, &_lit));
	code(x, "RIP", primitive(x, &_rip));
	code(x, "COMPILE,", primitive(x, &_compile));
	code(x, "BRANCH", primitive(x, &_branch));
	code(x, "?BRANCH", primitive(x, &_zbranch));
	code(x, "STRING", primitive(x, &_string));

	/* Quotations */

	code(x, "QUOTATION", primitive(x, &_quotation));
	code(x, "[:", primitive(x, &_start_quotation)); _immediate(x);
	code(x, ";]", primitive(x, &_end_quotation)); _immediate(x);

	/* Loop helper */

	code(x, "DOLOOP", primitive(x, &_doloop));

	/* Commands that can help you start or end work sessions */

	code(x, "ENVIRONMENT?", primitive(x, &_environment_q));
	code(x, "UNUSED", primitive(x, &_unused));
	code(x, "WORDS", primitive(x, &_words));
	code(x, "BYE", primitive(x, &_bye));
	code(x, "TIME&DATE", primitive(x, &_time_and_date));

	/* Commands to inspect memory, debug & view code */

	code(x, "DEPTH", primitive(x, &_depth));
	code(x, "LIST", primitive(x, &_list));
	code(x, "DUMP", primitive(x, &_dump));
	code(x, "?", primitive(x, &_question));
	code(x, ".S", primitive(x, &_dot_s));
	code(x, "SEE", primitive(x, &_see));

	/* Commands that change compilation & interpretation settings */

	code(x, "BASE", primitive(x, &_base));
	code(x, "DECIMAL", primitive(x, &_decimal));
	code(x, "FORGET", primitive(x, &_forget));
	code(x, "HEX", primitive(x, &_hex));
	code(x, "MARKER", primitive(x, &_marker));

	code(x, "ALSO", primitive(x, &_also));
	code(x, "DEFINITIONS", primitive(x, &_definitions));
	code(x, "FORTH", primitive(x, &_forth));
	code(x, "FORTH-WORDLIST", primitive(x, &_forth_wordlist));
	code(x, "GET-CURRENT", primitive(x, &_get_current));
	code(x, "GET-ORDER", primitive(x, &_get_order));
	code(x, "ONLY", primitive(x, &_only));
	code(x, "ORDER", primitive(x, &_order));
	code(x, "PREVIOUS", primitive(x, &_previous));
	code(x, "SET-CURRENT", primitive(x, &_set_current));
	code(x, "SET-ORDER", primitive(x, &_set_order));
	code(x, "WORDLIST", primitive(x, &_wordlist));
	
	code(x, "ASSEMBLER", primitive(x, &_assembler));
	code(x, "EDITOR", primitive(x, &_editor));

	/* Source code preprocessing, interpreting & auditing commands */

	code(x, ".(", primitive(x, &_dot_paren)); _immediate(x);
	code(x, "INCLUDE-FILE", primitive(x, &_include_file));
	code(x, "INCLUDED", primitive(x, &_included));
	code(x, "LOAD", primitive(x, &_load));
	code(x, "THRU", primitive(x, &_thru));
	code(x, "[IF]", primitive(x, &_bracket_if));
	code(x, "[ELSE]", primitive(x, &_bracket_else));
	code(x, "[THEN]", primitive(x, &_bracket_then));

	/* Comment-introducing operations */

	code(x, "\\", primitive(x, &_backslash)); _immediate(x);
	code(x, "(", primitive(x, &_paren)); _immediate(x);

	/* Dynamic memory operations */

	code(x, "ALLOCATE", primitive(x, &_allocate));
	code(x, "FREE", primitive(x, &_free));
	code(x, "RESIZE", primitive(x, &_resize));

	/* String operations */

	code(x, "CONVERT", primitive(x, &_convert));
	code(x, "COUNT", primitive(x, &_count));
	code(x, "ERASE", primitive(x, &_erase));
	code(x, "FILL", primitive(x, &_fill));
	code(x, "HOLD", primitive(x, &_hold));
	code(x, "MOVE", primitive(x, &_move));
	code(x, ">NUMBER", primitive(x, &_to_number));
	code(x, "<#", primitive(x, &_less_number_sign));
	code(x, "#>", primitive(x, &_number_sign_greater));
	code(x, "#", primitive(x, &_number_sign));
	code(x, "#S", primitive(x, &_number_sign_s));
	code(x, "SIGN", primitive(x, &_sign));
	code(x, "BLANK", primitive(x, &_blank));
	code(x, "CMOVE", primitive(x, &_cmove));
	code(x, "CMOVE>", primitive(x, &_cmove_up));
	code(x, "COMPARE", primitive(x, &_compare));
	code(x, "SEARCH", primitive(x, &_search));
	code(x, "/STRING", primitive(x, &_slash_string));
	code(x, "-TRAILING", primitive(x, &_dash_trailing));
	code(x, ">FLOAT", primitive(x, &_to_float));
	code(x, "REPRESENT", primitive(x, &_represent));

	/* More input/output operations */

	code(x, "ACCEPT", primitive(x, &_accept));
	code(x, "CR", primitive(x, &_cr));
	code(x, ".", primitive(x, &_dot));
	code(x, ".R", primitive(x, &_dot_r));
	code(x, ".\"", primitive(x, &_dot_quote)); _immediate(x);
	code(x, "EMIT", primitive(x, &_emit));
	code(x, "EXPECT", primitive(x, &_expect));
	code(x, "KEY", primitive(x, &_key));
	code(x, "SPACE", primitive(x, &_space));
	code(x, "SPACES", primitive(x, &_spaces));
	code(x, "TYPE", primitive(x, &_type));
	code(x, "U.", primitive(x, &_u_dot));
	code(x, "U.R", primitive(x, &_u_dot_r));
	code(x, "F.", primitive(x, &_f_dot));
	code(x, "FE.", primitive(x, &_f_e_dot));
	code(x, "FS.", primitive(x, &_f_s_dot));
	code(x, "AT-XY", primitive(x, &_at_x_y));
	code(x, "EKEY", primitive(x, &_e_key));
	code(x, "EKEY>CHAR", primitive(x, &_e_key_to_char));
	code(x, "EKEY?", primitive(x, &_e_key_question));
	code(x, "EMIT?", primitive(x, &_emit_question));
	code(x, "KEY?", primitive(x, &_key_question));
	code(x, "MS", primitive(x, &_ms));
	code(x, "PAGE", primitive(x, &_page));

	/* Arithmetic and logical operations */

	code(x, "ABS", primitive(x, &_abs));
	code(x, "DABS", primitive(x, &_d_abs));
	code(x, "AND", primitive(x, &_and));
	code(x, "FM/MOD", primitive(x, &_f_m_slash_mod));
	code(x, "INVERT", primitive(x, &_invert));
	code(x, "LSHIFT", primitive(x, &_l_shift));
	code(x, "M*", primitive(x, &_m_star));
	code(x, "MAX", primitive(x, &_max));
	code(x, "DMAX", primitive(x, &_d_max));
	code(x, "MIN", primitive(x, &_min));
	code(x, "DMIN", primitive(x, &_d_min));
	code(x, "-", primitive(x, &_minus));
	code(x, "D-", primitive(x, &_d_minus));
	code(x, "MOD", primitive(x, &_mod));
	code(x, "*/MOD", primitive(x, &_star_slash_mod));
	code(x, "/MOD", primitive(x, &_slash_mod));
	code(x, "NEGATE", primitive(x, &_negate));
	code(x, "1+", primitive(x, &_one_plus));
	code(x, "1-", primitive(x, &_one_minus));
	code(x, "OR", primitive(x, &_or));
	code(x, "+", primitive(x, &_plus));
	code(x, "D+", primitive(x, &_d_plus));
	code(x, "M+", primitive(x, &_m_plus));
	code(x, "+!", primitive(x, &_plus_store));
	code(x, "D+!", primitive(x, &_d_plus_store));
	code(x, "RSHIFT", primitive(x, &_r_shift));
	code(x, "/", primitive(x, &_slash));
	code(x, "D/", primitive(x, &_d_slash));
	code(x, "SM/REM", primitive(x, &_s_m_slash_rem));
	code(x, "*", primitive(x, &_star));
	code(x, "*/", primitive(x, &_star_slash));
	code(x, "M*/", primitive(x, &_m_star_slash));
	code(x, "2*", primitive(x, &_two_star));
	code(x, "D2*", primitive(x, &_d_two_star));
	code(x, "2/", primitive(x, &_two_slash));
	code(x, "D2/", primitive(x, &_d_two_slash));
	code(x, "UM*", primitive(x, &_u_m_star));
	code(x, "UM/MOD", primitive(x, &_u_m_slash_mod));
	code(x, "XOR", primitive(x, &_xor));

	code(x, "F*", primitive(x, &_f_star));
	code(x, "F/", primitive(x, &_f_slash));
	code(x, "F+", primitive(x, &_f_plus));
	code(x, "F+!", primitive(x, &_f_plus_store));
	code(x, "FLOOR", primitive(x, &_floor));
	code(x, "FMAX", primitive(x, &_f_max));
	code(x, "FMIN", primitive(x, &_f_min));
	code(x, "FNEGATE", primitive(x, &_f_negate));
	code(x, "FROUND", primitive(x, &_f_round));

	/* Number-type conversion operators */

	code(x, "S>D", primitive(x, &_s_to_d));
	code(x, "D>S", primitive(x, &_d_to_s));
	code(x, "D>F", primitive(x, &_d_to_f));
	code(x, "F>D", primitive(x, &_f_to_d));

	/* Commands to define data structures */

	code(x, "CONSTANT", primitive(x, &_constant));
	code(x, "VALUE", primitive(x, &_value));
	code(x, "VARIABLE", primitive(x, &_variable));

	code(x, "2CONSTANT", primitive(x, &_two_constant));
	code(x, "2VARIABLE", primitive(x, &_two_variable));

	code(x, "FCONSTANT", primitive(x, &_f_constant));
	code(x, "FVARIABLE", primitive(x, &_f_variable));

	code(x, "BUFFER:", primitive(x, &_buffer_colon));

	/* Memory-stack transfer operations */

	code(x, "C@", primitive(x, &_c_fetch));
	code(x, "C!", primitive(x, &_c_store));
	code(x, "@", primitive(x, &_fetch));
	code(x, "2@", primitive(x, &_two_fetch));
	code(x, "!", primitive(x, &_store));
	code(x, "2!", primitive(x, &_two_store));
	code(x, "TO", primitive(x, &_to));

	code(x, "F@", primitive(x, &_f_fetch));
	code(x, "F!", primitive(x, &_f_store));

	/* LOCAL: code(x, "TO", primitive(x, &_to)); */

	/* Comparison operations */

	code(x, "=", primitive(x, &_equals));
	code(x, ">", primitive(x, &_greater_than));
	code(x, "<", primitive(x, &_less_than));
	code(x, "<>", primitive(x, &_not_equals));
	code(x, "U<", primitive(x, &_u_less_than));
	code(x, "U>", primitive(x, &_u_greater_than));
	code(x, "WITHIN", primitive(x, &_within));
	code(x, "0=", primitive(x, &_zero_equals));
	code(x, "0>", primitive(x, &_zero_greater_than));
	code(x, "0<", primitive(x, &_zero_less_than));
	code(x, "0<>", primitive(x, &_zero_not_equals));

	code(x, "F<", primitive(x, &_f_less_than));
	code(x, "F0=", primitive(x, &_f_zero_equals));
	code(x, "F0<", primitive(x, &_f_zero_less_than));

	/* System constants & facilities for generating ASCII values */

	code(x, "BL", primitive(x, &_bl));
	code(x, "CHAR", primitive(x, &_char));
	code(x, "[CHAR]", primitive(x, &_bracket_char)); _immediate(x);
	code(x, "FALSE", primitive(x, &_false));
	code(x, "TRUE", primitive(x, &_true));

	/* Forming definite loops */

	code(x, "DO", primitive(x, &_do)); _immediate(x);
	code(x, "?DO", primitive(x, &_question_do)); _immediate(x);
	code(x, "I", primitive(x, &_i));
	code(x, "J", primitive(x, &_j));
	code(x, "LEAVE", primitive(x, &_leave));
	code(x, "UNLOOP", primitive(x, &_unloop));
	code(x, "LOOP", primitive(x, &_loop)); _immediate(x);
	code(x, "+LOOP", primitive(x, &_plus_loop)); _immediate(x);

	/* Forming indefinite loops (compiling-mode only) */

	code(x, "BEGIN", primitive(x, &_begin)); _immediate(x);
	code(x, "AGAIN", primitive(x, &_again)); _immediate(x);
	code(x, "UNTIL", primitive(x, &_until)); _immediate(x);
	code(x, "WHILE", primitive(x, &_while)); _immediate(x);
	code(x, "REPEAT", primitive(x, &_repeat)); _immediate(x);

	/* More facilities for defining routines (compiling-mode only) */

	code(x, "ABORT", primitive(x, &_abort));
	code(x, "ABORT\"", primitive(x, &_abort_quote));
	code(x, "C\"", primitive(x, &_c_quote));
	code(x, "CASE", primitive(x, &_case));
	code(x, "OF", primitive(x, &_of));
	code(x, "ENDOF", primitive(x, &_endof));
	code(x, "ENDCASE", primitive(x, &_endcase));
	code(x, ":", primitive(x, &_colon));
	code(x, ":NONAME", primitive(x, &_colon_no_name));
	code(x, ";", primitive(x, &_semicolon)); _immediate(x);
	/* Already defined: code(x, "EXIT", primitive(x, &p_exit)); */
	code(x, "IF", primitive(x, &_if)); _immediate(x);
	code(x, "ELSE", primitive(x, &_else)); _immediate(x);
	code(x, "THEN", primitive(x, &_then)); _immediate(x);
	code(x, "[", primitive(x, &_left_bracket)); _immediate(x);
	code(x, "QUIT", primitive(x, &_quit));
	code(x, "RECURSE", primitive(x, &_recurse)); _immediate(x);
	code(x, "]", primitive(x, &_right_bracket)); _immediate(x);
	code(x, "S\"", primitive(x, &_s_quote)); _immediate(x);

	code(x, "CATCH", primitive(x, &_catch));
	code(x, "THROW", primitive(x, &_throw));

	code(x, "CODE", primitive(x, &_code));
	code(x, ";CODE", primitive(x, &_semicolon_code));

	code(x, "(LOCAL)", primitive(x, &_paren_local_paren));
	code(x, "LOCALS", primitive(x, &_locals));

	/* Manipulating stack items */

	code(x, "DROP", primitive(x, &_drop));
	code(x, "2DROP", primitive(x, &_two_drop));
	code(x, "DUP", primitive(x, &_dup));
	code(x, "2DUP", primitive(x, &_two_dup));
	code(x, "?DUP", primitive(x, &_question_dup));
	code(x, "NIP", primitive(x, &_nip));
	code(x, "OVER", primitive(x, &_over));
	code(x, "2OVER", primitive(x, &_two_over));
	code(x, "PICK", primitive(x, &_pick));
	code(x, "2>R", primitive(x, &_two_to_r));
	code(x, ">R", primitive(x, &_to_r));
	code(x, "2R>", primitive(x, &_two_r_from));
	code(x, "R>", primitive(x, &_r_from));
	code(x, "R@", primitive(x, &_r_fetch));
	code(x, "2R@", primitive(x, &_two_r_fetch));
	code(x, "ROLL", primitive(x, &_roll));
	code(x, "ROT", primitive(x, &_rot));
	code(x, "SWAP", primitive(x, &_swap));
	code(x, "2SWAP", primitive(x, &_two_swap));
	code(x, "TUCK", primitive(x, &_tuck));

	code(x, "2ROT", primitive(x, &_two_rot));

	code(x, "FDEPTH", primitive(x, &_f_depth));
	code(x, "FDROP", primitive(x, &_f_drop));
	code(x, "FDUP", primitive(x, &_f_dup));
	code(x, "FOVER", primitive(x, &_f_over));
	code(x, "FROT", primitive(x, &_f_rot));
	code(x, "FSWAP", primitive(x, &_f_swap));

	/* Constructing compiler and interpreter system extensions */

	code(x, "ALIGN", primitive(x, &_align));
	code(x, "FALIGN", primitive(x, &_f_align));
	code(x, "ALIGNED", primitive(x, &_aligned));
	code(x, "FALIGNED", primitive(x, &_f_aligned));
	code(x, "ALLOT", primitive(x, &_allot));
	code(x, ">BODY", primitive(x, &_to_body));
	code(x, "C,", primitive(x, &_c_comma));
	code(x, "CELL+", primitive(x, &_cell_plus));
	code(x, "FLOAT+", primitive(x, &_float_plus));
	code(x, "CELLS", primitive(x, &_cells));
	code(x, "FLOATS", primitive(x, &_floats));
	code(x, "CHAR+", primitive(x, &_char_plus));
	code(x, "CHARS", primitive(x, &_chars));
	code(x, ",", primitive(x, &_comma));
	code(x, "COMPILE,", primitive(x, &_compile_comma));
	code(x, "[COMPILE]", primitive(x, &_bracket_compile));
	code(x, "CREATE", primitive(x, &_create));
	/* NON ANS: Helper compiled by DOES> */
	set(x, DOES, code(x, "DODOES", primitive(x, &_do_does)));
	code(x, "DOES>", primitive(x, &_does)); _immediate(x);
	code(x, "EVALUATE", primitive(x, &_evaluate));
	code(x, "EXECUTE", primitive(x, &_execute));
	code(x, "HERE", primitive(x, &_here));
	code(x, "IMMEDIATE", primitive(x, &_immediate));
	code(x, ">IN", primitive(x, &_to_in));
	code(x, "[']", primitive(x, &_bracket_tick)); _immediate(x);
	code(x, "LITERAL", primitive(x, &_literal)); _immediate(x);
	code(x, "PAD", primitive(x, &_pad));
	code(x, "PARSE", primitive(x, &_parse));
	code(x, "POSTPONE", primitive(x, &_postpone)); _immediate(x);
	/* code(x, "QUERY", primitive(x, &_query)); */
	code(x, "REFILL", primitive(x, &_refill));
	code(x, "RESTORE-INPUT", primitive(x, &_restore_input));
	code(x, "SAVE-INPUT", primitive(x, &_save_input));
	code(x, "SOURCE", primitive(x, &_source));
	code(x, "SOURCE-ID", primitive(x, &_source_id));
	code(x, "SPAN", primitive(x, &_span));
	code(x, "STATE", primitive(x, &_state));
	/* code(x, "TIB", primitive(x, &_tib)); */
	/* code(x, "#TIB", primitive(x, &_number_tib)); */
	code(x, "'", primitive(x, &_tick));
	code(x, "WORD", primitive(x, &_word));

	code(x, "FIND", primitive(x, &_find));
	code(x, "SEARCH-WORDLIST", primitive(x, &_search_wordlist));

	code(x, "SLITERAL", primitive(x, &_s_literal));

	code(x, "2LITERAL", primitive(x, &_two_literal));

	code(x, "FLITERAL", primitive(x, &_f_literal));

	code(x, "BLK", primitive(x, &_blk));

	code(x, "AHEAD", primitive(x, &_ahead));
	code(x, "CS-PICK", primitive(x, &_c_s_pick));
	code(x, "CS-ROLL", primitive(x, &_c_s_roll));

	/* Helper */

	code(x, "TO-ABS", primitive(x, &_to_abs));
	code(x, "TO-REL", primitive(x, &_to_rel));
}


