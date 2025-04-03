/* ---------------------------------------------------- */
/* ------------------ SLOTH Forth --------------------- */
/* ---------------------------------------------------- */

#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>

/* This were used by Claude's FM/MOD implementation */
#include <stddef.h>
#include <limits.h>

#include "getch.h"

/* ---------------------------------------------------- */
/* ---------------- Virtual machine ------------------- */
/* ---------------------------------------------------- */
/* This is the reference implementation of the SLOTH    */
/* Virtual Machine.                                     */
/* ---------------------------------------------------- */
/* This API defines how the virtual machine works and   */
/* allow access to its internals from the host.         */
/* ---------------------------------------------------- */
/* It uses a table of primitives (C functions that can  */
/* be called from Forth) that the bootstrapped          */
/* programming language can use to interact with the    */
/* virtual machine.                                     */
/* ---------------------------------------------------- */

typedef int8_t CHAR;
typedef intptr_t CELL;
typedef uintptr_t uCELL;

#if UINTPTR_MAX == UINT64_MAX
	#define hCELL_MASK 0xFFFFFFFF
	#define hCELL_BITS 32
#endif
#if UINTPTR_MAX == UINT32_MAX
	#define hCELL_MASK 0xFFFF
	#define hCELL_BITS 16 
#endif
#if UINTPTR_MAX == UINT16_MAX
	#define hCELL_MASK 0xFF
	#define hCELL_BITS 8
#endif

#define sCELL sizeof(CELL)
#define sCHAR sizeof(CHAR)
#define CELL_BITS sCELL*8

/* Predefined sizes */
/* TODO: they should be modifiable before context creation */
/* or before including this file */
#define STACK_SIZE 64
#define RETURN_STACK_SIZE 64
#define DSIZE 65536
#define PSIZE 512

struct VM;
typedef void (*F)(struct VM*);

typedef struct PRIMITIVES {
	CELL sz;
	CELL last;
	F *p;
} P;

typedef struct VM { 
	CELL s[STACK_SIZE], sp;
	CELL r[RETURN_STACK_SIZE], rp;
	CELL ip;
	CELL d, sz;	/* Dict base address, dict size */

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	P *p;
} X;


/* Pre-definitions */
void _do(X*);
void _question_do(X*);
void _i(X*);
void _j(X*);
void _loop(X*);
void _plus_loop(X*);
void _base(X*);
void _dot_s(X*);

void init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;

	x->jmpbuf_idx = -1;
}

/* Data stack */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }
CELL pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

/* Helpers for double numbers */

CELL lower_half(CELL v) { return ((v) & (((uCELL)1 << (sCELL*4)) - 1)); }

/* Return stack */

void rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL rpop(X* x) { x->rp--; return x->r[x->rp]; }
CELL rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

/* Memory */

/* 
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void store(X* x, CELL a, CELL v) { *((CELL*)a) = v; }
CELL fetch(X* x, CELL a) { return *((CELL*)a); }
void cstore(X* x, CELL a, CHAR v) { *((CHAR*)a) = v; }
CHAR cfetch(X* x, CELL a) { return *((CHAR*)a); }

/*
The next two macros allow transforming from relative to
absolute addresses.
*/
CELL to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL to_rel(X* x, CELL a) { return a - x->d; }

/* Inner interpreter */

CELL op(X* x) { 
	CELL o = fetch(x, to_abs(x, x->ip));
	x->ip += sCELL;
	return o; 
}

void do_prim(X* x, CELL p) { 
	(x->p->p[-1 - p])(x); 
}

void call(X* x, CELL q) { 
	if (x->ip >= 0) rpush(x, x->ip); 
	x->ip = q; 
}

void execute(X* x, CELL q) { 
	if (q < 0) do_prim(x, q); 
	else call(x, q); 
}

void inner(X* x) { 
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) { 
		execute(x, op(x));
	} 
}

void eval(X* x, CELL q) { 
	execute(x, q); 
	if (q > 0) inner(x); 
}

/* Exceptions */

void catch(X* x, CELL q) {
	int tsp = x->sp;
	int trp = x->rp;
	int tip = x->ip;
	int e;

	if (!(e = setjmp(x->jmpbuf[++x->jmpbuf_idx]))) {
		eval(x, q);
		push(x, 0);
	} else {
		x->sp = tsp;
		x->rp = trp;
		x->ip = tip;
		push(x, (CELL)e);
	}

	x->jmpbuf_idx--;
}

void throw(X* x, CELL e) {
	longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
}

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Constants --------------------------------------- */

/* Displacement of counted string buffer from here */
#define CBUF					64	/* Counted string buffer */
#define SBUF1					128	/* First string buffer */
#define SBUF2					256	/* Second string buffer */
#define NBUF					384	/* Pictured numeric output buffer */
#define PAD						416 /* PAD */

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
/* #define DOES					8*sCELL */
#define HLD						8*sCELL
#define LATESTXT			9*sCELL
#define IX						10*sCELL
#define JX						11*sCELL
#define KX						12*sCELL
#define LX						13*sCELL

/* Word statuses */

#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANT				4

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v) { store(x, to_abs(x, a), v); }
CELL get(X* x, CELL a) { return fetch(x, to_abs(x, a)); }

void cset(X* x, CELL a, CHAR v) { cstore(x, to_abs(x, a), v); }
CHAR cget(X* x, CELL a) { return cfetch(x, to_abs(x, a)); }

/* Memory management */

CELL here(X* x) { return get(x, HERE); }
void allot(X* x, CELL v) { set(x, HERE, get(x, HERE) + v); }
CELL aligned(CELL a) { return (a + (sCELL - 1)) & ~(sCELL - 1); }
void align(X* x) { 
	set(x, HERE, (get(x, HERE) + (sCELL - 1)) & ~(sCELL - 1)); 
}

/* Compilation */

void comma(X* x, CELL v) { set(x, here(x), v); allot(x, sCELL); }
void ccomma(X* x, CHAR v) { set(x, here(x), v); allot(x, sCHAR); }

void compile(X* x, CELL xt) { comma(x, xt); }

/* Pre-definition */ CELL find_word(X*, char*);
/* Pre-definition */ CELL get_xt(X*, CELL);

void literal(X* x, CELL n) { 
	comma(x, get_xt(x, find_word(x, "(LIT)")));
	comma(x, n); 
}

/* Headers */

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

CELL get_link(X* x, CELL w) { return get(x, w); }

CELL get_xt(X* x, CELL w) { return get(x, w + sCELL); }
void set_xt(X* x, CELL w, CELL xt) { set(x, w + sCELL, xt); }

CHAR get_flags(X* x, CELL w) { return cget(x, w + 2*sCELL); }
CELL has_flag(X* x, CELL w, CELL v) { return get_flags(x, w) & v; }

CHAR get_namelen(X* x, CELL w) { 
	return cget(x, w + 2*sCELL + sCHAR); 
}
CELL get_name_addr(X* x, CELL w) { 
	return to_abs(x, w + 2*sCELL + 2*sCHAR); 
}

/* Setting flags */

#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANT				4

void set_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) | v); 
}
void unset_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) & ~v); 
}

/* -- Primitives -------------------------------------- */

/* Primitives are defined as p_<name> */

void p_exit(X* x) { x->ip = (x->rp > 0) ? rpop(x) : -1; }
void _lit(X* x) { push(x, op(x)); }
void _rip(X* x) { push(x, to_abs(x, x->ip) + op(x) - sCELL); }

void _compile(X* x) { compile(x, pop(x)); }

void _branch(X* x) { x->ip += op(x) - sCELL; }
void _zbranch(X* x) { x->ip += pop(x) == 0 ? (op(x) - sCELL) : sCELL; }

void _string(X* x) { 
	CELL l = op(x); 
	push(x, to_abs(x, x->ip)); 
	push(x, l); 
	x->ip = aligned(x->ip + l); 
}

void _c_string(X* x) { 
	CHAR l = cfetch(x, to_abs(x, x->ip)); 
	push(x, to_abs(x, x->ip)); 
	x->ip = aligned(x->ip + l + 1);
}

/* Quotations (not in ANS Forth yet) */

void _quotation(X* x) { CELL d = op(x); push(x, x->ip); x->ip += d; }
void _start_quotation(X* x) {
	CELL s = get(x, STATE);
	set(x, STATE, s <= 0 ? s - 1 : s + 1);
	if (get(x, STATE) == -1) push(x, here(x) + 2*sCELL);
	push(x, get(x, LATESTXT));
	compile(x, get_xt(x, find_word(x, "(QUOTATION)")));
	push(x, to_abs(x, here(x)));
	comma(x, 0);
	set(x, LATESTXT, here(x));
}

void _end_quotation(X* x) {
	CELL s = get(x, STATE), a = pop(x);
	compile(x, get_xt(x, find_word(x, "EXIT")));
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
					/* printf("%.*s ?\n", tlen, tok); */
					throw(x, -13);
				}
			}
		}
	}
}

/* Commands that can help you start or end work sessions */

