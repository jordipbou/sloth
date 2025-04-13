#ifndef SLOTH_HEADER
#define SLOTH_HEADER

/* ----------------------------------------------------- */
/* ------------------ SLOTH Forth ---------------------- */
/* ----------------------------------------------------- */

#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<assert.h>

/* This are used by Claude's SM/REM implementation */
#include <stddef.h>
#include <limits.h> /* for CHAR_BIT */

/* -- getch multiplatform definition ------------------- */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
int getch();
#endif

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

typedef uint8_t uCHAR; /* CHARs are always unsigned */
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
#define suCHAR sizeof(uCHAR)
#define CELL_BITS sCELL*8

#ifndef SLOTH_STACK_SIZE
#define SLOTH_STACK_SIZE 64
#endif

#ifndef SLOTH_RETURN_STACK_SIZE
#define SLOTH_RETURN_STACK_SIZE 64
#endif

struct sloth_VM;
typedef void (*F)(struct sloth_VM*);

typedef struct sloth_PRIMITIVES {
	CELL sz;
	CELL last;
	F *p;
} sloth_P;

typedef struct sloth_VM { 
	CELL s[SLOTH_STACK_SIZE], sp;
	CELL r[SLOTH_RETURN_STACK_SIZE], rp;
	CELL ip;
	CELL d, sz;	/* Dict base address, dict size */

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	sloth_P *p;
} X;

/* -- Context initialization/destruction --------------- */

void sloth_init(X* x, CELL d, CELL sz);
X* sloth_create(int psize, int dsize);
X* sloth_new();
void sloth_free(X* x);

/* -- Data stack --------------------------------------- */

void sloth_push(X* x, CELL v);
CELL sloth_pop(X* x);
CELL sloth_pick(X* x, CELL a);

/* -- Return stack ------------------------------------- */

void sloth_rpush(X* x, CELL v);
CELL sloth_rpop(X* x);
CELL sloth_rpick(X* x, CELL a);

/* -- Memory ------------------------------------------- */

void sloth_store(X* x, CELL a, CELL v);
CELL sloth_fetch(X* x, CELL a);
void sloth_cstore(X* x, CELL a, uCHAR v);
uCHAR sloth_cfetch(X* x, CELL a);

CELL sloth_to_abs(X* x, CELL a);
CELL sloth_to_rel(X* x, CELL a);

/* -- Inner interpreter -------------------------------- */

CELL op(X* x);
void do_prim(X* x, CELL p);
void call(X* x, CELL q);
void execute(X* x, CELL q);
void inner(X* x);
void eval(X* x, CELL q);

/* -- Exceptions --------------------------------------- */

void catch(X* x, CELL q);
void throw(X* x, CELL e);

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Constants --------------------------------------- */

/* Displacement of counted string buffer from here */
#define CBUF					64	/* Counted string buffer */

/* Relative addresses of variables accessed both from C */
/* and Forth. */

#define HERE									0	
#define BASE									sCELL
#define FORTH_WORDLIST				2*sCELL	/* Not used in C */
#define STATE									3*sCELL
#define IBUF									4*sCELL
#define IPOS									5*sCELL
#define ILEN									6*sCELL
#define SOURCE_ID							7*sCELL
#define HLD										8*sCELL /* Not used in C */
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

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v);
CELL get(X* x, CELL a);

void cset(X* x, CELL a, uCHAR v);
uCHAR cget(X* x, CELL a);

/* Memory management */

CELL here(X* x);
void allot(X* x, CELL v);
CELL aligned(CELL a);
void align(X* x);

/* Compilation */

void comma(X* x, CELL v);
void ccomma(X* x, uCHAR v);

void compile(X* x, CELL xt);
void literal(X* x, CELL n);

/* Headers */

CELL get_latest(X* x);
void set_latest(X* x, CELL w);

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

CELL header(X* x, CELL n, CELL l);
CELL get_link(X* x, CELL w);
CELL get_xt(X* x, CELL w);
void set_xt(X* x, CELL w, CELL xt);
uCHAR get_flags(X* x, CELL w);
CELL has_flag(X* x, CELL w, CELL v);
uCHAR get_namelen(X* x, CELL w);
CELL get_name_addr(X* x, CELL w);

/* Setting flags */

void set_flag(X* x, CELL w, uCHAR v);
void unset_flag(X* x, CELL w, uCHAR v);

/* -- Primitives -------------------------------------- */

void p_exit(X* x);
void _lit(X* x);
void _rip(X* x);

void _compile(X* x);

void _branch(X* x);
void _zbranch(X* x);

void _string(X* x);
void _c_string(X* x);

/* Quotations (not in ANS Forth yet) */

void _quotation(X* x);
void _start_quotation(X* x);
void _end_quotation(X* x);

