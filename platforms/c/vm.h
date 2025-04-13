/* ----------------------------------------------------- */
/* ------------------ SLOTH Forth ---------------------- */
/* ----------------------------------------------------- */

#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

/* ----------------------------------------------------- */
/* ---------------- Virtual machine -------------------- */
/* ----------------------------------------------------- */
/* This is the reference implementation of the SLOTH     */
/* Virtual Machine.                                      */
/* ----------------------------------------------------- */
/* This API defines how the virtual machine works and    */
/* allow access to its internals from the host.          */
/* ----------------------------------------------------- */
/* It uses a table of primitives (C functions that can   */
/* be called from Forth) that the bootstrapped           */
/* programming language can use to interact with the     */
/* virtual machine.                                      */
/* ----------------------------------------------------- */

typedef uint8_t CHAR; /* CHARs are always unsigned */
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

#ifndef STACK_SIZE
#define STACK_SIZE 64
#endif

#ifndef RETURN_STACK_SIZE
#define RETURN_STACK_SIZE 64
#endif

#ifndef DSIZE
#define DSIZE 131072
#endif

#ifndef PSIZE
#define PSIZE 512
#endif

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

void init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;

	x->jmpbuf_idx = -1;
}

/* Data stack */

/* TODO: Add checks for stack underflow/overflow */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }
CELL pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

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
	if (x->jmpbuf_idx >= 0) {
		longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
	} else {
		/* If no exception frame has been nested with CATCH */
		/* the system should just go back to the OS. */
		printf(" Exception %ld, ending.\n", e);
		exit(e);
	}
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

#define HERE									0	
#define BASE									sCELL
#define FORTH_WORDLIST				2*sCELL
#define STATE									3*sCELL
#define IBUF									4*sCELL
#define IPOS									5*sCELL
#define ILEN									6*sCELL
#define SOURCE_ID							7*sCELL
#define HLD										8*sCELL
#define LATESTXT							9*sCELL
#define IX										10*sCELL
#define JX										11*sCELL
#define KX										12*sCELL
#define LX										13*sCELL
#define CURRENT								14*sCELL
#define ORDER									15*sCELL
#define CONTEXT								16*sCELL

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

CELL get_latest(X* x) { return fetch(x, get(x, CURRENT)); }
void set_latest(X* x, CELL w) { store(x, get(x, CURRENT), w); }

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
	comma(x, get_latest(x));
	set_latest(x, w);
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

CELL search_word(X* x, CELL n, int l) {
	CELL wl, w, i;
	for (i = 0; i < get(x, ORDER); i++) {
		wl = get(x, CONTEXT + i*sCELL);
		w = fetch(x, wl);
		while (w != 0) {
			if (!has_flag(x, w, HIDDEN) && compare_without_case(x, w, n, l)) return w;
			w = get_link(x, w);
		}
	}
	return 0;
}

void _find(X* x) {
	CELL cstring = pop(x);
	CELL w = search_word(x, cstring + sCHAR, cfetch(x, cstring));
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
	return search_word(x, (CELL)name, strlen(name));
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