void _environment_q(X* x) { 
	CELL u = pop(x);
	CHAR *s = (CHAR *)pop(x);
	/*
	if (compare_no_case(x, s, u, "/COUNTED-STRING")) {
		push(x, COUNTED_STRING_MAX_SIZE);
	} else if (compare_no_case(x, s, u, "/HOLD")) {
		push(x, NUMERIC_OUTPUT_STRING_BUFFER_SIZE);
	} else if (compare_no_case(x, s, u, "/PAD")) {
		push(x, SCRATCH_AREA_SIZE);
	} else if (compare_no_case(x, s, u, "ADDRESS-UNIT-BITS")) {
		push(x, BITS_PER_ADDRESS_UNIT);
	} else if (compare_no_case(x, s, u, "FLOORED")) {
		push(x, FLOORED_DIVISION_AS_DEFAULT);
	} else if (compare_no_case(x, s, u, "MAX-CHAR")) {
		push(x, MAX_VALUE_FOR_CHAR);
	} else if (compare_no_case(x, s, u, "MAX-D")) {
		push(x, LARGEST_USABLE_DOUBLE_NUMBER);
	} else if (compare_no_case(x, s, u, "MAX-N")) {
		push(x, LARGEST_USABLE_SIGNED_INTEGER);
	} else if (compare_no_case(x, s, u, "MAX-U")) {
		push(x, LARGEST_USABLE_UNSIGNED_INTEGER);
	} else if (compare_no_case(x, s, u, "MAX-UD")) {
		push(x, LARGEST_USABLE_UNSIGNED_DOUBLE_NUMBER);
	} else if (compare_no_case(x, s, u, "RETURN-STACK-CELLS")) {
		push(x, RETURN_STACK_SIZE);
	} else if (compare_no_case(x, s, u, "STACK-CELLS")) {
		push(x, STACK_SIZE);
	}
	*/
}
void _unused(X* x) { push(x, x->sz - get(x, HERE)); }
void _words(X* x) { /* TODO */ }
void _bye(X* x) { exit(0); }
void _time_and_date(X* x) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	push(x, tm->tm_sec);
	push(x, tm->tm_min);
	push(x, tm->tm_hour);
	push(x, tm->tm_mday);
	push(x, tm->tm_mon + 1);
	push(x, tm->tm_year + 1900);
}

/* Commands to inspect memory, debug & view code */

void _depth(X* x) { push(x, x->sp); }

/* Commands that change compilation & interpretation settings */

void _forget(X* x) { /* Obsolescent, don't implement */ }

void _also(X* x) { /* TODO */ }
void _definitions(X* x) { /* TODO */ }
void _forth(X* x) { /* TODO */ }
void _forth_wordlist(X* x) { /* TODO */ }
void _get_current(X* x) { /* TODO */ }
void _get_order(X* x) { /* TODO */ }
void _only(X* x) { /* TODO */ }
void _order(X* x) { /* TODO */ }
void _previous(X* x) { /* TODO */ }
void _set_current(X* x) { /* TODO */ }
void _set_order(X* x) { /* TODO */ }
void _wordlist(X* x) { /* TODO */ }

void _assembler(X* x) { /* TODO */ }
void _editor(X* x) { /* TODO */ }

/* Source code preprocessing, interpreting & auditing commands */

void _include_file(X* x) { /* TODO */ }
void _included(X* x) {
	FILE *f;
	char filename[1024];
	char linebuf[1024];
	CELL INTERPRET, e;

	CELL previbuf = get(x, IBUF);
	CELL previpos = get(x, IPOS);
	CELL previlen = get(x, ILEN);

	CELL prevsourceid = get(x, SOURCE_ID);

	CELL l = pop(x);
	CELL a = pop(x);

	strncpy(filename, (char*)a, (size_t)l);
	filename[l] = 0;

	f = fopen(filename, "r");

	if (f) {
		INTERPRET = get_xt(x, find_word(x, "INTERPRET"));

		set(x, SOURCE_ID, (CELL)f);

		while (fgets(linebuf, 1024, f)) {
			set(x, IBUF, (CELL)linebuf);
			set(x, IPOS, 0);
			set(x, ILEN, strlen(linebuf));

			catch(x, INTERPRET);
			if ((e = pop(x)) != 0) {
				printf("Exception %ld on line:\n", e);
				printf("%s", linebuf);
				break;
			}
		}

		set(x, SOURCE_ID, prevsourceid);

		fclose(f);
	} else {
		printf("ERROR: Can't open file (%.*s)\n", (int)l, (char*)a);
	}

	set(x, IBUF, previbuf);
	set(x, IPOS, previpos);
	set(x, ILEN, previlen);
}
void _load(X* x) { /* TODO */ }
void _thru(X* x) { /* TODO */ }
void _bracket_if(X* x) { /* TODO */ }
void _bracket_else(X* x) { /* TODO */ }
void _bracket_then(X* x) { /* TODO */ }

/* Dynamic memory operations */

void _allocate(X* x) { /* TODO */ }
void _free(X* x) { /* TODO */ }
void _resize(X* x) { /* TODO */ }

/* String operations */

void _erase(X* x) { /* TODO */ }
void _move(X* x) {
	CELL u = pop(x);
	CELL addr2 = pop(x);
	CELL addr1 = pop(x);
	CELL i;
	if (addr1 >= addr2) {
		for (i = 0; i < u; i++) {
			cstore(x, addr2 + i, cfetch(x, addr1 + i));
		}
	} else {
		for (i = u - 1; i >= 0; i--) {
			cstore(x, addr2 + i, cfetch(x, addr1 + i));
		}
	}
}
void _blank(X* x) { /* TODO */ }

void _to_float(X* x) { /* TODO */ }
void _represent(X* x) { /* TODO */ }

/* Disk input/output operations using files or block buffers */

void _block(X* x) { /* TODO */ }
void _buffer(X* x) { /* TODO */ }
void _empty_buffers(X* x) { /* TODO */ }
void _flush(X* x) { /* TODO */ }
void _save_buffers(X* x) { /* TODO */ }
void _scr(X* x) { /* TODO */ }
void _update(X* x) { /* TODO */ }

void _bin(X* x) { /* TODO */ }
void _close_file(X* x) { /* TODO */ }
void _create_file(X* x) { /* TODO */ }
void _delete_file(X* x) { /* TODO */ }
void _file_position(X* x) { /* TODO */ }
void _file_size(X* x) { /* TODO */ }
void _file_status(X* x) { /* TODO */ }
void _flush_file(X* x) { /* TODO */ }
void _open_file(X* x) { /* TODO */ }
void _r_o(X* x) { /* TODO */ }
void _r_w(X* x) { /* TODO */ }
void _read_file(X* x) { /* TODO */ }
void _read_line(X* x) { /* TODO */ }
void _rename_file(X* x) { /* TODO */ }
void _reposition_file(X* x) { /* TODO */ }
void _resize_file(X* x) { /* TODO */ }
void _w_o(X* x) { /* TODO */ }
void _write_file(X* x) { /* TODO */ }
void _write_line(X* x) { /* TODO */ }

/* More input/output operations */

/* Pre-defined */ void _dup(X*);
/* Pre-defined */ void _key(X*);
/* Pre-defined */ void _emit(X*);
/* TODO: It would be interesting to manage at least backspace */
void _accept(X* x) {
	CELL n = pop(x);
	CELL addr = pop(x);
	CELL i;
	CHAR c;
	for (i = 0; i < n; i++) {
		_key(x);
		_dup(x); _emit(x);
		c = (CHAR)pop(x);
		if (c < 32) { break; }
		cstore(x, addr + i, c);
	}
	push(x, i);
}
void _dot(X* x) { printf("%ld ", pop(x)); }
void _dot_r(X* x) { /* TODO */ }
void _emit(X* x) { printf("%c", (CHAR)pop(x)); }
void _expect(X* x) { /* TODO */ }
void _key(X* x) { push(x, getch()); }

void _u_dot(X* x) { printf("%lu ", (uCELL)pop(x)); }
void _u_dot_r(X* x) { /* TODO */ }

void _f_dot(X* x) { /* TODO */ }
void _f_e_dot(X* x) { /* TODO */ }
void _f_s_dot(X* x) { /* TODO */ }

void _at_x_y(X* x) { /* TODO */ }
void _e_key(X* x) { /* TODO */ }
void _e_key_to_char(X* x) { /* TODO */ }
void _e_key_question(X* x) { /* TODO */ }
void _emit_question(X* x) { /* TODO */ }
void _key_question(X* x) { /* TODO */ }
void _ms(X* x) { /* TODO */ }
void _page(X* x) { /* TODO */ }

/* Arithmetic and logical operations */