/* Loop helpers */

void ipush(X* x);
void ipop(X* x);
void _unloop(X* x);
void _doloop(X* x);

/* Parsing input */

void _word(X* x);

/* Finding words */

int compare_without_case(X* x, CELL w, CELL t, CELL l);
CELL search_word(X* x, CELL n, int l);
void _find(X* x);
CELL find_word(X* x, char* name);

/* Outer interpreter */

void _interpret(X* x);

/* -- Required words to bootstrap ---------------------- */

/* Commands that can help you start or end work sessions */

void _bye(X* x);

/* Commands to inspect memory, debug & view code */

void _depth(X* x);
void _unused(X* x);

/* Source code preprocessing, interpreting & auditing commands */

void _included(X* x);

/* String operations */

void _move(X* x);

/* More input/output operations */

void _emit(X* x);
void _key(X* x);

/* Arithmetic and logical operations */

void _and(X* x);
void _invert(X* x);
void _l_shift(X* x);
void _m_star(X* x);
void _minus(X* x);
void _s_m_slash_rem(X* x);
void _plus(X* x);
void _d_plus(X* x);
void _r_shift(X* x);
void _star(X* x);
void _two_slash(X* x);
void _u_m_star(X* x);
void _u_m_slash_mod(X* x);

/* Memory-stack transfer operations */

void _c_fetch(X* x);
void _c_store(X* x);
void _fetch(X* x);
void _store(X* x);

/* Comparison operations */

void _equals(X* x);
void _less_than(X* x);

/* More facilities for defining routines (compiling-mode only) */

void _colon(X* x);
void _colon_no_name(X* x);
void _semicolon(X* x);
void _recurse(X* x);
void _catch(X* x);
void _throw(X* x);

/* Manipulating stack items */

void _drop(X* x);
void _over(X* x);
void _pick(X* x);
void _to_r(X* x);
void _r_from(X* x);
void _swap(X* x);

/* Constructing compiler and interpreter system extensions */

void _allot(X* x);
void _cells(X* x);
void _chars(X* x);
void _compile_comma(X* x);
void _create(X* x);
void _do_does(X* x);
void _does(X* x);
void _evaluate(X* x);
void _execute(X* x);
void _here(X* x);
void _immediate(X* x);
void _to_in(X* x);
void _postpone(X* x);
void _refill(X* x);
void _source(X* x);

/* -- Helpers to add primitives to the dictionary ------ */

CELL primitive(X* x, F f);
CELL code(X* x, char* name, CELL xt);

/* Helper to work with absolute/relative memory addresses */

void _to_abs(X* x);
void _to_rel(X* x);

/* Helper to empty the return stack */

void _empty_rs(X* x);

/* -- Bootstrapping ------------------------------------ */

void bootstrap(X* x);

/* Helpers to work with files from C */

void include(X* x, char* f);

/* Helper REPL */

void repl(X* x);

#ifdef SLOTH_IMPLEMENTATION

/* -- getch multiplatform implementation --------------- */

#if !defined(WIN32) && !defined(_WIN32) && !defined(_WIN64)
int getch() {
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}
#endif

/* -- Context initialization/destruction --------------- */

void sloth_init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;

	x->jmpbuf_idx = -1;
}

/* TODO Allot the ability to not use malloc at all in */
/* this file. */
X* sloth_create(int psize, int dsize) {
	X* x;

	x = malloc(sizeof(X));
	x->p = malloc(sizeof(sloth_P));
	x->p->p = malloc(sizeof(F) * psize);
	x->p->last = 0;
	x->p->sz = psize;
	x->d = (CELL)malloc(dsize);

	sloth_init(x, x->d, dsize);

	return x;
}

X* sloth_new() { return sloth_create(512, 262144); }

void sloth_free(X* x) {
	free((void*)x->d);
	free(x->p);
	free(x);
}

/* -- Data stack --------------------------------------- */

void sloth_push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL sloth_pop(X* x) { x->sp--; return x->s[x->sp]; }
CELL sloth_pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

/* -- Return stack ------------------------------------- */

void sloth_rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL sloth_rpop(X* x) { x->rp--; return x->r[x->rp]; }
CELL sloth_rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

/* -- Memory ------------------------------------------- */

/* 
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void sloth_store(X* x, CELL a, CELL v) { *((CELL*)a) = v; }
CELL sloth_fetch(X* x, CELL a) { return *((CELL*)a); }
void sloth_cstore(X* x, CELL a, uCHAR v) { *((uCHAR*)a) = v; }
uCHAR sloth_cfetch(X* x, CELL a) { return *((uCHAR*)a); }

/*
The next two macros allow transforming from relative to
absolute addresses.
*/
CELL sloth_to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL sloth_to_rel(X* x, CELL a) { return a - x->d; }

/* -- Inner interpreter -------------------------------- */

CELL op(X* x) { 
	CELL o = sloth_fetch(x, sloth_to_abs(x, x->ip));
	x->ip += sCELL;
	return o; 
}

void do_prim(X* x, CELL p) { 
	(x->p->p[-1 - p])(x); 
}

void call(X* x, CELL q) { 
	if (x->ip >= 0) sloth_rpush(x, x->ip); 
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

/* -- Exceptions --------------------------------------- */

void catch(X* x, CELL q) {
	int tsp = x->sp;
	int trp = x->rp;
	int tip = x->ip;
	int e;

	if (!(e = setjmp(x->jmpbuf[++x->jmpbuf_idx]))) {
		eval(x, q);
		sloth_push(x, 0);
	} else {
		x->sp = tsp;
		x->rp = trp;
		x->ip = tip;
		sloth_push(x, (CELL)e);
	}

	x->jmpbuf_idx--;
}

void throw(X* x, CELL e) {
	if (x->jmpbuf_idx >= 0) {
		longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
	} else {
		exit(e);
	}
}

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v) { sloth_store(x, sloth_to_abs(x, a), v); }
CELL get(X* x, CELL a) { return sloth_fetch(x, sloth_to_abs(x, a)); }

void cset(X* x, CELL a, uCHAR v) { sloth_cstore(x, sloth_to_abs(x, a), v); }
uCHAR cget(X* x, CELL a) { return sloth_cfetch(x, sloth_to_abs(x, a)); }

/* Memory management */

CELL here(X* x) { return get(x, HERE); }
void allot(X* x, CELL v) { set(x, HERE, get(x, HERE) + v); }
CELL aligned(CELL a) { return (a + (sCELL - 1)) & ~(sCELL - 1); }
void align(X* x) { 
	set(x, HERE, (get(x, HERE) + (sCELL - 1)) & ~(sCELL - 1)); 
}

/* Compilation */

void comma(X* x, CELL v) { set(x, here(x), v); allot(x, sCELL); }
void ccomma(X* x, uCHAR v) { set(x, here(x), v); allot(x, suCHAR); }

void compile(X* x, CELL xt) { comma(x, xt); }

void literal(X* x, CELL n) { 
	comma(x, get_xt(x, find_word(x, "(LIT)")));
	comma(x, n); 
}

/* Headers */

CELL get_latest(X* x) { return sloth_fetch(x, get(x, CURRENT)); }
void set_latest(X* x, CELL w) { sloth_store(x, get(x, CURRENT), w); }

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = here(x); /* NT address */
	comma(x, get_latest(x));
	set_latest(x, w);
	comma(x, 0); /* Reserve space for XT */
	ccomma(x, 0); /* Flags (default flags: 0) */
	ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) ccomma(x, sloth_fetch(x, n + i)); /* Name */
	align(x); /* Align XT address */
	set(x, w + sCELL, here(x));
	return w;
}

CELL get_link(X* x, CELL w) { return get(x, w); }

CELL get_xt(X* x, CELL w) { return get(x, w + sCELL); }
void set_xt(X* x, CELL w, CELL xt) { set(x, w + sCELL, xt); }

uCHAR get_flags(X* x, CELL w) { return cget(x, w + 2*sCELL); }
CELL has_flag(X* x, CELL w, CELL v) { return get_flags(x, w) & v; }

uCHAR get_namelen(X* x, CELL w) { 
	return cget(x, w + 2*sCELL + suCHAR); 
}
CELL get_name_addr(X* x, CELL w) { 
	return sloth_to_abs(x, w + 2*sCELL + 2*suCHAR); 
}

/* Setting flags */

void set_flag(X* x, CELL w, uCHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) | v); 
}
void unset_flag(X* x, CELL w, uCHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) & ~v); 
}

/* -- Primitives -------------------------------------- */

/* _exit does exist, so p_exit is used (from primitive) */

void p_exit(X* x) { x->ip = (x->rp > 0) ? sloth_rpop(x) : -1; }
void _lit(X* x) { sloth_push(x, op(x)); }
void _rip(X* x) { sloth_push(x, sloth_to_abs(x, x->ip) + op(x) - sCELL); }

void _compile(X* x) { compile(x, sloth_pop(x)); }

void _branch(X* x) { x->ip += op(x) - sCELL; }
void _zbranch(X* x) { x->ip += sloth_pop(x) == 0 ? (op(x) - sCELL) : sCELL; }

void _string(X* x) { 
	CELL l = op(x); 
	sloth_push(x, sloth_to_abs(x, x->ip)); 
	sloth_push(x, l); 
	x->ip = aligned(x->ip + l); 
}