void _d_abs(X* x) { /* TODO */ }
void _and(X* x) { CELL v = pop(x); push(x, pop(x) & v); }
void _f_m_slash_mod(X* x) {
	CELL n1 = pop(x), d1_hi = pop(x), d1_lo = pop(x);

	uCELL q, r;
    
  /* Track signs */
  CELL d_neg = (d1_hi < 0);
  CELL n_neg = (n1 < 0);
    
  /* Use unsigned arithmetic for the algorithm */
  uCELL abs_n1 = n_neg ? (uCELL)-n1 : (uCELL)n1;
  uCELL abs_d_hi, abs_d_lo;
    
  /* Get absolute value of double-word dividend */
  if (d_neg) {
    /* Two's complement negation for double word */
    abs_d_lo = (uCELL)-d1_lo;
    abs_d_hi = (uCELL)(~d1_hi + (abs_d_lo == 0));
  } else {
    abs_d_hi = (uCELL)d1_hi;
    abs_d_lo = (uCELL)d1_lo;
  }
    
  /* Initialize quotient and remainder */
  q = 0;
  r = 0;
    
  /* Process 128 bits of the dividend (64 high bits + 64 low bits) 
  * using the standard long division algorithm
  */
    
  /* First the high bits */
  {
    CELL i;
    for (i = sCELL * CHAR_BIT - 1; i >= 0; i--) {
      /* Shift remainder left by 1 bit and bring in next bit of dividend */
      r = (r << 1) | ((abs_d_hi >> i) & 1);
            
      /* If remainder >= divisor, subtract and set quotient bit */
      if (r >= abs_n1) {
        r -= abs_n1;
        q |= (1UL << i);
      }
    }
  }
    
  /* Then the low bits */
  {
    CELL i;
    for (i = sCELL * CHAR_BIT - 1; i >= 0; i--) {
      /* Shift remainder left by 1 bit and bring in next bit of dividend */
      r = (r << 1) | ((abs_d_lo >> i) & 1);
            
      /* If remainder >= divisor, subtract and set quotient bit */
      if (r >= abs_n1) {
        r -= abs_n1;
        q |= (1UL << i);
      }
    }
  }
    
  /* Apply sign to results */
  {
    CELL quotient = (d_neg != n_neg) ? -(CELL)q : q;
    CELL remainder = d_neg ? -(CELL)r : r;
    
    /* Adjust for floored division semantics */
    if ((d_neg != n_neg) && remainder != 0) {
        quotient--;
        remainder += n1;
    }
    
    /* Push results back onto the stack */
		push(x, remainder);
		push(x, quotient);
  }
}


void _invert(X* x) { push(x, ~pop(x)); }
void _l_shift(X* x) { CELL n = pop(x); push(x, pop(x) << n); }
/* Code for _m_star has been created by claude.ai */
void _m_star(X* x) {
	CELL b = pop(x), a = pop(x), high, low;

	/* Convert the signed 64-bit integers to unsigned */
	/* for bit manipulation */
	uCELL ua = (uCELL)a;
	uCELL ub = (uCELL)b;

	/* Compute the full 128-bit product using 32 bit pieces */
	uCELL a_low = ua & hCELL_MASK;
	uCELL a_high = ua >> hCELL_BITS;
	uCELL b_low = ub & hCELL_MASK;
	uCELL b_high = ub >> hCELL_BITS;

	/* Multiply the components */
	uCELL low_low = a_low * b_low;
	uCELL low_high = a_low * b_high;
	uCELL high_low = a_high * b_low;
	uCELL high_high = a_high * b_high;

	/* Combine the partial product */
	uCELL carry = ((low_low >> hCELL_BITS) + (low_high & hCELL_MASK) + (high_low & hCELL_MASK)) >> hCELL_BITS;

	/* Calculate the low 64 bits of the result */
	low = (uCELL)(low_low + (low_high << hCELL_BITS) + (high_low << hCELL_BITS));

	/* Calculate the high 64 bits of the result */
	high = (uCELL)(high_high + (low_high >> hCELL_BITS) + (high_low >> hCELL_BITS) + carry);

	/* Adjust for sign */
	if (a < 0) high -= b;
	if (b < 0) high -= a;

	push(x, low);
	push(x, high);
}
void _d_max(X* x) { /* TODO */ }
void _d_min(X* x) { /* TODO */ }
void _minus(X* x) { CELL a = pop(x); push(x, pop(x) - a); }
void _d_minus(X* x) { /* TODO */ }
/* Pre-definition */ void _s_m_slash_rem(X*);
/* Pre-definition */ void _to_r(X*);
/* Pre-definition */ void _r_from(X*);
void _star_slash_mod(X* x) { _to_r(x); _m_star(x); _r_from(x); _s_m_slash_rem(x); }
void _d_negate(X* x) { /* TODO */ }
void _plus(X* x) { CELL a = pop(x); push(x, pop(x) + a); }
/* Code adapted from pForth */
void _d_plus(X* x) { 
	uCELL ah, al, bl, bh, sh, sl;
	bh = (uCELL)pop(x);
	bl = (uCELL)pop(x);
	ah = (uCELL)pop(x);
	al = (uCELL)pop(x);
	sh = 0;
	sl = al + bl;
	if (sl < bl) sh = 1;	/* carry */
	sh += ah + bh;
	push(x, sl);
	push(x, sh);
}
void _m_plus(X* x) { /* TODO */ }
void _d_plus_store(X* x) { /* TODO */ }
void _r_shift(X* x) { CELL n = pop(x); push(x, ((uCELL)pop(x)) >> n); }
void _d_slash(X* x) { /* TODO */ }
void _star(X* x) { CELL b = pop(x); push(x, pop(x) * b); }
/* Pre-definition */ void _nip(X*);
void _m_star_slash(X* x) { /* TODO */ }
void _d_two_star(X* x) { /* TODO */ }
void _two_slash(X* x) { push(x, pop(x) >> 1); }
void _d_two_slash(X* x) { /* TODO */ }
void _u_m_star(X* x) {
	uCELL b = (uCELL)pop(x), a = (uCELL)pop(x), high, low;

	/* Split each 64-bit integer into 32-bit pieces for multiplication */
	uCELL a_low = a & hCELL_MASK;
	uCELL a_high = a >> hCELL_BITS;
	uCELL b_low = b & hCELL_MASK;
	uCELL b_high = b >> hCELL_BITS;
	
	/* Multiply the 32-bit components */
	uCELL low_low = a_low * b_low;
	uCELL low_high = a_low * b_high;
	uCELL high_low = a_high * b_low;
	uCELL high_high = a_high * b_high;

	uCELL carry; /* Pre-definition */

	/* Intermediate values for calculating the carries */
	uCELL mid = low_low >> hCELL_BITS;
	mid += low_high & hCELL_MASK;
	mid += high_low & hCELL_MASK;
	
	/* Calculate carry for the high part */
	carry = mid >> hCELL_BITS;
	
	/* Calculate the low 64 bits of the result */
	low = (mid << hCELL_BITS) | (low_low & hCELL_MASK);
	
	/* Calculate the high 64 bits of the result */
	high = high_high + (low_high >> hCELL_BITS) + (high_low >> hCELL_BITS) + carry;

	push(x, low);
	push(x, high);
}
/* UM/MOD code taken from pForth */
#define DULT(du1l,du1h,du2l,du2h) ( (du2h<du1h) ? 0 : ( (du2h==du1h) ? (du1l<du2l) : 1) )
void _u_m_slash_mod(X* x) {
	uCELL ah, al, q, di, bl, bh, sl, sh;
	bh = (uCELL)pop(x);
	bl = 0;
	ah = (uCELL)pop(x);
	al = (uCELL)pop(x);
	q = 0;
	for( di=0; di<CELL_BITS; di++ )
	{
	    if( !DULT(al,ah,bl,bh) )
	    {
	        sh = 0;
	        sl = al - bl;
	        if( al < bl ) sh = 1; /* Borrow */
	        sh = ah - bh - sh;
	        ah = sh;
	        al = sl;
	        q |= 1;
	    }
	    q = q << 1;
	    bl = (bl >> 1) | (bh << (CELL_BITS-1));
	    bh = bh >> 1;
	}
	if( !DULT(al,ah,bl,bh) )
	{
	    al = al - bl;
	    q |= 1;
	}
	push(x, al); /* rem */
	push(x, q);
}

void _f_star(X* x) { /* TODO */ }
void _f_slash(X* x) { /* TODO */ }
void _f_plus(X* x) { /* TODO */ }
void _f_plus_store(X* x) { /* TODO */ }
void _floor(X* x) { /* TODO */ }
void _f_max(X* x) { /* TODO */ }
void _f_min(X* x) { /* TODO */ }
void _f_negate(X* x) { /* TODO */ }
void _f_round(X* x) { /* TODO */ }

/* Number-type conversion operators */

void _s_to_d(X* x) { push(x, pick(x, 0) < 0 ? -1 : 0); }

void _d_to_s(X* x) { /* TODO */ }

void _d_to_f(X* x) { /* TODO */ }
void _f_to_d(X* x) { /* TODO */ }

/* Commands to define data structures */

void _two_constant(X* x) { /* TODO */ }
void _two_variable(X* x) { /* TODO */ }

void _f_constant(X* x) { /* TODO */ }
void _f_variable(X* x) { /* TODO */ }

/* Memory-stack transfer operations */

void _c_fetch(X* x) { push(x, cfetch(x, pop(x))); }
void _c_store(X* x) { CELL a = pop(x); cstore(x, a, pop(x)); }
void _fetch(X* x) { push(x, fetch(x, pop(x))); }
void _store(X* x) { CELL a = pop(x); store(x, a, pop(x)); }

void _f_fetch(X* x) { /* TODO */ }
void _f_store(X* x) { /* TODO */ }

/* LOCAL (TODO): void _to(X* x) { } */

/* Comparison operations */