void _c_string(X* x) { 
	uCHAR l = sloth_cfetch(x, sloth_to_abs(x, x->ip)); 
	sloth_push(x, sloth_to_abs(x, x->ip)); 
	x->ip = aligned(x->ip + l + 1);
}

/* Quotations (not in ANS Forth yet) */

void _quotation(X* x) { CELL d = op(x); sloth_push(x, x->ip); x->ip += d; }
void _start_quotation(X* x) {
	CELL s = get(x, STATE);
	set(x, STATE, s <= 0 ? s - 1 : s + 1);
	if (get(x, STATE) == -1) sloth_push(x, here(x) + 2*sCELL);
	sloth_push(x, get(x, LATESTXT));
	compile(x, get_xt(x, find_word(x, "(QUOTATION)")));
	sloth_push(x, sloth_to_abs(x, here(x)));
	comma(x, 0);
	set(x, LATESTXT, here(x));
}

void _end_quotation(X* x) {
	CELL s = get(x, STATE), a = sloth_pop(x);
	compile(x, get_xt(x, find_word(x, "EXIT")));
	sloth_store(x, a, sloth_to_abs(x, here(x)) - a - sCELL);
	set(x, LATESTXT, sloth_pop(x));
	set(x, STATE, s < 0 ? s + 1 : s - 1);
}

/* Loop helpers */

void ipush(X* x) { 
	sloth_rpush(x, get(x, KX));
	set(x, KX, get(x, JX));
	set(x, JX, get(x, IX));
	set(x, LX, 0);
}
void ipop(X* x) { 
	set(x, LX, 0);
	set(x, IX, get(x, JX));
	set(x, JX, get(x, KX));
	set(x, KX, sloth_rpop(x));
}

void _unloop(X* x) { 
	set(x, LX, get(x, LX) - 1);
	if (get(x, LX) == -1) {
		set(x, IX, get(x, JX));
		set(x, JX, get(x, KX));
		set(x, KX, sloth_rpick(x, 1));
	} else if (get(x, LX) == -2) {
		set(x, IX, get(x, JX));
		set(x, JX, get(x, KX));
		set(x, KX, sloth_rpick(x, 3));
	}
}

/* Algorithm for doloop taken from pForth */
/* (pf_inner.c case ID_PLUS_LOOP) */
void _doloop(X* x) {
	CELL q, do_first_loop, l, o, d;
	ipush(x);
	q = sloth_pop(x);
	do_first_loop = sloth_pop(x);
	set(x, IX, sloth_pop(x));
	l = sloth_pop(x);

	o = get(x, IX) - l;
	d = 0;

	/* First iteration is executed always on a DO */
	if (do_first_loop == 1) {
		eval(x, q);
		if (get(x, LX) == 0) {
			d = sloth_pop(x);
			o = get(x, IX) - l;
			set(x, IX, get(x, IX) + d);
			/* printf("LX == 0 l %ld o %ld d %ld\n", l, o, d); */
		}
	}

	if (!(do_first_loop == 0 && o == 0)) {
		while (((o ^ (o + d)) & (o ^ d)) >= 0 && get(x, LX) == 0) {
			eval(x, q);
			if (get(x, LX) == 0) { /* Avoid pop if we're leaving */
				d = sloth_pop(x);
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
		sloth_rpop(x);
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
	uCHAR c = (uCHAR)sloth_pop(x);
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	CELL start, end, i;
	/* First, ignore c until not c is found */
	/* The Forth Standard says that if the control character is */
	/* the space (hex 20) then control characters may be treated */
	/* as delimiters. */
	if (c == 32) {
		while (ipos < ilen && sloth_cfetch(x, ibuf + ipos) <= c) ipos++;
	} else {
		while (ipos < ilen && sloth_cfetch(x, ibuf + ipos) == c) ipos++;
	}
	start = ibuf + ipos;
	/* Next, continue parsing until c is found again */
	if (c == 32) {
		while (ipos < ilen && sloth_cfetch(x, ibuf + ipos) > c) ipos++;
	} else {
		while (ipos < ilen && sloth_cfetch(x, ibuf + ipos) != c) ipos++;
	}
	end = ibuf + ipos;	
	/* Now, copy it to the counted string buffer */
	sloth_cstore(x, sloth_to_abs(x, here(x) + CBUF), end - start);
	for (i = 0; i < (end - start); i++) {
		sloth_cstore(x, sloth_to_abs(x, here(x) + CBUF + suCHAR + i*suCHAR), sloth_cfetch(x, start + i*suCHAR));
	}
	sloth_push(x, sloth_to_abs(x, here(x) + CBUF));
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
		uCHAR a = sloth_cfetch(x, t + i);
		uCHAR b = sloth_cfetch(x, get_name_addr(x, w) + i);
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
		w = sloth_fetch(x, wl);
		while (w != 0) {
			if (!has_flag(x, w, HIDDEN) && compare_without_case(x, w, n, l)) return w;
			w = get_link(x, w);
		}
	}
	return 0;
}

void _find(X* x) {
	CELL cstring = sloth_pop(x);
	CELL w = search_word(x, cstring + suCHAR, sloth_cfetch(x, cstring));
	if (w == 0) {
		sloth_push(x, cstring);
		sloth_push(x, 0);
	} else if (has_flag(x, w, IMMEDIATE)) {
		sloth_push(x, get_xt(x, w));
		sloth_push(x, 1);
	} else {
		sloth_push(x, get_xt(x, w));
		sloth_push(x, -1);
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
		sloth_push(x, 32); _word(x);
		tok = (char*)(sloth_pick(x, 0) + suCHAR);
		tlen = sloth_cfetch(x, sloth_pick(x, 0));
		if (tlen == 0) { sloth_pop(x); return; }
		_find(x);
		if ((flag = sloth_pop(x)) != 0) {
			if (get(x, STATE) == 0
			|| (get(x, STATE) != 0 && flag == 1)) {
				eval(x, sloth_pop(x));	
			} else {
				compile(x, sloth_pop(x));
			}
		} else {
			CELL temp_base = get(x, BASE);
			sloth_pop(x);
			if (tlen == 3 && *tok == '\'' && (*(tok + 2)) == '\'') {
				/* Character literal */
				if (get(x, STATE) == 0)	sloth_push(x, *(tok + 1));
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
					if (get(x, STATE) == 0) sloth_push(x, n);
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

/* -- Required words to bootstrap ---------------------- */

/* Commands that can help you start or end work sessions */

void _bye(X* x) { printf("\n"); exit(0); }

/* Commands to inspect memory, debug & view code */

void _depth(X* x) { sloth_push(x, x->sp); }
void _unused(X* x) { sloth_push(x, x->sz - get(x, HERE)); }

/* Source code preprocessing, interpreting & auditing commands */

void _included(X* x) {
	FILE *f;
	char filename[1024];
	char linebuf[1024];
	CELL INTERPRET, e;

	CELL previbuf = get(x, IBUF);
	CELL previpos = get(x, IPOS);
	CELL previlen = get(x, ILEN);

	CELL prevsourceid = get(x, SOURCE_ID);

	CELL l = sloth_pop(x);
	CELL a = sloth_pop(x);

	strncpy(filename, (char*)a, (size_t)l);
	filename[l] = 0;

	f = fopen(filename, "r");

	if (f) {
		INTERPRET = get_xt(x, find_word(x, "INTERPRET"));

		set(x, SOURCE_ID, (CELL)f);

		while (fgets(linebuf, 1024, f)) {
			/* printf(">>>> %s\n", linebuf); */
			/* I tried to use _refill from here as the next */
			/* lines of code do exactly the same but, the */
			/* input buffer of the included file is overwritten */
			/* when doing some REFILL from Forth (for an [IF] */
			/* for example). So I left this here to be able to */
			/* use linebuf here. */
			set(x, IBUF, (CELL)linebuf);
			set(x, IPOS, 0);
			if (linebuf[strlen(linebuf) - 1] < ' ') {
				set(x, ILEN, strlen(linebuf) - 1);
			} else {
				set(x, ILEN, strlen(linebuf));
			}

			eval(x, INTERPRET);
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

/* String operations */

void _move(X* x) {
	CELL u = sloth_pop(x);
	CELL addr2 = sloth_pop(x);
	CELL addr1 = sloth_pop(x);
	CELL i;
	if (addr1 >= addr2) {
		for (i = 0; i < u; i++) {
			sloth_cstore(x, addr2 + i, sloth_cfetch(x, addr1 + i));
		}
	} else {
		for (i = u - 1; i >= 0; i--) {
			sloth_cstore(x, addr2 + i, sloth_cfetch(x, addr1 + i));
		}
	}
}

/* More input/output operations */

#ifndef SLOTH_CUSTOM_EMIT
void _emit(X* x) { printf("%c", (uCHAR)sloth_pop(x)); }
#endif
#ifndef SLOTH_CUSTOM_KEY
void _key(X* x) { sloth_push(x, getch()); }
#endif

/* Arithmetic and logical operations */

void _and(X* x) { CELL v = sloth_pop(x); sloth_push(x, sloth_pop(x) & v); }

void _invert(X* x) { sloth_push(x, ~sloth_pop(x)); }
void _l_shift(X* x) { CELL n = sloth_pop(x); sloth_push(x, sloth_pop(x) << n); }

/* Code for _m_star has been created by claude.ai */
void _m_star(X* x) {
	CELL b = sloth_pop(x), a = sloth_pop(x), high, low;

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

	sloth_push(x, low);
	sloth_push(x, high);
}

void _minus(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) - a); }

void _s_m_slash_rem(X* x) {
	CELL n1 = sloth_pop(x), d1_hi = sloth_pop(x), d1_lo = sloth_pop(x);

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
		sloth_push(x, remainder);
		sloth_push(x, quotient);
	}
}

void _plus(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) + a); }

/* Code adapted from pForth */
void _d_plus(X* x) { 
	uCELL ah, al, bl, bh, sh, sl;
	bh = (uCELL)sloth_pop(x);
	bl = (uCELL)sloth_pop(x);
	ah = (uCELL)sloth_pop(x);
	al = (uCELL)sloth_pop(x);
	sh = 0;
	sl = al + bl;
	if (sl < bl) sh = 1;	/* carry */
	sh += ah + bh;
	sloth_push(x, sl);
	sloth_push(x, sh);
}

void _r_shift(X* x) { 
	CELL n = sloth_pop(x); 
	sloth_push(x, ((uCELL)sloth_pop(x)) >> n); 
}

void _star(X* x) { CELL b = sloth_pop(x); sloth_push(x, sloth_pop(x) * b); }

void _two_slash(X* x) { sloth_push(x, sloth_pop(x) >> 1); }
void _u_m_star(X* x) {
	uCELL b = (uCELL)sloth_pop(x), a = (uCELL)sloth_pop(x), high, low;

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

	sloth_push(x, low);
	sloth_push(x, high);
}
/* UM/MOD code taken from pForth */
#define DULT(du1l,du1h,du2l,du2h) ( (du2h<du1h) ? 0 : ( (du2h==du1h) ? (du1l<du2l) : 1) )
void _u_m_slash_mod(X* x) {
	uCELL ah, al, q, di, bl, bh, sl, sh;
	bh = (uCELL)sloth_pop(x);
	bl = 0;
	ah = (uCELL)sloth_pop(x);
	al = (uCELL)sloth_pop(x);
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
	sloth_push(x, al); /* rem */
	sloth_push(x, q);
}

/* Memory-stack transfer operations */

void _c_fetch(X* x) { sloth_push(x, sloth_cfetch(x, sloth_pop(x))); }
void _c_store(X* x) { CELL a = sloth_pop(x); sloth_cstore(x, a, sloth_pop(x)); }
void _fetch(X* x) { sloth_push(x, sloth_fetch(x, sloth_pop(x))); }
void _store(X* x) { CELL a = sloth_pop(x); sloth_store(x, a, sloth_pop(x)); }

/* Comparison operations */

void _equals(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) == a ? -1 : 0); }
void _less_than(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) < a ? -1 : 0); }