void _equals(X* x) { CELL a = pop(x); push(x, pop(x) == a ? -1 : 0); }
void _less_than(X* x) { CELL a = pop(x); push(x, pop(x) < a ? -1 : 0); }

void _f_less_than(X* x) { /* TODO */ }
void _f_zero_equals(X* x) { /* TODO */ }
void _f_zero_less_than(X* x) { /* TODO */ }

/* System constants & facilities for generating ASCII values */

void _char(X* x) {
	push(x, 32); _word(x);
	push(x, cfetch(x, pop(x) + 1));
}
void _bracket_char(X* x) { 
	push(x, 32); _word(x);
	literal(x, cfetch(x, pop(x) + 1));
}
void _false(X* x) { push(x, 0); }
void _true(X* x) { push(x, -1); }

/* More facilities for defining routines (compiling-mode only) */

void _here(X* x); /* Predefined for if */
void _over(X* x); /* Predefined for then */
void _ahead(X* x); /* Predefined for else */
void _then(X* x); /* Predefined for else */
void _swap(X* x); /* Predefined for swap */

void _abort(X* x) { /* TODO */ }
void _abort_quote(X* x) { /* TODO */ }
void _case(X* x) { /* TODO */ }
void _of(X* x) { /* TODO */ }
void _endof(X* x) { /* TODO */ }
void _endcase(X* x) { /* TODO */ }
void _colon(X* x) {
	CELL tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	set(x, LATESTXT, get_xt(x, get(x, LATEST)));
	set_flag(x, get(x, LATEST), HIDDEN);
	set(x, STATE, 1);
}
void _colon_no_name(X* x) { 
	push(x, here(x));
	set(x, LATESTXT, here(x));
	set(x, STATE, 1);
}
void _semicolon(X* x) {
	compile(x, get_xt(x, find_word(x, "EXIT")));
	set(x, STATE, 0);
	/* Don't change flags for nonames */
	if (get_xt(x, get(x, LATEST)) == get(x, LATESTXT))
		unset_flag(x, get(x, LATEST), HIDDEN);

}

void _recurse(X* x) { compile(x, get(x, LATESTXT)); }
void _catch(X* x) { catch(x, pop(x)); }
void _throw(X* x) { CELL e = pop(x); if (e != 0) throw(x, e); }

void _code(X* x) { /* TODO */ }
void _semicolon_code(X* x) { /* TODO */ }

void _paren_local_paren(X* x) { /* TODO */ }
void _locals(X* x) { /* TODO */ }

/* Manipulating stack items */

void _drop(X* x) { pop(x); }
void _over(X* x) { push(x, pick(x, 1)); }
void _pick(X* x) {  push(x, pick(x, pop(x))); }
void _to_r(X* x) { rpush(x, pop(x)); }
void _r_from(X* x) { push(x, rpop(x)); }
void _swap(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); }

void _two_rot(X* x) { /* TODO */ }

void _f_depth(X* x) { /* TODO */ }
void _f_drop(X* x) { /* TODO */ }
void _f_dup(X* x) { /* TODO */ }
void _f_over(X* x) { /* TODO */ }
void _f_rot(X* x) { /* TODO */ }
void _f_swap(X* x) { /* TODO */ }

/* Constructing compiler and interpreter system extensions */

void _f_align(X* x) { /* TODO */ }
void _f_aligned(X* x) { /* TODO */ }
void _allot(X* x) { allot(x, pop(x)); }
void _float_plus(X* x) { /* TODO */ }
void _cells(X* x) { push(x, pop(x) * sCELL); }
void _floats(X* x) { /* TODO */ }
void _chars(X* x) { /* TODO */ }
void _compile_comma(X* x) { compile(x, pop(x)); }
void _bracket_compile(X* x) { /* TODO */ }
/* CREATE parses the next word in the input buffer, creates */
/* a new header for it and then compiles some code. */
/* The compiled code is 4 CELLS long and has a RIP instruction */
/* a displacement of 4 CELLS and to EXIT instructions. */
/* The RIP instruction will load the address after the last */
/* EXIT instruction onto the stack. That's the address used */
/* by created words. */
/* The first EXIT instruction exists to be replaced with a */
/* call if CREATE DOES> is used. */
/* The last EXIT is the real end of the word. */
void _create(X* x) {
	CELL tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	compile(x, get_xt(x, find_word(x, "(RIP)"))); 
	compile(x, 4*sCELL); 
	compile(x, get_xt(x, find_word(x, "EXIT"))); 
	compile(x, get_xt(x, find_word(x, "EXIT")));
}
/* Helper compiled by DOES> that replaces the first EXIT */
/* compiled by CREATE on the new created word with a call */
/* to the code after the DOES> in the CREATE DOES> word */
void _do_does(X* x) {
	set(x, get_xt(x, get(x, LATEST)) + 2*sCELL, pop(x));
}
void _does(X* x) {
	literal(x, here(x) + 4*sCELL);
	compile(x, get_xt(x, find_word(x, "(DOES)")));
	compile(x, get_xt(x, find_word(x, "EXIT")));
}
void _evaluate(X* x) {
	CELL l = pop(x), a = pop(x);

	CELL previbuf = get(x, IBUF);
	CELL previpos = get(x, IPOS);
	CELL previlen = get(x, ILEN);

	CELL prevsourceid = get(x, SOURCE_ID);

	set(x, SOURCE_ID, -1);

	set(x, IBUF, a);
	set(x, IPOS, 0);
	set(x, ILEN, l);

	_interpret(x);

	set(x, SOURCE_ID, prevsourceid);

	set(x, IBUF, previbuf);
	set(x, IPOS, previpos);
	set(x, ILEN, previlen);
}
void _execute(X* x) { eval(x, pop(x)); }
void _here(X* x) { push(x, to_abs(x, here(x))); }
void _immediate(X* x) { set_flag(x, get(x, LATEST), IMMEDIATE); }
void _to_in(X* x) { push(x, to_abs(x, IPOS)); }
void _postpone(X* x) { 
	CELL i, xt, tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pick(x, 0));
	if (tlen == 0) { pop(x); return; }
	_find(x); 
	i = pop(x);
	xt = pop(x);
	if (i == 0) { 
		return;
	} else if (i == -1) {
		/* Compile the compilation of the normal word */
		literal(x, xt);
		compile(x, get_xt(x, find_word(x, "(COMPILE)")));
	} else if (i == 1) {
		/* Compile the immediate word */

		compile(x, xt);
	}
}
/* void _query(X* x) */
void _refill(X* x) {
	char linebuf[1024];
	switch (get(x, SOURCE_ID)) {
	case -1: 
		push(x, 0);
		break;
	case 0:
		push(x, get(x, IBUF)); push(x, 80);
		_accept(x);
		set(x, ILEN, pop(x));
		set(x, IPOS, 0);
		push(x, -1); 
		break;
	default: 
		if (fgets(linebuf, 1024, (FILE *)get(x, SOURCE_ID))) {
			set(x, IBUF, (CELL)linebuf);
			set(x, IPOS, 0);
			set(x, ILEN, strlen(linebuf));
			push(x, -1);
		} else {
			push(x, 0);
		}
		break;
	}	
}

void _restore_input(X* x) { /* TODO */ }
void _save_input(X* x) { /* TODO */ }
void _source(X* x) { push(x, get(x, IBUF)); push(x, get(x, ILEN)); }
void _span(X* x) { /* TODO */ }
/* void _tib(X* x) */
/* void _number_tib(X* x) */
/* Already defined: void _word(X* x) */
/* Already defined: void _find(X* x) */

void _two_literal(X* x) { /* TODO */ }

void _f_literal(X* x) { /* TODO */ }

/* BLOCK */ void _blk(X* x) { /* TODO */ }

void _c_s_pick(X* x) { /* TODO */ }
void _c_s_roll(X* x) { /* TODO */ }

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
	/* Not needed: comma(x, 0); // DOES */
	comma(x, 0); /* HLD */
	comma(x, 0); /* LATESTXT */
	comma(x, 0); /* IX */
	comma(x, 0); /* JX */
	comma(x, 0); /* KX */
	comma(x, 0); /* LX */

	/* Basic primitives */

	code(x, "EXIT", primitive(x, &p_exit));
	code(x, "(LIT)", primitive(x, &_lit));
	code(x, "(RIP)", primitive(x, &_rip));
	code(x, "(COMPILE)", primitive(x, &_compile));
	code(x, "(BRANCH)", primitive(x, &_branch));
	code(x, "(?BRANCH)", primitive(x, &_zbranch));
	code(x, "(STRING)", primitive(x, &_string));
	code(x, "(CSTRING)", primitive(x, &_c_string));
	code(x, "(QUOTATION)", primitive(x, &_quotation));
	code(x, "(DOES)", primitive(x, &_do_does));
	code(x, "(DOLOOP)", primitive(x, &_doloop));

	/* Quotations */

	code(x, "[:", primitive(x, &_start_quotation)); _immediate(x);
	code(x, ";]", primitive(x, &_end_quotation)); _immediate(x);

	/* Commands that can help you start or end work sessions */

	code(x, "ENVIRONMENT?", primitive(x, &_environment_q));
	/* NEEDED */ code(x, "UNUSED", primitive(x, &_unused));
	code(x, "WORDS", primitive(x, &_words));
	/* NEEDED */ code(x, "BYE", primitive(x, &_bye));
	/* NEEDED */ code(x, "TIME&DATE", primitive(x, &_time_and_date));

	/* Commands to inspect memory, debug & view code */

	/* NEEDED */ code(x, "DEPTH", primitive(x, &_depth));
	/* BLOCK: Not needed code(x, "LIST", primitive(x, &_list)); */
	/* Not needed: code(x, "DUMP", primitive(x, &_dump)); */
	/* Not needed: code(x, "?", primitive(x, &_question)); */
	/* Not needed: code(x, ".S", primitive(x, &_dot_s)); */
	/* Not needed: code(x, "SEE", primitive(x, &_see)); */

	/* Commands that change compilation & interpretation settings */

	/* Not needed: code(x, "BASE", primitive(x, &_base)); */
	/* Not needed: code(x, "DECIMAL", primitive(x, &_decimal)); */
	code(x, "FORGET", primitive(x, &_forget));
	/* Not needed: code(x, "HEX", primitive(x, &_hex)); */
	/* Not needed: code(x, "MARKER", primitive(x, &_marker)); */

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

	/* Not needed: code(x, ".(", primitive(x, &_dot_paren)); _immediate(x); */
	code(x, "INCLUDE-FILE", primitive(x, &_include_file));
	code(x, "INCLUDED", primitive(x, &_included));
	code(x, "LOAD", primitive(x, &_load));
	code(x, "THRU", primitive(x, &_thru));
	code(x, "[IF]", primitive(x, &_bracket_if));
	code(x, "[ELSE]", primitive(x, &_bracket_else));
	code(x, "[THEN]", primitive(x, &_bracket_then));

	/* Comment-introducing operations */

	/* Not needed: code(x, "\\", primitive(x, &_backslash)); _immediate(x); */
	/* Not needed: code(x, "(", primitive(x, &_paren)); _immediate(x); */

	/* Dynamic memory operations */

	code(x, "ALLOCATE", primitive(x, &_allocate));
	code(x, "FREE", primitive(x, &_free));
	code(x, "RESIZE", primitive(x, &_resize));

	/* String operations */

	/* Not needed: code(x, "COUNT", primitive(x, &_count)); */
	code(x, "ERASE", primitive(x, &_erase));
	/* Not needed: code(x, "FILL", primitive(x, &_fill)); */
	/* Not needed: code(x, "HOLD", primitive(x, &_hold)); */
	code(x, "MOVE", primitive(x, &_move));
	/* Not needed: code(x, ">NUMBER", primitive(x, &_to_number)); */
	/* Not needed: code(x, "<#", primitive(x, &_less_number_sign)); */
	/* Not needed: code(x, "#>", primitive(x, &_number_sign_greater)); */
	/* Not needed: code(x, "#", primitive(x, &_number_sign)); */
	/* Not needed: code(x, "#S", primitive(x, &_number_sign_s)); */
	/* Not needed: code(x, "SIGN", primitive(x, &_sign)); */
	code(x, "BLANK", primitive(x, &_blank));
	/* Not needed: code(x, "CMOVE", primitive(x, &_cmove)); */
	/* Not needed: code(x, "CMOVE>", primitive(x, &_cmove_up)); */
	/* Not needed: code(x, "COMPARE", primitive(x, &_compare)); */
	/* Not needed: code(x, "SEARCH", primitive(x, &_search)); */
	/* Not needed: code(x, "/STRING", primitive(x, &_slash_string)); */
	/* Not needed: code(x, "-TRAILING", primitive(x, &_dash_trailing)); */
	code(x, ">FLOAT", primitive(x, &_to_float));
	code(x, "REPRESENT", primitive(x, &_represent));

	/* More input/output operations */

	code(x, "ACCEPT", primitive(x, &_accept));
	/* Not needed: code(x, "CR", primitive(x, &_cr)); */
	code(x, ".", primitive(x, &_dot));
	code(x, ".R", primitive(x, &_dot_r));
	/* Not needed: code(x, ".\"", primitive(x, &_dot_quote)); _immediate(x); */
	code(x, "EMIT", primitive(x, &_emit));
	code(x, "EXPECT", primitive(x, &_expect));
	code(x, "KEY", primitive(x, &_key));
	/* Not needed: code(x, "SPACE", primitive(x, &_space)); */
	/* Not needed: code(x, "SPACES", primitive(x, &_spaces)); */
	/* Not needed: code(x, "TYPE", primitive(x, &_type)); */
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

	/* Not needed: code(x, "ABS", primitive(x, &_abs)); */
	code(x, "DABS", primitive(x, &_d_abs));
	code(x, "AND", primitive(x, &_and));
	code(x, "FM/MOD", primitive(x, &_f_m_slash_mod));
	code(x, "INVERT", primitive(x, &_invert));
	code(x, "LSHIFT", primitive(x, &_l_shift));
	code(x, "M*", primitive(x, &_m_star));
	/* Not needed: code(x, "MAX", primitive(x, &_max)); */
	code(x, "DMAX", primitive(x, &_d_max));
	/* Not needed: code(x, "MIN", primitive(x, &_min)); */
	code(x, "DMIN", primitive(x, &_d_min));
	code(x, "-", primitive(x, &_minus));
	code(x, "D-", primitive(x, &_d_minus));
	/* Not needed: code(x, "MOD", primitive(x, &_mod)); */
	code(x, "*/MOD", primitive(x, &_star_slash_mod));
	/* Not needed: code(x, "/MOD", primitive(x, &_slash_mod)); */
	/* Not needed: code(x, "NEGATE", primitive(x, &_negate)); */
	/* Not needed: code(x, "1+", primitive(x, &_one_plus)); */
	/* Not needed: code(x, "1-", primitive(x, &_one_minus)); */
	/* Not needed: code(x, "OR", primitive(x, &_or)); */
	code(x, "+", primitive(x, &_plus));
	code(x, "D+", primitive(x, &_d_plus));
	code(x, "M+", primitive(x, &_m_plus));
	/* Not needed: code(x, "+!", primitive(x, &_plus_store)); */
	code(x, "D+!", primitive(x, &_d_plus_store));
	code(x, "RSHIFT", primitive(x, &_r_shift));
	/* Not needed: code(x, "/", primitive(x, &_slash)); */
	code(x, "D/", primitive(x, &_d_slash));
	/* Not needed: code(x, "SM/REM", primitive(x, &_s_m_slash_rem)); */
	code(x, "*", primitive(x, &_star));
	/* I needed to put the name as *//* to comment this one */
	/* Not needed: code(x, "*//*", primitive(x, &_star_slash)); */
	code(x, "M*/", primitive(x, &_m_star_slash));
	/* Not needed: code(x, "2*", primitive(x, &_two_star)); */
	code(x, "D2*", primitive(x, &_d_two_star));
	code(x, "2/", primitive(x, &_two_slash));
	code(x, "D2/", primitive(x, &_d_two_slash));
	code(x, "UM*", primitive(x, &_u_m_star));
	code(x, "UM/MOD", primitive(x, &_u_m_slash_mod));
	/* Not needed: code(x, "XOR", primitive(x, &_xor)); */

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

	/* Not needed: code(x, "CONSTANT", primitive(x, &_constant)); */
	/* Not needed: code(x, "VALUE", primitive(x, &_value)); */
	/* Not needed: code(x, "VARIABLE", primitive(x, &_variable)); */

	code(x, "2CONSTANT", primitive(x, &_two_constant));
	code(x, "2VARIABLE", primitive(x, &_two_variable));

	code(x, "FCONSTANT", primitive(x, &_f_constant));
	code(x, "FVARIABLE", primitive(x, &_f_variable));

	/* Not needed: code(x, "BUFFER:", primitive(x, &_buffer_colon)); */

	/* Memory-stack transfer operations */

	code(x, "C@", primitive(x, &_c_fetch));
	code(x, "C!", primitive(x, &_c_store));
	code(x, "@", primitive(x, &_fetch));
	/* Not needed: code(x, "2@", primitive(x, &_two_fetch)); */
	code(x, "!", primitive(x, &_store));
	/* Not needed: code(x, "2!", primitive(x, &_two_store)); */
	/* code(x, "TO", primitive(x, &_to)); */

	code(x, "F@", primitive(x, &_f_fetch));
	code(x, "F!", primitive(x, &_f_store));

	/* LOCAL: code(x, "TO", primitive(x, &_to)); */

	/* Comparison operations */

	code(x, "=", primitive(x, &_equals));
	/* Not needed: code(x, ">", primitive(x, &_greater_than)); */
	code(x, "<", primitive(x, &_less_than));
	/* Not needed: code(x, "<>", primitive(x, &_not_equals)); */
	/* Not needed: code(x, "U<", primitive(x, &_u_less_than)); */
	/* Not needed: code(x, "U>", primitive(x, &_u_greater_than)); */
	/* Not needed: code(x, "WITHIN", primitive(x, &_within)); */
	/* Not needed: code(x, "0=", primitive(x, &_zero_equals)); */
	/* Not needed: code(x, "0>", primitive(x, &_zero_greater_than)); */
	/* Not needed: code(x, "0<", primitive(x, &_zero_less_than)); */
	/* Not needed: code(x, "0<>", primitive(x, &_zero_not_equals)); */

	code(x, "F<", primitive(x, &_f_less_than));
	code(x, "F0=", primitive(x, &_f_zero_equals));
	code(x, "F0<", primitive(x, &_f_zero_less_than));

	/* System constants & facilities for generating ASCII values */

	/* Not needed: code(x, "BL", primitive(x, &_bl)); */
	code(x, "CHAR", primitive(x, &_char));
	code(x, "[CHAR]", primitive(x, &_bracket_char)); _immediate(x);
	code(x, "FALSE", primitive(x, &_false));
	code(x, "TRUE", primitive(x, &_true));

	/* Forming definite loops */

	/* Not needed: code(x, "DO", primitive(x, &_do)); _immediate(x); */
	/* Not needed: code(x, "?DO", primitive(x, &_question_do)); _immediate(x); */
	/* Not needed: code(x, "I", primitive(x, &_i)); */
	/* Not needed: code(x, "J", primitive(x, &_j)); */
	/* Not needed: code(x, "LEAVE", primitive(x, &_leave)); */
	code(x, "UNLOOP", primitive(x, &_unloop));
	/* Not needed: code(x, "LOOP", primitive(x, &_loop)); _immediate(x); */
	/* Not needed: code(x, "+LOOP", primitive(x, &_plus_loop)); _immediate(x); */

	/* Forming indefinite loops (compiling-mode only) */

	/* Not needed: code(x, "BEGIN", primitive(x, &_begin)); _immediate(x); */
	/* Not needed: code(x, "AGAIN", primitive(x, &_again)); _immediate(x); */
	/* Not needed: code(x, "UNTIL", primitive(x, &_until)); _immediate(x); */
	/* Not needed: code(x, "WHILE", primitive(x, &_while)); _immediate(x); */
	/* Not needed: code(x, "REPEAT", primitive(x, &_repeat)); _immediate(x); */

	/* More facilities for defining routines (compiling-mode only) */

	code(x, "ABORT", primitive(x, &_abort));
	code(x, "ABORT\"", primitive(x, &_abort_quote));
	/* Not needed: code(x, "C\"", primitive(x, &_c_quote)); */
	code(x, "CASE", primitive(x, &_case));
	code(x, "OF", primitive(x, &_of));
	code(x, "ENDOF", primitive(x, &_endof));
	code(x, "ENDCASE", primitive(x, &_endcase));
	code(x, ":", primitive(x, &_colon));
	code(x, ":NONAME", primitive(x, &_colon_no_name));
	code(x, ";", primitive(x, &_semicolon)); _immediate(x);
	/* Already defined: code(x, "EXIT", primitive(x, &p_exit)); */
	/* Not needed: code(x, "IF", primitive(x, &_if)); _immediate(x); */
	/* Not needed: code(x, "ELSE", primitive(x, &_else)); _immediate(x); */
	/* Not needed: code(x, "THEN", primitive(x, &_then)); _immediate(x); */
	/* Not needed: code(x, "[", primitive(x, &_left_bracket)); _immediate(x); */
	/* Not needed: code(x, "QUIT", primitive(x, &_quit)); */
	code(x, "RECURSE", primitive(x, &_recurse)); _immediate(x);
	/* Not needed: code(x, "]", primitive(x, &_right_bracket)); _immediate(x); */
	/* Not needed: code(x, "S\"", primitive(x, &_s_quote)); _immediate(x); */

	code(x, "CATCH", primitive(x, &_catch));
	code(x, "THROW", primitive(x, &_throw));

	code(x, "CODE", primitive(x, &_code));
	code(x, ";CODE", primitive(x, &_semicolon_code));

	code(x, "(LOCAL)", primitive(x, &_paren_local_paren));
	code(x, "LOCALS", primitive(x, &_locals));

	/* Manipulating stack items */

	code(x, "DROP", primitive(x, &_drop));
	/* Not needed: code(x, "2DROP", primitive(x, &_two_drop)); */
	/* Not needed: code(x, "DUP", primitive(x, &_dup)); */
	/* Not needed: code(x, "2DUP", primitive(x, &_two_dup)); */
	/* Not needed: code(x, "?DUP", primitive(x, &_question_dup)); */
	/* Not needed: code(x, "NIP", primitive(x, &_nip)); */
	code(x, "OVER", primitive(x, &_over));
	/* Not needed: code(x, "2OVER", primitive(x, &_two_over)); */
	code(x, "PICK", primitive(x, &_pick));
	/* Not needed: code(x, "2>R", primitive(x, &_two_to_r)); */
	code(x, ">R", primitive(x, &_to_r));
	/* Not needed: code(x, "2R>", primitive(x, &_two_r_from)); */
	code(x, "R>", primitive(x, &_r_from));
	/* Not needed: code(x, "R@", primitive(x, &_r_fetch)); */
	/* Not needed: code(x, "2R@", primitive(x, &_two_r_fetch)); */
	/* Not needed: code(x, "ROLL", primitive(x, &_roll)); */
	/* Not needed: code(x, "ROT", primitive(x, &_rot)); */
	code(x, "SWAP", primitive(x, &_swap));
	/* Not needed: code(x, "2SWAP", primitive(x, &_two_swap)); */
	/* Not needed: code(x, "TUCK", primitive(x, &_tuck)); */

	code(x, "2ROT", primitive(x, &_two_rot));

	code(x, "FDEPTH", primitive(x, &_f_depth));
	code(x, "FDROP", primitive(x, &_f_drop));
	code(x, "FDUP", primitive(x, &_f_dup));
	code(x, "FOVER", primitive(x, &_f_over));
	code(x, "FROT", primitive(x, &_f_rot));
	code(x, "FSWAP", primitive(x, &_f_swap));

	/* Constructing compiler and interpreter system extensions */

	/* Not needed: code(x, "ALIGN", primitive(x, &_align)); */
	code(x, "FALIGN", primitive(x, &_f_align));
	/* Not needed: code(x, "ALIGNED", primitive(x, &_aligned)); */
	code(x, "FALIGNED", primitive(x, &_f_aligned));
	code(x, "ALLOT", primitive(x, &_allot));
	/* Not needed: code(x, ">BODY", primitive(x, &_to_body)); */
	/* Not needed: code(x, "C,", primitive(x, &_c_comma)); */
	/* Not needed: code(x, "CELL+", primitive(x, &_cell_plus)); */
	code(x, "FLOAT+", primitive(x, &_float_plus));
	code(x, "CELLS", primitive(x, &_cells));
	code(x, "FLOATS", primitive(x, &_floats));
	/* Not needed: code(x, "CHAR+", primitive(x, &_char_plus)); */
	code(x, "CHARS", primitive(x, &_chars));
	/* Not needed: code(x, ",", primitive(x, &_comma)); */
	code(x, "COMPILE,", primitive(x, &_compile_comma));
	code(x, "[COMPILE]", primitive(x, &_bracket_compile));
	code(x, "CREATE", primitive(x, &_create));
	code(x, "DOES>", primitive(x, &_does)); _immediate(x);
	code(x, "EVALUATE", primitive(x, &_evaluate));
	code(x, "EXECUTE", primitive(x, &_execute));
	code(x, "HERE", primitive(x, &_here));
	code(x, "IMMEDIATE", primitive(x, &_immediate));
	code(x, ">IN", primitive(x, &_to_in));
	/* Not needed: code(x, "[']", primitive(x, &_bracket_tick)); _immediate(x); */
	/* Not needed: code(x, "LITERAL", primitive(x, &_literal)); _immediate(x); */
	/* Not needed: code(x, "PAD", primitive(x, &_pad)); */
	/* Not needed: code(x, "PARSE", primitive(x, &_parse)); */
	code(x, "POSTPONE", primitive(x, &_postpone)); _immediate(x);
	/* code(x, "QUERY", primitive(x, &_query)); */
	code(x, "REFILL", primitive(x, &_refill));
	code(x, "RESTORE-INPUT", primitive(x, &_restore_input));
	code(x, "SAVE-INPUT", primitive(x, &_save_input));
	code(x, "SOURCE", primitive(x, &_source));
	/* Not needed: code(x, "SOURCE-ID", primitive(x, &_source_id)); */
	code(x, "SPAN", primitive(x, &_span));
	/* Not needed: code(x, "STATE", primitive(x, &_state)); */
	/* code(x, "TIB", primitive(x, &_tib)); */
	/* code(x, "#TIB", primitive(x, &_number_tib)); */
	/* Not needed: code(x, "'", primitive(x, &_tick)); */
	code(x, "WORD", primitive(x, &_word));

	code(x, "FIND", primitive(x, &_find));
	/* Not needed: code(x, "SEARCH-WORDLIST", primitive(x, &_search_wordlist)); */

	/* Not needed: code(x, "SLITERAL", primitive(x, &_s_literal)); */

	code(x, "2LITERAL", primitive(x, &_two_literal));

	code(x, "FLITERAL", primitive(x, &_f_literal));

	code(x, "BLK", primitive(x, &_blk));

	/* Not needed: code(x, "AHEAD", primitive(x, &_ahead)); */
	code(x, "CS-PICK", primitive(x, &_c_s_pick));
	code(x, "CS-ROLL", primitive(x, &_c_s_roll));

	/* Helper */

	code(x, "TO-ABS", primitive(x, &_to_abs));
	code(x, "TO-REL", primitive(x, &_to_rel));
	code(x, "INTERPRET", primitive(x, &_interpret));
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
	set(x, IBUF, (CELL)buf);
	set(x, IPOS, 0);
	set(x, ILEN, 80);
	eval(x, get_xt(x, find_word(x, "QUIT")));
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

	include(x, "ans.fth");

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