/* More facilities for defining routines (compiling-mode only) */

void _colon(X* x) {
	CELL tok, tlen;
	sloth_push(x, 32); _word(x);
	tok = sloth_pick(x, 0) + suCHAR;
	tlen = sloth_cfetch(x, sloth_pop(x));
	header(x, tok, tlen);
	set(x, LATESTXT, get_xt(x, get_latest(x)));
	set_flag(x, get_latest(x), HIDDEN);
	set(x, STATE, 1);
}
void _colon_no_name(X* x) { 
	sloth_push(x, here(x));
	set(x, LATESTXT, here(x));
	set(x, STATE, 1);
}
void _semicolon(X* x) {
	compile(x, get_xt(x, find_word(x, "EXIT")));
	set(x, STATE, 0);
	/* Don't change flags for nonames */
	if (get_xt(x, get_latest(x)) == get(x, LATESTXT))
		unset_flag(x, get_latest(x), HIDDEN);
}

void _recurse(X* x) { compile(x, get(x, LATESTXT)); }
void _catch(X* x) { catch(x, sloth_pop(x)); }
void _throw(X* x) { 
	CELL e = sloth_pop(x); 
	if (e != 0) {
		if (e == -2) {
			int l = (int)sloth_pop(x);
			char *a = (char*)sloth_pop(x);
			if (x->jmpbuf_idx < 0) {
				printf("Error: %.*s\n", l, a);
			}
		}
		throw(x, e);
	}
}

/* Manipulating stack items */

void _drop(X* x) { if (x->sp <= 0) throw(x, -4); else sloth_pop(x); }
void _over(X* x) { sloth_push(x, sloth_pick(x, 1)); }
void _pick(X* x) {  sloth_push(x, sloth_pick(x, sloth_pop(x))); }
void _to_r(X* x) { sloth_rpush(x, sloth_pop(x)); }
void _r_from(X* x) { sloth_push(x, sloth_rpop(x)); }
void _swap(X* x) { CELL a = sloth_pop(x); CELL b = sloth_pop(x);sloth_push(x, a); sloth_push(x, b); }

/* Constructing compiler and interpreter system extensions */

void _allot(X* x) { allot(x, sloth_pop(x)); }
void _cells(X* x) { sloth_push(x, sloth_pop(x) * sCELL); }
void _chars(X* x) { /* Does nothing */ }
void _compile_comma(X* x) { compile(x, sloth_pop(x)); }
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
	sloth_push(x, 32); _word(x);
	tok = sloth_pick(x, 0) + suCHAR;
	tlen = sloth_cfetch(x, sloth_pop(x));
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
	set(x, get_xt(x, get_latest(x)) + 2*sCELL, sloth_pop(x));
}
void _does(X* x) {
	literal(x, here(x) + 4*sCELL);
	compile(x, get_xt(x, find_word(x, "(DOES)")));
	compile(x, get_xt(x, find_word(x, "EXIT")));
}
void _evaluate(X* x) {
	CELL l = sloth_pop(x), a = sloth_pop(x);

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
void _execute(X* x) { eval(x, sloth_pop(x)); }
void _here(X* x) { sloth_push(x, sloth_to_abs(x, here(x))); }
void _immediate(X* x) { set_flag(x, get_latest(x), IMMEDIATE); }
void _to_in(X* x) { sloth_push(x, sloth_to_abs(x, IPOS)); }
void _postpone(X* x) { 
	CELL i, xt, tok, tlen;
	sloth_push(x, 32); _word(x);
	tok = sloth_pick(x, 0) + suCHAR;
	tlen = sloth_cfetch(x, sloth_pick(x, 0));
	if (tlen == 0) { sloth_pop(x); return; }
	_find(x); 
	i = sloth_pop(x);
	xt = sloth_pop(x);
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
void _refill(X* x) {
	char linebuf[1024];
	int i;
	switch (get(x, SOURCE_ID)) {
	case -1: 
		sloth_push(x, 0);
		break;
	case 0:
		sloth_push(x, get(x, IBUF)); sloth_push(x, 80);
		eval(x, get_xt(x, find_word(x, "ACCEPT")));
		set(x, ILEN, sloth_pop(x));
		set(x, IPOS, 0);
		sloth_push(x, -1); 
		break;
	default: 
		if (fgets(linebuf, 1024, (FILE *)get(x, SOURCE_ID))) {
			set(x, IBUF, (CELL)linebuf);
			set(x, IPOS, 0);
			/* Although I haven't found anywhere that \n should */
			/* not be part of the input buffer when reading from */
			/* a file, the results from preliminary tests when */
			/* using SOURCE ... TYPE add newlines (because they */
			/* are present) and on some other Forths they do not. */
			/* So I just added a check to remove the \n at then */
			/* end. */
			if (linebuf[strlen(linebuf) - 1] < ' ') {
				set(x, ILEN, strlen(linebuf) - 1);
			} else {
				set(x, ILEN, strlen(linebuf));
			}
			sloth_push(x, -1);
		} else {
			sloth_push(x, 0);
		}
		break;
	}	
}

void _source(X* x) { sloth_push(x, get(x, IBUF)); sloth_push(x, get(x, ILEN)); }

/* -- Helpers to add primitives to the dictionary ------ */

CELL primitive(X* x, F f) { 
	assert(x->p->last < x->p->sz);
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL code(X* x, char* name, CELL xt) {
	CELL w = header(x, (CELL)name, strlen(name));
	set_xt(x, w, xt);
	return xt; 
}

/* Helper to work with absolute/relative memory addresses */

void _to_abs(X* x) { sloth_push(x, sloth_pop(x) + x->d); }
void _to_rel(X* x) { sloth_push(x, sloth_pop(x) - x->d); }

/* Helper to empty the return stack */

void _empty_rs(X* x) { x->rp = 0; }

/* -- Bootstrapping ------------------------------------ */

void bootstrap(X* x) {

	/* Variables commonly shared from C and Forth */

	/* comma can not be used for first variable because it */
	/* needs here(x) and that function gets the value from */
	/* the first variable. */

	*((CELL*)x->d) = sCELL; /* HERE */

	comma(x, 10); /* BASE */
	comma(x, 0); /* FORTH-WORDLIST */
	comma(x, 0); /* STATE */
	comma(x, 0); /* IBUF */
	comma(x, 0); /* IPOS */
	comma(x, 0); /* ILEN */
	comma(x, 0); /* SOURCE_ID */
	comma(x, 0); /* HLD */
	comma(x, 0); /* LATESTXT */
	comma(x, 0); /* IX */
	comma(x, 0); /* JX */
	comma(x, 0); /* KX */
	comma(x, 0); /* LX */
	comma(x, sloth_to_abs(x, FORTH_WORDLIST)); /* CURRENT */
	comma(x, 1); /* #ORDER */
	comma(x, sloth_to_abs(x, FORTH_WORDLIST)); /* CONTEXT 0 */
	allot(x, 15*sCELL);
	/* Variable indicating if we are on Linux or Windows */
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	comma(x, 1);
#else
	comma(x, 0);
#endif

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

	code(x, "UNUSED", primitive(x, &_unused));
	code(x, "BYE", primitive(x, &_bye));

	/* Commands to inspect memory, debug & view code */

	code(x, "DEPTH", primitive(x, &_depth));

	/* Source code preprocessing, interpreting & auditing commands */

	code(x, "INCLUDED", primitive(x, &_included));

	/* String operations */

	code(x, "MOVE", primitive(x, &_move));
	
	/* More input/output operations */

	code(x, "EMIT", primitive(x, &_emit));
	code(x, "KEY", primitive(x, &_key));

	/* Arithmetic and logical operations */

	code(x, "AND", primitive(x, &_and));
	code(x, "INVERT", primitive(x, &_invert));
	code(x, "LSHIFT", primitive(x, &_l_shift));
	code(x, "M*", primitive(x, &_m_star));
	code(x, "-", primitive(x, &_minus));
	code(x, "SM/REM", primitive(x, &_s_m_slash_rem));
	code(x, "+", primitive(x, &_plus));
	code(x, "D+", primitive(x, &_d_plus));
	code(x, "RSHIFT", primitive(x, &_r_shift));
	code(x, "*", primitive(x, &_star));
	code(x, "2/", primitive(x, &_two_slash));
	code(x, "UM*", primitive(x, &_u_m_star));
	code(x, "UM/MOD", primitive(x, &_u_m_slash_mod));

	/* Memory-stack transfer operations */

	code(x, "C@", primitive(x, &_c_fetch));
	code(x, "C!", primitive(x, &_c_store));
	code(x, "@", primitive(x, &_fetch));
	code(x, "!", primitive(x, &_store));

	/* Comparison operations */

	code(x, "=", primitive(x, &_equals));
	code(x, "<", primitive(x, &_less_than));

	/* Forming definite loops */

	code(x, "UNLOOP", primitive(x, &_unloop));

	/* More facilities for defining routines (compiling-mode only) */

	code(x, ":", primitive(x, &_colon));
	code(x, ":NONAME", primitive(x, &_colon_no_name));
	code(x, ";", primitive(x, &_semicolon)); _immediate(x);

	code(x, "CATCH", primitive(x, &_catch));
	code(x, "THROW", primitive(x, &_throw));

	/* Manipulating stack items */

	code(x, "DROP", primitive(x, &_drop));
	code(x, "OVER", primitive(x, &_over));
	code(x, "PICK", primitive(x, &_pick));
	code(x, ">R", primitive(x, &_to_r));
	code(x, "R>", primitive(x, &_r_from));
	code(x, "SWAP", primitive(x, &_swap));

	code(x, "RECURSE", primitive(x, &_recurse)); _immediate(x);

	/* Constructing compiler and interpreter system extensions */

	code(x, "ALLOT", primitive(x, &_allot));
	code(x, "CELLS", primitive(x, &_cells));
	code(x, "CHARS", primitive(x, &_chars));
	code(x, "COMPILE,", primitive(x, &_compile_comma));
	code(x, "CREATE", primitive(x, &_create));
	code(x, "DOES>", primitive(x, &_does)); _immediate(x);
	code(x, "EVALUATE", primitive(x, &_evaluate));
	code(x, "EXECUTE", primitive(x, &_execute));
	code(x, "HERE", primitive(x, &_here));
	code(x, "IMMEDIATE", primitive(x, &_immediate));
	code(x, ">IN", primitive(x, &_to_in));
	code(x, "POSTPONE", primitive(x, &_postpone)); _immediate(x);
	code(x, "REFILL", primitive(x, &_refill));
	code(x, "SOURCE", primitive(x, &_source));
	code(x, "WORD", primitive(x, &_word));

	code(x, "FIND", primitive(x, &_find));

	/* Helpers */

	code(x, "TO-ABS", primitive(x, &_to_abs));
	code(x, "TO-REL", primitive(x, &_to_rel));
	/* TODO Maybe INTERPRET should be deferred or as a */
	/* variable to be able to create an INTERPRET for */
	/* debugging in Forth itself. */
	code(x, "INTERPRET", primitive(x, &_interpret));
	code(x, "(EMPTY-RETURN-STACK)", primitive(x, &_empty_rs));
}

/* Helpers to work with files from C */

void include(X* x, char* f) {
	sloth_push(x, (CELL)f);
	sloth_push(x, strlen(f));
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

#endif
#endif