/* CORE WORDS THAT ARE NOT NEEDED */

/* Manipulating stack items */

void _dup(X* x) { push(x, pick(x, 0)); }
void _rot(X* x) { 
	CELL a = pop(x); 
	CELL b = pop(x); 
	CELL c = pop(x); 
	push(x, b); 
	push(x, a); 
	push(x, c); 
}
void _nip(X* x) { CELL v = pop(x); pop(x); push(x, v); }
void _tuck(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); push(x, a); }
void _roll(X* x) { /* TODO */ }
void _question_dup(X* x) { if (pick(x, 0) != 0) _dup(x); }
void _r_fetch(X* x) { push(x, rpick(x, 0)); }
void _two_drop(X* x) { pop(x); pop(x); }
void _two_dup(X* x) { push(x, pick(x, 1)); push(x, pick(x, 1)); }
void _two_swap(X* x) { 
	CELL a = pop(x); 
	CELL b = pop(x); 
	CELL c = pop(x); 
	CELL d = pop(x); 
	push(x, b); 
	push(x, a); 
	push(x, d); 
	push(x, c); 
}
void _two_to_r(X* x) { 
	CELL a = pop(x); 
	rpush(x, pop(x)); 
	rpush(x, a); 
}
void _two_r_fetch(X* x) { 
	push(x, rpick(x, 1)); 
	push(x, rpick(x, 0)); 
}
void _two_r_from(X* x) { 
	CELL a = rpop(x); 
	push(x, rpop(x)); 
	push(x, a); 
}
void _two_over(X* x) { 
	push(x, pick(x, 3)); 
	push(x, pick(x, 3)); 
}

/* Memory-stack transfer operations */

void _two_store(X* x) {
	CELL a = pop(x);
	store(x, a, pop(x));
	store(x, a + sCELL, pop(x));
}
void _two_fetch(X* x) { 
	CELL a = pop(x);
	push(x, fetch(x, a + sCELL));
	push(x, fetch(x, a));
}

/* Arithmetic and logical operations */

void _mod(X* x) { CELL a = pop(x); push(x, pop(x) % a); }
void _slash(X* x) { CELL a = pop(x); push(x, pop(x) / a); }
void _star_slash(X* x) { _star_slash_mod(x); _nip(x); }
void _slash_mod(X* x) { 
	CELL b = pop(x), a = pop(x);
	push(x, a%b);
	push(x, a/b);
}
void _one_plus(X* x) { push(x, pop(x) + 1); }
void _one_minus(X* x) { push(x, pop(x) - 1); }
void _two_star(X* x) { push(x, 2*pop(x)); }
void _abs(X* x) { CELL v = pop(x); push(x, v < 0 ? (0-v) : v); }
void _negate(X* x) { push(x, 0 - pop(x)); }
void _or(X* x) { CELL a = pop(x); push(x, pop(x) | a); }
void _xor(X* x) { CELL a = pop(x); push(x, pop(x) ^ a); }
void _min(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a < b ? a : b); }
void _max(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a > b ? a : b); }
void _plus_store(X* x) {
	CELL a = pop(x);
	CELL n = pop(x);
	store(x, a, fetch(x, a) + n);
}
void _s_m_slash_rem(X* x) {
	CELL n1 = pop(x), d1_hi = pop(x), d1_lo = pop(x);

	uCELL q, r;
	
	/* Track signs */
	CELL d_neg = (d1_hi < 0);
	CELL n_neg = (n1 < 0);
	
	/* Use unsigned arithmetic for the algorithm */
	uCELL abs_n1 = n_neg ? (uCELL)-n1 : (uCELL)n1;
	uCELL abs_d_hi, abs_d_lo;
	
	/* Get absolute value of double-word dividend */
	if (d_neg) {
		/* Two's complement negation for double word */
		abs_d_lo = (uCELL)-d1_lo;
		abs_d_hi = (uCELL)(~d1_hi + (abs_d_lo == 0));
	} else {
		abs_d_hi = (uCELL)d1_hi;
		abs_d_lo = (uCELL)d1_lo;
	}
	
	/* Initialize quotient and remainder */
	q = 0;
	r = 0;
	
	/* Process 128 bits of the dividend (64 high bits + 64 low bits) 
	 * using the standard long division algorithm
	 */
	
	/* First the high bits */
	{
		CELL i;
		for (i = sCELL * CHAR_BIT - 1; i >= 0; i--) {
			/* Shift remainder left by 1 bit and bring in next bit of dividend */
			r = (r << 1) | ((abs_d_hi >> i) & 1);
			
			/* If remainder >= divisor, subtract and set quotient bit */
			if (r >= abs_n1) {
				r -= abs_n1;
				q |= (1UL << i);
			}
		}
	}
	
	/* Then the low bits */
	{
		CELL i;
		for (i = sCELL * CHAR_BIT - 1; i >= 0; i--) {
			/* Shift remainder left by 1 bit and bring in next bit of dividend */
			r = (r << 1) | ((abs_d_lo >> i) & 1);
			
			/* If remainder >= divisor, subtract and set quotient bit */
			if (r >= abs_n1) {
				r -= abs_n1;
				q |= (1UL << i);
			}
		}
	}
	
	/* Apply sign to results */
	{
		/* For symmetric division (SM/REM):
		 * 1. Quotient sign depends on operand signs (like normal division)
		 * 2. Remainder has the same sign as the dividend
		 * 3. No adjustment needed - we just truncate toward zero
		 */
		CELL quotient = (d_neg != n_neg) ? -(CELL)q : q;
		CELL remainder = d_neg ? -(CELL)r : r;
		
		/* Push results back onto the stack */
		push(x, remainder);
		push(x, quotient);
	}
}

/* Comparison operations */

void _greater_than(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) > a ? -1 : 0); 
}
void _not_equals(X* x) { CELL a = pop(x); push(x, pop(x) != a ? -1 : 0); }
void _zero_equals(X* x) { push(x, pop(x) == 0 ? -1 : 0); }
void _zero_greater_than(X* x) { push(x, pop(x) > 0 ? -1 : 0); }
void _zero_not_equals(X* x) { push(x, pop(x) != 0 ? -1 : 0); }
void _zero_less_than(X* x) { push(x, pop(x) < 0 ? -1 : 0); }
void _u_less_than(X* x) { 
	uCELL b = (uCELL)pop(x);
	uCELL a = (uCELL)pop(x);
	push(x, a < b ? -1 : 0);
}
void _u_greater_than(X* x) { 
	uCELL b = (uCELL)pop(x);
	uCELL a = (uCELL)pop(x);
	push(x, a > b ? -1 : 0);
}
void _within(X* x) { /* TODO */ }

/* Comment-introducing operations */

void _backslash(X* x) { set(x, IPOS, get(x, ILEN)); }
void _paren(X* x) {
	/* TODO */
	/* ( should be able to work multiline if reading from file */
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != ')') {
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, IPOS) != get(x, ILEN))
		set(x, IPOS, get(x, IPOS) + 1);
}

/* Constructing compiler and interpreter system extensions */

void _cell_plus(X* x) { push(x, pop(x) + sCELL); }
void _char_plus(X* x) { push(x, pop(x) + 1); }
void _c_comma(X* x) { ccomma(x, pop(x)); }
void _comma(X* x) { comma(x, pop(x)); }
void _literal(X* x) { literal(x, pop(x)); }
void _aligned(X* x) { /* TODO */ }
void _align(X* x) { /* TODO */ }
void _tick(X* x) {
	CELL tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pick(x, 0));
	if (tlen == 0) { pop(x); return; }
	_find(x);
	pop(x);
	push(x, pop(x));
}
void _bracket_tick(X* x) { _tick(x); literal(x, pop(x)); }
void _ahead(X* x) { 
	compile(x, get_xt(x, find_word(x, "(BRANCH)"))); 
	_here(x); 
	comma(x, 0); 
}
void _to_body(X* x) { push(x, to_abs(x, pop(x) + 4*sCELL)); }
void _to(X* x) { /* TODO */ }
void _parse(X* x) {
	CHAR c = (CHAR)pop(x);
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	push(x, ibuf + ipos);
	while (ipos < ilen && cfetch(x, ibuf + ipos) != c) ipos++;
	push(x, ibuf + ipos - pick(x, 0));
	if (ipos < ilen) ipos++;
	set(x, IPOS, ipos);
}
void _s_literal(X* x) { /* TODO */ }
void _state(X* x) { push(x, to_abs(x, STATE)); }
void _source_id(X* x) { /* TODO */ }
void _pad(X* x) { _here(x); push(x, PAD); _plus(x); }
void _search_wordlist(X* x) { /* TODO */ }

/* More facilities for defining routines (compiling-mode only) */

void _if(X* x) { 
	compile(x, get_xt(x, find_word(x, "(?BRANCH)"))); 
	_here(x); comma(x, 0); 
}
void _then(X* x) { _here(x); _over(x); _minus(x); _swap(x); _store(x); }
void _else(X* x) { _ahead(x); _swap(x); _then(x); }
void _s_quote(X* x) { 
	CELL i;
	/* Parsing */
	CELL l = 0;
	CELL a = get(x, IBUF) + get(x, IPOS);
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != '"') {
		l++;
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, STATE) == 0) {
		/* TODO: Should change between SBUF1 and SBUF2 */
		for (i = 0; i < l; i++) {
			cset(x, here(x) + SBUF1 + i, cfetch(x, a + i));
		}
		push(x, to_abs(x, here(x) + SBUF1));
		push(x, l);
	} else {
		compile(x, get_xt(x, find_word(x, "(STRING)")));
		comma(x, l);
		for (i = a; i < (a + l); i++) ccomma(x, cfetch(x, i));
		align(x);
	}
	if (get(x, IPOS) < get(x, ILEN))
		set(x, IPOS, get(x, IPOS) + 1);
}
void _c_quote(X* x) { /* TODO */ }
void _left_bracket(X* x) { set(x, STATE, 0); }
void _right_bracket(X* x) { set(x, STATE, 1); }
void _quit(X* x) { /* TODO */ }

/* Forming indefinite loops (compiling-mode only) */

void _begin(X* x) { _here(x); }
void _again(X* x) { 
	compile(x, get_xt(x, find_word(x, "(BRANCH)"))); 
	_here(x); _minus(x); _comma(x); 
}
void _until(X* x) { 
	compile(x, get_xt(x, find_word(x, "(?BRANCH)"))); 
	_here(x); _minus(x); _comma(x); 
}
void _while(X* x) { _if(x); _swap(x); }
void _repeat(X* x) { _again(x); _then(x); }

/* Commands to define data structures */

void _variable(X* x) { _create(x); push(x, 0); _comma(x); }
void _constant(X* x) { 
	CELL tok, tlen, v = pop(x);
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	literal(x, v); 
	compile(x, get_xt(x, find_word(x, "EXIT")));
}
void _buffer_colon(X* x) { _create(x); _allot(x); }
void _value(X* x) { /* TODO */ }

/* String operations */

void _count(X* x) { CELL a = pop(x); push(x, a + 1); push(x, cfetch(x, a)); }
void _fill(X* x) { 
	CHAR c = (CHAR)pop(x);
	CELL u = pop(x);
	CELL addr = pop(x);
	if (u > 0) {
		CELL i;
		for (i = 0; i < u; i++) {
			cstore(x, addr + i, c);
		}
	}
}
void _slash_string(X* x) { /* TODO */ }
void _hold(X* x) { 
	set(x, HLD, get(x, HLD) - 1);
	cstore(x, get(x, HLD), pop(x));
}
void _less_number_sign(X* x) {
	set(x, HLD, to_abs(x, here(x) + NBUF));
}
void _number_sign_greater(X* x) { 
	pop(x); pop(x);
	push(x, get(x, HLD));
	push(x, to_abs(x, here(x) + NBUF) - get(x, HLD));
}
/* Code adapted from lbForth */
void _number_sign(X* x) {
	CELL r;
	push(x, 0);
	push(x, get(x, BASE));
	_u_m_slash_mod(x);
	_to_r(x);
	push(x, get(x, BASE));
	_u_m_slash_mod(x);
	_r_from(x);
	_rot(x);
	r = pop(x);
	if (r > 9) r += 7;
	r += 48;
	push(x, r); _hold(x);
}
void _number_sign_s(X* x) {
	do {
		_number_sign(x);
		_two_dup(x);
		_or(x);
		_zero_equals(x);
	} while(pop(x) == 0);
}
void _sign(X* x) { 
	if (pop(x) < 0) {
		push(x, '-'); _hold(x);
	} 
}
/* Helper function for _to_number */
void _digit(X* x) {
	_to_r(x);
	_dup(x); push(x, 'a'); _less_than(x); _zero_equals(x); /* NOT */
	if (pop(x)) {
		push(x, 'a'); _minus(x); push(x, 'A'); _plus(x);
	}
	_dup(x); _dup(x); push(x, 'A'); _one_minus(x); _greater_than(x);
	if (pop(x)) {
		push(x, 'A'); _minus(x); push(x, '9'); _plus(x); _one_plus(x);
	} else {
		_dup(x); push(x, '9'); _greater_than(x);
		if (pop(x)) {
			_drop(x); push(x, 0);
		}
	}
	push(x, '0'); _minus(x);
	_dup(x); _r_from(x); _less_than(x);
	if (pop(x)) {
		_dup(x); _one_plus(x); _zero_greater_than(x);
		if (pop(x)) {
			_nip(x); _true(x);
		} else {
			_drop(x); _false(x);
		}
	} else {
		_drop(x); _false(x);
	}
}
/* Code adapted from pForth Forth code */
void _to_number(X* x) {
	_to_r(x);
	do { /* BEGIN */
		_r_fetch(x); _zero_greater_than(x);
		if (pop(x)) {
			_dup(x); _c_fetch(x); _base(x); _fetch(x);
			_digit(x);
			if (pop(x)) {
				_true(x);
			} else {
				_drop(x); _false(x);
			}
		} else {
			_false(x);
		}
		if (pop(x) == 0) break; /* WHILE */
		_swap(x); _to_r(x);
		_swap(x); _base(x); _fetch(x);
		_u_m_star(x); _drop(x);
		_rot(x); _base(x); _fetch(x);
		_u_m_star(x);
		_d_plus(x);
		_r_from(x); _one_plus(x);
		_r_from(x); _one_minus(x); _to_r(x);
	} while(1);
	_r_from(x);
}
void _cmove(X* x) { /* TODO */ }
void _cmove_up(X* x) { /* TODO */ }
void _dash_trailing(X* x) { /* TODO */ }
void _search(X* x) { /* TODO */ }
void _compare(X* x) { /* TODO */ }

/* System constants & facilities for generating ASCII values */

void _bl(X* x) { push(x, 32); }

/* More input/output operations */

void _cr(X* x) { printf("\n"); }
void _space(X* x) { printf(" "); }
void _spaces(X* x) { 
	CELL i, u = pop(x); 
	for (i = 0; i < u; i++) printf(" ");
}
void _type(X* x) { 
	CELL l = pop(x); 
	CELL a = pop(x);
	CELL i;
	CHAR c;
	for (i = a; i < a + l; i++) {
		c = cfetch(x, i);
		if (c >= 32 && c <= 126) printf("%c", c);
	}
}

/* Commands to inspect memory, debug & view code */

void _dot_s(X* x) { 
	CELL i;
	printf("<%ld> ", x->sp);
	for (i = 0; i < x->sp; i++) {
		printf("%ld ", x->s[i]);
	}
}
void _dump(X* x) { /* TODO */ }
/* BLOCK EXT */ void _list(X* x) { /* Not implemented */ }
void _question(X* x) { printf("%ld ", fetch(x, pop(x))); }
void _see(X* x) { 
	CELL tok, tlen, i, xt, op = 0;
	CELL q = 0;
	CELL EXIT, QUOTATION, LIT;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
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

/* Commands that change compilation & interpretation settings */

void _decimal(X* x) { set(x, BASE, 10); }
void _hex(X* x) { set(x, BASE, 16); }
void _marker(X* x) { /* TODO */ }
void _base(X* x) { push(x, to_abs(x, BASE)); }

/* Forming definite loops */

void _do(X* x) { literal(x, 1); _start_quotation(x); }
void _question_do(X* x) { literal(x, 0); _start_quotation(x); }
void _i(X* x) { push(x, get(x, IX)); }
void _j(X* x) { push(x, get(x, JX)); }
void _leave(X* x) { set(x, LX, 1); p_exit(x); }
/* Already defined: void _unloop(X* x); */
void _loop(X* x) { 
	literal(x, 1); 
	_end_quotation(x); 
	compile(x, get_xt(x, find_word(x, "(DOLOOP)"))); 
}
void _plus_loop(X* x) { 
	_end_quotation(x); 
	compile(x, get_xt(x, find_word(x, "(DOLOOP)"))); 
}

/* More input/output operations */

void _dot_quote(X* x) { 
	CELL u, addr, i;
	push(x, '"'); 
	_parse(x); 
	u = pop(x);
	addr = pop(x);
	compile(x, get_xt(x, find_word(x, "(STRING)")));
	comma(x, u);
	for (i = 0; i < u; i++) {
		ccomma(x, cfetch(x, addr + i));
	}
	align(x);
	compile(x, get_xt(x, find_word(x, "TYPE")));
}

/* Source code preprocessing, interpreting & auditing commands */

void _dot_paren(X* x) { 
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != ')') {
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, IPOS) != get(x, ILEN)) 
		set(x, IPOS, get(x, IPOS) + 1);
}
