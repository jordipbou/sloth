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
#define WINDOWS
#endif

#if defined(WINDOWS)
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

#define sCELL sizeof(CELL)
#define suCHAR sizeof(uCHAR)
#define CELL_BITS sCELL*8

#define ALIGNED(a, t) (((a) + ((t) - 1)) & ~((t) - 1))

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

#ifndef SLOTH_STACK_SIZE
#define SLOTH_STACK_SIZE 64
#endif

#ifndef SLOTH_RETURN_STACK_SIZE
#define SLOTH_RETURN_STACK_SIZE 64
#endif

#ifndef SLOTH_FLOAT_STACK_SIZE
#define SLOTH_FLOAT_STACK_SIZE 64
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

	#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

		FCELL f[SLOTH_FLOAT_STACK_SIZE]; 
		CELL fp;

	#endif

	CELL ip;
	CELL d, sz;	/* Dict base address, dict size */
	CELL u, uz; /* User area base address and size */

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	sloth_P *p;
} X;

/* -- Context initialization/destruction --------------- */

void sloth_init(X* x, CELL d, CELL sz, CELL u, CELL uz);
X* sloth_create(int psize, int dsize, int usize);
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

CELL sloth_op(X* x);
void sloth__do_prim(X* x, CELL p);
void sloth__call(X* x, CELL q);
void sloth__execute(X* x, CELL q);
void sloth__inner(X* x);
void sloth_eval(X* x, CELL q);

/* -- Exceptions --------------------------------------- */

void sloth_catch(X* x, CELL q);
void sloth_throw(X* x, CELL e);

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Constants --------------------------------------- */

/* Displacement of counted string buffer from here */
#define SLOTH_CBUF				64	/* Counted string buffer */

/* Relative addresses of variables accessed both from C */
/* and Forth. */

#define SLOTH_HERE							0	
#define SLOTH_INTERNAL_WL				1*sCELL
#define SLOTH_FORTH_WL					2*sCELL

/* User area variables */

#define SLOTH_CURRENT						0*sCELL
#define SLOTH_ORDER							1*sCELL
#define SLOTH_CONTEXT						2*sCELL
/* There are 16 CELLS reserved to search order */
#define SLOTH_BASE							18*sCELL
#define SLOTH_STATE							19*sCELL
#define SLOTH_IBUF							20*sCELL
#define SLOTH_IPOS							21*sCELL
#define SLOTH_ILEN							22*sCELL
#define SLOTH_SOURCE_ID					23*sCELL
#define SLOTH_LATESTXT					24*sCELL
#define SLOTH_IX								25*sCELL
#define SLOTH_JX								26*sCELL
#define SLOTH_KX								27*sCELL
#define SLOTH_LX								28*sCELL
#define SLOTH_INTERPRET					29*sCELL

#define SLOTH_PATH_START				30*sCELL
#define SLOTH_PATH_END					31*sCELL
#define SLOTH_PATH							32*sCELL

#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

#define SLOTH_PRECISION					97*sCELL

#endif


/* Word statuses */

#define SLOTH_HIDDEN					1
#define SLOTH_IMMEDIATE				2

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void sloth_set(X* x, CELL a, CELL v);
CELL sloth_get(X* x, CELL a);

void sloth_cset(X* x, CELL a, uCHAR v);
uCHAR sloth_cget(X* x, CELL a);

void sloth_user_area_set(X* x, CELL a, CELL v);
CELL sloth_user_area_get(X* x, CELL a);

/* Memory management */

CELL sloth_here(X* x);
void sloth_allot(X* x, CELL v);
CELL sloth_aligned(CELL a);
void sloth_align(X* x);

/* Compilation */

void sloth_comma(X* x, CELL v);
void sloth_ccomma(X* x, uCHAR v);

void sloth_compile(X* x, CELL xt);
void sloth_literal(X* x, CELL n);

/* Headers */

CELL sloth_get_latest(X* x);
void sloth_set_latest(X* x, CELL w);

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

CELL sloth_header(X* x, CELL n, CELL l);
CELL sloth_get_link(X* x, CELL w);
CELL sloth_get_xt(X* x, CELL w);
void sloth_set_xt(X* x, CELL w, CELL xt);
uCHAR sloth_get_flags(X* x, CELL w);
CELL sloth_has_flag(X* x, CELL w, CELL v);
uCHAR sloth_get_namelen(X* x, CELL w);
CELL sloth_get_name_addr(X* x, CELL w);

/* Setting flags */

void sloth_set_flag(X* x, CELL w, uCHAR v);
void sloth_unset_flag(X* x, CELL w, uCHAR v);

/* -- Primitives -------------------------------------- */

void sloth_exit_(X* x);
void sloth_lit_(X* x);
void sloth_rip_(X* x);

void sloth_compile_(X* x);

void sloth_branch_(X* x);
void sloth_zbranch_(X* x);

void sloth_string_(X* x);
void sloth_c_string_(X* x);

/* Quotations (not in ANS Forth yet) */

void sloth_quotation_(X* x);
void sloth_start_quotation_(X* x);
void sloth_end_quotation_(X* x);

/* Loop helpers */

void sloth__ipush(X* x);
void sloth__ipop(X* x);
void sloth_unloop_(X* x);
void sloth_doloop_(X* x);

/* Environment queries */

void sloth_environment_(X* x);

/* Parsing input */

void sloth_word_(X* x);

/* Finding words */

int sloth__compare_without_case(X* x, CELL w, CELL t, CELL l);
CELL sloth__search_word(X* x, CELL n, int l);
void sloth_find_(X* x);
CELL sloth_find_word(X* x, char* name);

/* Outer interpreter */

void sloth_interpret_(X* x);

/* -- Required words to bootstrap ---------------------- */

/* Commands that can help you start or end work sessions */

void sloth_bye_(X* x);

/* Commands to inspect memory, debug & view code */

void sloth_depth_(X* x);
void sloth_r_depth_(X* x);
void sloth_unused_(X* x);

/* Source code preprocessing, interpreting & auditing commands */

void sloth_included_(X* x);

/* String operations */

void sloth_move_(X* x);

/* More input/output operations */

void sloth_emit_(X* x);
void sloth_key_(X* x);

/* Arithmetic and logical operations */

void sloth_and_(X* x);
void sloth_invert_(X* x);
void sloth_l_shift_(X* x);
void sloth_m_star_(X* x);
void sloth_minus_(X* x);
void sloth_plus_(X* x);
void sloth_d_plus_(X* x);
void sloth_r_shift_(X* x);
void sloth_star_(X* x);
void sloth_two_slash_(X* x);
void sloth_u_m_star_(X* x);
void sloth_u_m_slash_mod_(X* x);

/* Memory-stack transfer operations */

void sloth_c_fetch_(X* x);
void sloth_c_store_(X* x);
void sloth_fetch_(X* x);
void sloth_store_(X* x);

/* Comparison operations */

void sloth_equals_(X* x);
void sloth_less_than_(X* x);

/* More facilities for defining routines (compiling-mode only) */

void sloth_colon_(X* x);
void sloth_colon_no_name_(X* x);
void sloth_semicolon_(X* x);
void sloth_recurse_(X* x);
void sloth_catch_(X* x);
void sloth_throw_(X* x);

/* Manipulating stack items */

void sloth_drop_(X* x);
void sloth_over_(X* x);
void sloth_pick_(X* x);
void sloth_r_pick_(X* x);
void sloth_to_r_(X* x);
void sloth_r_from_(X* x);
void sloth_swap_(X* x);

/* Constructing compiler and interpreter system extensions */

void sloth_allot_(X* x);
void sloth_cells_(X* x);
void sloth_chars_(X* x);
void sloth_compile_comma_(X* x);
void sloth_create_(X* x);
void sloth_do_does_(X* x);
void sloth_does_(X* x);
void sloth_evaluate_(X* x);
void sloth_execute_(X* x);
void sloth_here_(X* x);
void sloth_immediate_(X* x);
/* void sloth_to_in_(X* x); */
void sloth_postpone_(X* x);
void sloth_refill_(X* x);
void sloth_source_(X* x);

/* == Helpers and bootstrapping ======================== */

/* -- Helpers to add primitives to the dictionary ------ */

CELL sloth_primitive(X* x, F f);
CELL sloth_code(X* x, char* name, CELL xt);

/* Helper to work with absolute/relative memory addresses */

void sloth_to_abs_(X* x);
void sloth_to_rel_(X* x);

/* Helper to empty the return stack */

void sloth_empty_rs_(X* x);

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_kernel(X* x);
void sloth_bootstrap(X* x);

/* Helpers to work with files from C */

void sloth_include(X* x, char* f);

/* Helper REPL */

void sloth_repl(X* x);

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

void sloth_init(X* x, CELL d, CELL sz, CELL u, CELL uz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;
	x->u = u;
	x->uz = uz;

	#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

		x->fp = 0;

	#endif

	x->jmpbuf_idx = -1;
}

/* TODO Allot the ability to not use malloc at all in */
/* this file. */
X* sloth_create(int psize, int dsize, int usize) {
	X* x;

	x = malloc(sizeof(X));
	x->p = malloc(sizeof(sloth_P));
	x->p->p = malloc(sizeof(F) * psize);
	x->p->last = 0;
	x->p->sz = psize;
	x->d = (CELL)malloc(dsize);
	x->u = (CELL)malloc(usize);

	sloth_init(x, x->d, dsize, x->u, usize);

	return x;
}

X* sloth_new() { return sloth_create(512, 262144, 1024); }

void sloth_free(X* x) {
	free((void*)x->d);
	free(x->p->p);
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
The next two functions allow transforming from relative to
absolute addresses.
*/
CELL sloth_to_abs(X* x, CELL a) { return (CELL)(x->d + a); }

CELL sloth_to_rel(X* x, CELL a) { return a - x->d; }

/* -- Inner interpreter -------------------------------- */

CELL sloth_op(X* x) {
	CELL o = sloth_fetch(x, x->ip);
	x->ip += sCELL;
	return o;
}

void sloth__do_prim(X* x, CELL p) { 
	(x->p->p[-1 - p])(x); 
}

void sloth__call(X* x, CELL q) { 
	if (x->ip >= 0) sloth_rpush(x, x->ip); 
	x->ip = q; 
}

void sloth__execute(X* x, CELL q) { 
	if (q < 0) sloth__do_prim(x, q); 
	else sloth__call(x, q); 
}

void sloth__inner(X* x) { 
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) { 
		sloth__execute(x, sloth_op(x));
	} 
}

void sloth_eval(X* x, CELL q) { 
	sloth__execute(x, q); 
	if (q > 0) sloth__inner(x); 
}

/* -- Exceptions --------------------------------------- */

void sloth_catch(X* x, CELL q) {
	volatile int tsp = x->sp;
	volatile int trp = x->rp;
	volatile CELL tip = x->ip;
	volatile int e;

	if (!(e = setjmp(x->jmpbuf[++x->jmpbuf_idx]))) {
		sloth_eval(x, q);
		sloth_push(x, 0);
	} else {
		x->sp = tsp;
		x->rp = trp;
		x->ip = tip;
		sloth_push(x, (CELL)e);
	}

	x->jmpbuf_idx--;
}

void sloth_throw(X* x, CELL e) {
	if (x->jmpbuf_idx >= 0) {
		longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
	} else {
		CELL ibuf = *((CELL*)(x->d+5*sCELL));
		CELL ipos = *((CELL*)(x->d+6*sCELL));
		CELL ilen = *((CELL*)(x->d+7*sCELL));
		if (ibuf && ipos <= ilen) {
		    printf("BUFFER: <%.*s>\n", (int)ilen, (char*)ibuf);
		    printf("TOKEN: <%.*s>\n", (int)(ilen - ipos), (char*)(ibuf + ipos));
		}
#if defined(WINDOWS)
		printf("Exception: %Id\n", e);
#else
		printf("Exception: %ld\n", e);
#endif
		exit(e);
	}
}

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void sloth_set(X* x, CELL a, CELL v) { 
	sloth_store(x, sloth_to_abs(x, a), v);
}
CELL sloth_get(X* x, CELL a) { 
	return sloth_fetch(x, sloth_to_abs(x, a)); 
}

void sloth_cset(X* x, CELL a, uCHAR v) { 
	sloth_cstore(x, sloth_to_abs(x, a), v); 
}
uCHAR sloth_cget(X* x, CELL a) { 
	return sloth_cfetch(x, sloth_to_abs(x, a)); 
}

void sloth_user_area_set(X* x, CELL a, CELL v) {
	sloth_store(x, x->u + a, v);
}
CELL sloth_user_area_get(X* x, CELL a) {
	return sloth_fetch(x, x->u + a);
}

/* Memory management */

CELL sloth_here(X* x) { 
	return sloth_get(x, SLOTH_HERE);
}

void sloth_allot(X* x, CELL v) { 
	sloth_set(x, SLOTH_HERE, sloth_here(x) + v); 
}
/* CELL sloth_aligned(CELL a) { return (a + (sCELL - 1)) & ~(sCELL - 1); } */
CELL sloth_aligned(CELL a) { return ALIGNED(a, sCELL); }
void sloth_align(X* x) { 
	sloth_set(
		x, 
		SLOTH_HERE, 
/*		(sloth_get(x, SLOTH_HERE) + (sCELL - 1)) & ~(sCELL - 1));  */
		ALIGNED(sloth_here(x), sCELL));
}

/* Compilation */

void sloth_comma(X* x, CELL v) { 
	sloth_store(x, sloth_here(x), v);
	sloth_allot(x, sCELL); 
}
void sloth_ccomma(X* x, uCHAR v) { 
	sloth_cstore(x, sloth_here(x), v);
	sloth_allot(x, suCHAR); 
}

void sloth_compile(X* x, CELL xt) { sloth_comma(x, xt); }

void sloth_literal(X* x, CELL n) { 
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(LIT)")));
	sloth_comma(x, n); 
}

/* Headers */

CELL sloth_get_latest(X* x) { 
	/*
	return sloth_fetch(x, sloth_get(x, SLOTH_CURRENT)); 
	*/
	return sloth_fetch(x, sloth_user_area_get(x, SLOTH_CURRENT));
}
void sloth_set_latest(X* x, CELL w) { 
	/*
	sloth_store(x, sloth_get(x, SLOTH_CURRENT), w); 
	*/
	sloth_store(x, sloth_user_area_get(x, SLOTH_CURRENT), w);
}

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

CELL sloth_header(X* x, CELL n, CELL l) {
	CELL w, i;
	sloth_align(x);
	w = sloth_here(x); /* NT address */
	sloth_comma(x, sloth_get_latest(x));
	sloth_set_latest(x, w);
	sloth_comma(x, 0); /* Reserve space for XT */
	sloth_ccomma(x, 0); /* Flags (default flags: 0) */
	sloth_ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) sloth_ccomma(x, sloth_fetch(x, n + i)); /* Name */
	sloth_align(x); /* Align XT address */
	sloth_store(x, w + sCELL, sloth_here(x));
	return w;
}

CELL sloth_get_link(X* x, CELL w) {
	return sloth_fetch(x, w);
}

CELL sloth_get_xt(X* x, CELL w) {
	return sloth_fetch(x, w + sCELL);
}

void sloth_set_xt(X* x, CELL w, CELL xt) { 
	sloth_store(x, w + sCELL, xt); 
}

uCHAR sloth_get_flags(X* x, CELL w) {
	return sloth_cfetch(x, w + 2*sCELL);
}

CELL sloth_has_flag(X* x, CELL w, CELL v) { 
	return sloth_get_flags(x, w) & v; 
}

uCHAR sloth_get_namelen(X* x, CELL w) {
	return sloth_cfetch(x, w + 2*sCELL + suCHAR);
}

CELL sloth_get_name_addr(X* x, CELL w) {
	return w + 2*sCELL + 2*suCHAR;
}

/* Setting flags */

void sloth_set_flag(X* x, CELL w, uCHAR v) {
	sloth_cstore(x, w + 2*sCELL, sloth_get_flags(x, w) | v);
}

void sloth_unset_flag(X* x, CELL w, uCHAR v) {
	sloth_cstore(x, w + 2*sCELL, sloth_get_flags(x, w) & ~v);
}

/* -- Primitives -------------------------------------- */

void sloth_exit_(X* x) { 
	x->ip = (x->rp > 0) ? sloth_rpop(x) : -1; 
}
void sloth_lit_(X* x) { sloth_push(x, sloth_op(x)); }
void sloth_rip_(X* x) {
	CELL ip = x->ip;
	CELL o = sloth_op(x);
	sloth_push(x, ip + o - sCELL);
}

void sloth_compile_(X* x) { sloth_compile(x, sloth_pop(x)); }

void sloth_branch_(X* x) { x->ip += sloth_op(x) - sCELL; }
void sloth_zbranch_(X* x) { 
	x->ip += sloth_pop(x) == 0 ? 
		(sloth_op(x) - sCELL) 
		: sCELL; 
}

void sloth_string_(X* x) {
	CELL l = sloth_op(x);
	sloth_push(x, x->ip);
	sloth_push(x, l);
	x->ip = sloth_aligned(x->ip + l + 1);
}

void sloth_c_string_(X* x) {
	uCHAR l = sloth_cfetch(x, x->ip);
	sloth_push(x, x->ip);
	x->ip = sloth_aligned(x->ip + l + 2);
}

/* Quotations (not in ANS Forth yet) */

void sloth_quotation_(X* x) { 
	CELL d = sloth_op(x); 
	sloth_push(x, x->ip); 
	x->ip += d; 
}
void sloth_start_quotation_(X* x) {
	CELL s = sloth_user_area_get(x, SLOTH_STATE);
	sloth_user_area_set(x, SLOTH_STATE, s <= 0 ? s - 1 : s + 1);
	if (sloth_user_area_get(x, SLOTH_STATE) == -1) 
		sloth_push(x, sloth_here(x) + 2*sCELL);
	sloth_push(x, sloth_user_area_get(x, SLOTH_LATESTXT));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(QUOTATION)")));
	sloth_push(x, sloth_here(x));
	sloth_comma(x, 0);
	sloth_user_area_set(x, SLOTH_LATESTXT, sloth_here(x));
}

void sloth_end_quotation_(X* x) {
	CELL s = sloth_user_area_get(x, SLOTH_STATE), a = sloth_pop(x);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_store(x, a, sloth_here(x) - a - sCELL);
	sloth_user_area_set(x, SLOTH_LATESTXT, sloth_pop(x));
	sloth_user_area_set(x, SLOTH_STATE, s < 0 ? s + 1 : s - 1);
}

/* Loop helpers */

void sloth__ipush(X* x) { 
	sloth_rpush(x, sloth_user_area_get(x, SLOTH_KX));
	sloth_user_area_set(x, SLOTH_KX, sloth_user_area_get(x, SLOTH_JX));
	sloth_user_area_set(x, SLOTH_JX, sloth_user_area_get(x, SLOTH_IX));
	sloth_user_area_set(x, SLOTH_LX, 0);
}
void sloth__ipop(X* x) { 
	sloth_user_area_set(x, SLOTH_LX, 0);
	sloth_user_area_set(x, SLOTH_IX, sloth_user_area_get(x, SLOTH_JX));
	sloth_user_area_set(x, SLOTH_JX, sloth_user_area_get(x, SLOTH_KX));
	sloth_user_area_set(x, SLOTH_KX, sloth_rpop(x));
}

void sloth_unloop_(X* x) { 
	sloth_user_area_set(x, SLOTH_LX, sloth_user_area_get(x, SLOTH_LX) - 1);
	if (sloth_user_area_get(x, SLOTH_LX) == -1) {
		sloth_user_area_set(x, SLOTH_IX, sloth_user_area_get(x, SLOTH_JX));
		sloth_user_area_set(x, SLOTH_JX, sloth_user_area_get(x, SLOTH_KX));
		sloth_user_area_set(x, SLOTH_KX, sloth_rpick(x, 1));
	} else if (sloth_user_area_get(x, SLOTH_LX) == -2) {
		sloth_user_area_set(x, SLOTH_IX, sloth_user_area_get(x, SLOTH_JX));
		sloth_user_area_set(x, SLOTH_JX, sloth_user_area_get(x, SLOTH_KX));
		sloth_user_area_set(x, SLOTH_KX, sloth_rpick(x, 3));
	}
}

/* Algorithm for doloop taken from pForth */
/* (pf_inner.c case ID_PLUS_LOOP) */
void sloth_doloop_(X* x) {
	CELL q, do_first_loop, l, o, d;
	sloth__ipush(x);
	q = sloth_pop(x);
	do_first_loop = sloth_pop(x);
	sloth_user_area_set(x, SLOTH_IX, sloth_pop(x));
	l = sloth_pop(x);

	o = sloth_user_area_get(x, SLOTH_IX) - l;
	d = 0;

	/* First iteration is executed always on a DO */
	if (do_first_loop == 1) {
		sloth_eval(x, q);
		if (sloth_user_area_get(x, SLOTH_LX) == 0) {
			d = sloth_pop(x);
			o = sloth_user_area_get(x, SLOTH_IX) - l;
			sloth_user_area_set(x, SLOTH_IX, sloth_user_area_get(x, SLOTH_IX) + d);
			/* printf("LX == 0 l %ld o %ld d %ld\n", l, o, d); */
		}
	}

	if (!(do_first_loop == 0 && o == 0)) {
		while (((o ^ (o + d)) & (o ^ d)) >= 0 && sloth_user_area_get(x, SLOTH_LX) == 0) {
			sloth_eval(x, q);
			if (sloth_user_area_get(x, SLOTH_LX) == 0) { /* Avoid pop if we're leaving */
				d = sloth_pop(x);
				o = sloth_user_area_get(x, SLOTH_IX) - l;
				sloth_user_area_set(x, SLOTH_IX, sloth_user_area_get(x, SLOTH_IX) + d);
			}
		}
	}

	if (sloth_user_area_get(x, SLOTH_LX) == 0 || sloth_user_area_get(x, SLOTH_LX) == 1) { 
		/* Leave case */
		sloth__ipop(x);
	} else if (sloth_user_area_get(x, SLOTH_LX) < 0) {
		/* Unloop case */
		sloth_user_area_set(x, SLOTH_LX, sloth_user_area_get(x, SLOTH_LX) + 1);
		sloth_rpop(x);
		sloth_exit_(x);
	}
}

/* Environment queries */

void sloth_environment_(X* x) {
	switch (sloth_pop(x)) {
	case 0: /* /COUNTED-STRING */
		sloth_push(x, 64); 
		break;
	case 1: /* /HOLD */
		/* TODO */ 
		break;
	case 2: /* /PAD */
		/* TODO */ 
		break;
	case 3: /* ADDRESS-UNIT-BITS */
 		sloth_push(x, CHAR_BIT); 
		break;
	case 4: /* FLOORED */
		/* Good explanation about floored/symmetric division: */
		/* https://www.nimblemachines.com/symmetric-division-considered-harmful/ */
		sloth_push(x, (-3 / 2 == -2) ? -1 : 0);
		break;
	case 5: /* MAX-CHAR */
		sloth_push(x, UCHAR_MAX); 
		break;
	case 6: /* MAX-D */
		/* TODO */ break;
	case 7: /* MAX-N */
		/* TODO */ break;
	case 8: /* MAX-U */
		/* TODO */ break;
	case 9: /* MAX-UD */
		/* TODO */ break;
	case 10: /* RETURN-STACK-CELLS */
		sloth_push(x, SLOTH_RETURN_STACK_SIZE);
		break;
	case 11: /* STACK-CELLS */
		sloth_push(x, SLOTH_STACK_SIZE);
		break;
	case 12: /* FLOATING-STACK */
		#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

			sloth_push(x, SLOTH_FLOAT_STACK_SIZE);

		#else

			sloth_push(x, -1);

		#endif
		break;
	/* Obsolescent queries (but required for tests) */
	case 100:
		#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

			sloth_push(x, -1);

		#else

			sloth_push(x, 0);

		#endif
		break;
	/* Non standard queries */
	case -1: /* PLATFORM */
		/* This code adapted from: */
		/* https://stackoverflow.com/questions/142508/how-do-i-check-os-with-a-preprocessor-directive */
		#if defined(_WIN64)
			sloth_push(x, 0);
		#elif defined(WIN32) || defined(_WIN32)
			sloth_push(x, 1);
		#elif defined(__CYGWIN__) && !defined(_WIN32)
			sloth_push(x, 2);
		#elif defined(__ANDROID__)
			sloth_push(x, 3);
		#elif defined(__linux__)
			sloth_push(x, 4);
		#else
			sloth_push(x, -1);
		#endif
		break;
	}
}

/* Parsing input */

/* I would prefer using PARSE-NAME but its not yet */
/* standarized and there's no need to implement two */
/* words with almost the same functionality. */
void sloth_word_(X* x) {
	/* The region to store WORD counted strings starts */
	/* at here + CBUF. */
	uCHAR c = (uCHAR)sloth_pop(x);
	CELL ibuf = sloth_user_area_get(x, SLOTH_IBUF);
	CELL ilen = sloth_user_area_get(x, SLOTH_ILEN);
	CELL ipos = sloth_user_area_get(x, SLOTH_IPOS);
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
	sloth_cstore(x, sloth_here(x) + SLOTH_CBUF, end - start);

	for (i = 0; i < (end - start); i++) {
		sloth_cstore(
			x, 
			sloth_here(x) + SLOTH_CBUF + suCHAR + i*suCHAR, 
			sloth_cfetch(x, start + i*suCHAR));
	}
	sloth_push(x, sloth_here(x) + SLOTH_CBUF);

	/* If we are not at the end of the input buffer, */
	/* skip c after the word, but its not part of the counted */
	/* string */
	if (ipos < ilen) ipos++;
	sloth_user_area_set(x, SLOTH_IPOS, ipos);
}

/* Finding words */

/* Helper function to compare a string and a word's name */
/* without case sensitivity. */
int sloth__compare_without_case(X* x, CELL w, CELL t, CELL l) {
	int i;
	if (sloth_get_namelen(x, w) != l) return 0;
	for (i = 0; i < l; i++) {
		uCHAR a = sloth_cfetch(x, t + i);
		uCHAR b = sloth_cfetch(x, sloth_get_name_addr(x, w) + i);
		if (a >= 97 && a <= 122) a -= 32;
		if (b >= 97 && b <= 122) b -= 32;
		if (a != b) return 0;
	}
	return 1;
}

CELL sloth__search_word(X* x, CELL n, int l) {
	CELL wl, w, i;
	for (i = 0; i < sloth_user_area_get(x, SLOTH_ORDER); i++) {
		wl = sloth_user_area_get(x, SLOTH_CONTEXT + i*sCELL);
		w = sloth_fetch(x, wl);
		while (w > 0) {
			if (!sloth_has_flag(x, w, SLOTH_HIDDEN) 
			 && sloth__compare_without_case(x, w, n, l)) 
				return w;
			w = sloth_get_link(x, w);
		}
	}
	return 0;
}

void sloth_find_(X* x) {
	CELL cstring = sloth_pop(x);
	CELL w = sloth__search_word(
		x, 
		cstring + suCHAR, 
		sloth_cfetch(x, cstring)
	);
	if (w == 0) {
		sloth_push(x, cstring);
		sloth_push(x, 0);
	} else if (sloth_has_flag(x, w, SLOTH_IMMEDIATE)) {
		sloth_push(x, sloth_get_xt(x, w));
		sloth_push(x, 1);
	} else {
		sloth_push(x, sloth_get_xt(x, w));
		sloth_push(x, -1);
	}
}

/* Helper to find words from C */
CELL sloth_find_word(X* x, char* name) {
	return sloth__search_word(x, (CELL)name, strlen(name));
}

/* Outer interpreter */

/* INTERPRET is not an ANS word ??!! */
void sloth_interpret_(X* x) {
	CELL nt, flag, n;
	char* tok;
	int tlen;
	char buf[128]; char *endptr;
	int is_double;
	while (sloth_user_area_get(x, SLOTH_IPOS) < sloth_user_area_get(x, SLOTH_ILEN)) {
		sloth_push(x, 32); sloth_word_(x);
		tok = (char*)(sloth_pick(x, 0) + suCHAR);
		tlen = sloth_cfetch(x, sloth_pick(x, 0));
		if (tlen == 0) { sloth_pop(x); return; }
		sloth_find_(x);
		if ((flag = sloth_pop(x)) != 0) {
			if (sloth_user_area_get(x, SLOTH_STATE) == 0
			|| (sloth_user_area_get(x, SLOTH_STATE) != 0 && flag == 1)) {
				sloth_eval(x, sloth_pop(x));	
			} else {
				sloth_compile(x, sloth_pop(x));
			}
		} else {
			CELL temp_base;
			temp_base = sloth_user_area_get(x, SLOTH_BASE);
			sloth_pop(x);
			if (tlen == 3 && *tok == '\'' && (*(tok + 2)) == '\'') {
				/* Character literal */
				if (sloth_user_area_get(x, SLOTH_STATE) == 0)	
					sloth_push(x, *(tok + 1));
				else 
					sloth_literal(x, *(tok + 1));
			} else {
				is_double = 0;
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
				} else if (*(tok + tlen - 1) == '.') {
					tlen--;
					is_double = 1;
				}
				strncpy(buf, tok, tlen);
				buf[tlen] = 0;
				n = strtoll(buf, &endptr, temp_base);	
				if (*endptr == '\0') {
					if (sloth_user_area_get(x, SLOTH_STATE) == 0) {
						sloth_push(x, n);
						if (is_double) sloth_push(x, n < 0 ? -1 : 0);
					} else { 
						sloth_literal(x, n);
						if (is_double) sloth_literal(x, n < 0 ? -1 : 0);
					}
				} else {
				#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

					FCELL r;
					if (sloth_user_area_get(x, SLOTH_BASE) == 10) {
						r = strtod(buf, &endptr);	
						if (r == 0 && buf == endptr) {
							if (sloth_user_area_get(x, SLOTH_SOURCE_ID) != -1) {
								printf("%.*s ?\n", tlen, tok);
							}
							sloth_throw(x, -13);
						} else {
							if (sloth_user_area_get(x, SLOTH_STATE) == 0) {
								sloth_fpush(x, r);
							} else {
								sloth_fliteral(x, r);
							}
						}
					} else {
						if (sloth_user_area_get(x, SLOTH_SOURCE_ID) != -1) {
							printf("%.*s ?\n", tlen, tok);
						}
						sloth_throw(x, -13);
					}

				#else

					if (sloth_user_area_get(x, SLOTH_SOURCE_ID) != -1) {
						printf("%.*s ?\n", tlen, tok);
					}
					sloth_throw(x, -13);

				#endif
				}
			}
		}
	}
}

/* -- Required words to bootstrap ---------------------- */

/* Commands that can help you start or end work sessions */

void sloth_bye_(X* x) { printf("\n"); exit(0); }

/* Commands to inspect memory, debug & view code */

void sloth_depth_(X* x) { sloth_push(x, x->sp); }
void sloth_r_depth_(X* x) { sloth_push(x, x->rp); }
void sloth_unused_(X* x) {
	sloth_push(x, x->d + x->sz - sloth_get(x, SLOTH_HERE)); 
}

/* Source code preprocessing, interpreting & auditing commands */

void sloth_included_(X* x) {
	FILE *f;
	char linebuf[1024];
	CELL INTERPRET, e;

	CELL previbuf = sloth_user_area_get(x, SLOTH_IBUF);
	CELL previpos = sloth_user_area_get(x, SLOTH_IPOS);
	CELL previlen = sloth_user_area_get(x, SLOTH_ILEN);

	CELL prevsourceid = sloth_user_area_get(x, SLOTH_SOURCE_ID);

	size_t l = (size_t)sloth_pop(x);
	char* a = (char*)sloth_pop(x);

	/* Store current path pointers */
	char* prevstart = (char*)sloth_user_area_get(x, SLOTH_PATH_START);
	char* prevend = (char*)sloth_user_area_get(x, SLOTH_PATH_END);
	/* Variables for working with path */
	char* pathstart = prevstart;
	char* pathend = prevend;
	char* path_pos;
	/* Copy filename to end of current path */
	strncpy(pathend, a, l);
	*(pathend + l) = 0;
	/* Try to use it as absolute path filename or relative to */
	/* current directory. */
	f = fopen(pathend, "r");
	if (f) {
		/* Storing path as absolute or relative to cwd */
		pathstart = pathend;
		pathend = pathend + l;
	} else {
		/* Trying as relative to previous path. */
		f = fopen(pathstart, "r");
		if (f) pathend = pathend + l;
	}

	if (f) {
		/* Remove filename from path... */
		while (pathend > pathstart) {
			if (*pathend == '/' || *pathend == '\\') {
				pathend++;
				break;
			}
			pathend--;
		}
		/* ...and store for nested includes. */
		sloth_user_area_set(x, SLOTH_PATH_START, (CELL)pathstart);
		sloth_user_area_set(x, SLOTH_PATH_END, (CELL)pathend);

		INTERPRET = sloth_user_area_get(x, SLOTH_INTERPRET);

		sloth_user_area_set(x, SLOTH_SOURCE_ID, (CELL)f);

		while (fgets(linebuf, 1024, f)) {
			/* printf(">>>> %s\n", linebuf); */
			/* I tried to use _refill from here as the next */
			/* lines of code do exactly the same but, the */
			/* input buffer of the included file is overwritten */
			/* when doing some REFILL from Forth (for an [IF] */
			/* for example). So I left this here to be able to */
			/* use linebuf here. */
			sloth_user_area_set(x, SLOTH_IBUF, (CELL)linebuf);
			sloth_user_area_set(x, SLOTH_IPOS, 0);
			if (linebuf[strlen(linebuf) - 1] < ' ') {
				sloth_user_area_set(x, SLOTH_ILEN, strlen(linebuf) - 1);
			} else {
				sloth_user_area_set(x, SLOTH_ILEN, strlen(linebuf));
			}

			sloth_eval(x, INTERPRET);
		}

		sloth_user_area_set(x, SLOTH_SOURCE_ID, prevsourceid);

		fclose(f);
	} else {
		printf("ERROR: Can't open file (%.*s)\n", (int)l, (char*)a);
	}

	/* Restore previous path */
	sloth_user_area_set(x, SLOTH_PATH_START, (CELL)prevstart);
	sloth_user_area_set(x, SLOTH_PATH_END, (CELL)prevend);

	/* Restore previous input buffer */
	sloth_user_area_set(x, SLOTH_IBUF, previbuf);
	sloth_user_area_set(x, SLOTH_IPOS, previpos);
	sloth_user_area_set(x, SLOTH_ILEN, previlen);
}

/* String operations */

void sloth_move_(X* x) {
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
void sloth_emit_(X* x) { printf("%c", (uCHAR)sloth_pop(x)); }
#endif
#ifndef SLOTH_CUSTOM_KEY
void sloth_key_(X* x) { sloth_push(x, getch()); }
#endif

/* Arithmetic and logical operations */

void sloth_and_(X* x) { CELL v = sloth_pop(x); sloth_push(x, sloth_pop(x) & v); }

void sloth_invert_(X* x) { sloth_push(x, ~sloth_pop(x)); }
void sloth_l_shift_(X* x) { CELL n = sloth_pop(x); sloth_push(x, sloth_pop(x) << n); }

/* Code for _m_star has been created by claude.ai */
void sloth_m_star_(X* x) {
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

void sloth_minus_(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) - a); }

void sloth_plus_(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) + a); }

/* Code adapted from pForth */
void sloth_d_plus_(X* x) { 
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

void sloth_r_shift_(X* x) { 
	CELL n = sloth_pop(x); 
	sloth_push(x, ((uCELL)sloth_pop(x)) >> n); 
}

void sloth_star_(X* x) { CELL b = sloth_pop(x); sloth_push(x, sloth_pop(x) * b); }

void sloth_two_slash_(X* x) { sloth_push(x, sloth_pop(x) >> 1); }
void sloth_u_m_star_(X* x) {
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
void sloth_u_m_slash_mod_(X* x) {
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

void sloth_c_fetch_(X* x) { sloth_push(x, sloth_cfetch(x, sloth_pop(x))); }
void sloth_c_store_(X* x) { CELL a = sloth_pop(x); sloth_cstore(x, a, sloth_pop(x)); }
void sloth_fetch_(X* x) { 
	sloth_push(x, sloth_fetch(x, sloth_pop(x))); 
}
void sloth_store_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_store(x, a, sloth_pop(x)); 
}

/* Comparison operations */

void sloth_equals_(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) == a ? -1 : 0); }
void sloth_less_than_(X* x) { CELL a = sloth_pop(x); sloth_push(x, sloth_pop(x) < a ? -1 : 0); }

/* More facilities for defining routines (compiling-mode only) */

void sloth_colon_(X* x) {
	CELL tok, tlen;
	sloth_push(x, 32); sloth_word_(x);
	tok = sloth_pick(x, 0) + suCHAR;
	tlen = sloth_cfetch(x, sloth_pop(x));
	sloth_header(x, tok, tlen);
	sloth_user_area_set(x, SLOTH_LATESTXT, sloth_get_xt(x, sloth_get_latest(x)));
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_HIDDEN);
	sloth_user_area_set(x, SLOTH_STATE, 1);
}
void sloth_colon_no_name_(X* x) { 
	sloth_push(x, sloth_here(x));
	sloth_user_area_set(x, SLOTH_LATESTXT, sloth_here(x));
	sloth_user_area_set(x, SLOTH_STATE, 1);
}
void sloth_semicolon_(X* x) {
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_user_area_set(x, SLOTH_STATE, 0);
	/* Don't change flags for nonames */
	if (sloth_get_xt(x, sloth_get_latest(x)) == sloth_user_area_get(x, SLOTH_LATESTXT))
		sloth_unset_flag(x, sloth_get_latest(x), SLOTH_HIDDEN);
}

void sloth_recurse_(X* x) { sloth_compile(x, sloth_user_area_get(x, SLOTH_LATESTXT)); }
void sloth_catch_(X* x) { sloth_catch(x, sloth_pop(x)); }
void sloth_throw_(X* x) { 
	CELL e = sloth_pop(x); 
	if (e != 0) {
		if (e == -2) {
			int l = (int)sloth_pop(x);
			char *a = (char*)sloth_pop(x);
			if (x->jmpbuf_idx < 0) {
				printf("Error: %.*s\n", l, a);
			}
		}
		sloth_throw(x, e);
	}
}

/* Manipulating stack items */

void sloth_drop_(X* x) { 
	if (x->sp <= 0) 
		sloth_throw(x, -4); 
	else 
		sloth_pop(x); 
}
void sloth_over_(X* x) { sloth_push(x, sloth_pick(x, 1)); }
void sloth_pick_(X* x) {  sloth_push(x, sloth_pick(x, sloth_pop(x))); }
void sloth_r_pick_(X* x) { sloth_push(x, sloth_rpick(x, sloth_pop(x))); }
void sloth_to_r_(X* x) { sloth_rpush(x, sloth_pop(x)); }
void sloth_r_from_(X* x) { sloth_push(x, sloth_rpop(x)); }
void sloth_swap_(X* x) { CELL a = sloth_pop(x); CELL b = sloth_pop(x);sloth_push(x, a); sloth_push(x, b); }

/* Constructing compiler and interpreter system extensions */

void sloth_allot_(X* x) { sloth_allot(x, sloth_pop(x)); }
void sloth_cells_(X* x) { sloth_push(x, sloth_pop(x) * sCELL); }
void sloth_chars_(X* x) { /* Does nothing */ }
void sloth_compile_comma_(X* x) { sloth_compile(x, sloth_pop(x)); }
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
void sloth_create_(X* x) {
	CELL tok, tlen;
	sloth_push(x, 32); sloth_word_(x);
	tok = sloth_pick(x, 0) + suCHAR;
	tlen = sloth_cfetch(x, sloth_pop(x));
	sloth_header(x, tok, tlen);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(RIP)"))); 
	sloth_compile(x, 4*sCELL); 
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT"))); 
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
}
/* Helper compiled by DOES> that replaces the first EXIT */
/* compiled by CREATE on the new created word with a call */
/* to the code after the DOES> in the CREATE DOES> word */
void sloth_do_does_(X* x) {
	CELL a = sloth_pop(x);
	sloth_store(x, sloth_get_xt(x, sloth_get_latest(x)) + 2*sCELL, a);
}

void sloth_does_(X* x) {
	sloth_literal(x, sloth_here(x) + 4*sCELL);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(DOES)")));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
}
void sloth_evaluate_(X* x) {
	CELL e;
	CELL l = sloth_pop(x), a = sloth_pop(x);

	CELL previbuf = sloth_user_area_get(x, SLOTH_IBUF);
	CELL previpos = sloth_user_area_get(x, SLOTH_IPOS);
	CELL previlen = sloth_user_area_get(x, SLOTH_ILEN);

	CELL prevsourceid = sloth_user_area_get(x, SLOTH_SOURCE_ID);

	sloth_user_area_set(x, SLOTH_SOURCE_ID, -1);

	sloth_user_area_set(x, SLOTH_IBUF, a);
	sloth_user_area_set(x, SLOTH_IPOS, 0);
	sloth_user_area_set(x, SLOTH_ILEN, l);

	/* To ensure that the input buffer is restored correctly */
	/* even in case of a throw, I catch any possible throw */
	/* here and rethrow it after restoring the input buffer. */
	sloth_catch(x, sloth_user_area_get(x, SLOTH_INTERPRET));
		
	sloth_user_area_set(x, SLOTH_SOURCE_ID, prevsourceid);

	sloth_user_area_set(x, SLOTH_IBUF, previbuf);
	sloth_user_area_set(x, SLOTH_IPOS, previpos);
	sloth_user_area_set(x, SLOTH_ILEN, previlen);
	
	e = sloth_pop(x);
	if (e != 0) {
		sloth_throw(x, e);
	}
}
void sloth_execute_(X* x) { sloth_eval(x, sloth_pop(x)); }
void sloth_here_(X* x) { sloth_push(x, sloth_here(x)); }

void sloth_immediate_(X* x) { 
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_IMMEDIATE); 
}
/* Not needed anymore
void sloth_to_in_(X* x) { 
	sloth_push(x, sloth_to_abs(x, SLOTH_IPOS)); 
}
*/
void sloth_postpone_(X* x) { 
	CELL i, xt, tok, tlen;
	sloth_push(x, 32); sloth_word_(x);
	tok = sloth_pick(x, 0) + suCHAR;
	tlen = sloth_cfetch(x, sloth_pick(x, 0));
	if (tlen == 0) { sloth_pop(x); return; }
	sloth_find_(x); 
	i = sloth_pop(x);
	xt = sloth_pop(x);
	if (i == 0) { 
		return;
	} else if (i == -1) {
		/* Compile the compilation of the normal word */
		sloth_literal(x, xt);
		sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(COMPILE)")));
	} else if (i == 1) {
		/* Compile the immediate word */

		sloth_compile(x, xt);
	}
}
void sloth_refill_(X* x) {
	char linebuf[1024];
	int i;
	switch (sloth_user_area_get(x, SLOTH_SOURCE_ID)) {
	case -1: 
		sloth_push(x, 0);
		break;
	case 0:
		sloth_push(x, sloth_user_area_get(x, SLOTH_IBUF)); 
		sloth_push(x, 80);
		sloth_eval(x, sloth_get_xt(x, sloth_find_word(x, "ACCEPT")));
		sloth_user_area_set(x, SLOTH_ILEN, sloth_pop(x));
		sloth_user_area_set(x, SLOTH_IPOS, 0);
		sloth_push(x, -1); 
		break;
	default: 
		if (fgets(linebuf, 1024, (FILE *)sloth_user_area_get(x, SLOTH_SOURCE_ID))) {
			sloth_user_area_set(x, SLOTH_IBUF, (CELL)linebuf);
			sloth_user_area_set(x, SLOTH_IPOS, 0);
			/* Although I haven't found anywhere that \n should */
			/* not be part of the input buffer when reading from */
			/* a file, the results from preliminary tests when */
			/* using SOURCE ... TYPE add newlines (because they */
			/* are present) and on some other Forths they do not. */
			/* So I just added a check to remove the \n at then */
			/* end. */
			if (linebuf[strlen(linebuf) - 1] < ' ') {
				sloth_user_area_set(x, SLOTH_ILEN, strlen(linebuf) - 1);
			} else {
				sloth_user_area_set(x, SLOTH_ILEN, strlen(linebuf));
			}
			sloth_push(x, -1);
		} else {
			sloth_push(x, 0);
		}
		break;
	}	
}

void sloth_source_(X* x) { 
	sloth_push(x, sloth_user_area_get(x, SLOTH_IBUF)); 
	sloth_push(x, sloth_user_area_get(x, SLOTH_ILEN)); 
}

/* == Helpers and bootstrapping ======================== */

/* -- Helpers to add primitives to the dictionary ------ */

CELL sloth_primitive(X* x, F f) { 
	assert(x->p->last < x->p->sz);
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL sloth_code(X* x, char* name, CELL xt) {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, xt);
	return xt; 
}

void sloth_user_variable(X* x, char*name, CELL d, CELL v) {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, sloth_here(x));
	sloth_literal(x, (CELL)(x->u + d));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_store(x, x->u + d, v);
}

/* Helper to work with absolute/relative memory addresses */

void sloth_to_abs_(X* x) { sloth_push(x, sloth_pop(x) + x->d); }
void sloth_to_rel_(X* x) { sloth_push(x, sloth_pop(x) - x->d); }

/* Helper to empty the return stack */

void sloth_empty_rs_(X* x) { x->rp = 0; }

/* Helper to work with structs with C data sizes */

void sloth_ints_(X* x) { sloth_push(x, sloth_pop(x)*sizeof(int)); }

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_kernel(X* x) {

	/* TODO Better comments for memory initialization */

	/* Initialization of dictionary */

	*((CELL*)x->d) = sloth_to_abs(x, sCELL); /* HERE */
	sloth_comma(x, 0); /* INTERNAL-WORDLIST */
	sloth_comma(x, 0); /* FORTH-WORDLIST */

	/* Initialization of user area */

	*((CELL*)x->u) = sloth_to_abs(x, SLOTH_FORTH_WL); /* CURRENT */
	*((CELL*)(x->u + 1*sCELL)) = 2; /* #ORDER */
	/* CONTEXT 0 */
	*((CELL*)(x->u + 2*sCELL)) = sloth_to_abs(x, SLOTH_FORTH_WL);
	/* CONTEXT 1 */
	*((CELL*)(x->u + 3*sCELL)) = sloth_to_abs(x, SLOTH_INTERNAL_WL);
	
	/* Basic primitives */

	/* EXIT and (LIT) must be defined before using */
	/* user_area_variable. */

	sloth_code(x, "EXIT", sloth_primitive(x, &sloth_exit_));

	sloth_user_area_set(x, SLOTH_CURRENT, sloth_to_abs(x, SLOTH_INTERNAL_WL));

	sloth_code(x, "(LIT)", sloth_primitive(x, &sloth_lit_));

	/* User area variables */

	/* TODO Comment why (CURREN) #ORDER and CONTEXT are */
	/* reinitialized. */

	sloth_user_variable(x, "(CURRENT)", SLOTH_CURRENT, sloth_to_abs(x, SLOTH_FORTH_WL));
	sloth_user_variable(x, "#ORDER", SLOTH_ORDER, 2);
	sloth_user_variable(x, "CONTEXT", SLOTH_CONTEXT, sloth_to_abs(x, SLOTH_FORTH_WL));
	sloth_user_variable(x, "BASE", SLOTH_BASE, 10);
	sloth_user_variable(x, "STATE", SLOTH_STATE, 0);
	sloth_user_variable(x, "(IBUF)", SLOTH_IBUF, 0);
	sloth_user_variable(x, ">IN", SLOTH_IPOS, 0);
	sloth_user_variable(x, "(ILEN)", SLOTH_ILEN, 0);
	sloth_user_variable(x, "(SOURCE-ID)", SLOTH_SOURCE_ID, 0);
	sloth_user_variable(x, "(LATESTXT)", SLOTH_LATESTXT, 0);
	/* TODO IX, JX, KX, LX could be registers of the context */
	/* and I, J the words used. Think about it. */
	sloth_user_variable(x, "(IX)", SLOTH_IX, 0);
	sloth_user_variable(x, "(JX)", SLOTH_JX, 0);
	sloth_user_variable(x, "(KX)", SLOTH_KX, 0);
	sloth_user_variable(x, "(LX)", SLOTH_LX, 0);
	sloth_user_variable(x, "(INTERPRET)", SLOTH_INTERPRET, 0);

	sloth_user_variable(x, "(SLOTH_PATH_START)", SLOTH_PATH_START, x->u + SLOTH_PATH);
	sloth_user_variable(x, "(SLOTH_PATH_END)", SLOTH_PATH_END, x->u + SLOTH_PATH);
	sloth_user_variable(x, "(SLOTH_PATH)", SLOTH_PATH, 0);

	#ifdef SLOTH_FLOATING_POINT_WORD_SET_HEADER

	sloth_user_variable(x, "(PRECISION)", SLOTH_PRECISION, 15);

	#endif

	/* -- Primitives */

	sloth_code(x, "(RIP)", sloth_primitive(x, &sloth_rip_));
	sloth_code(x, "(COMPILE)", sloth_primitive(x, &sloth_compile_));
	sloth_code(x, "(BRANCH)", sloth_primitive(x, &sloth_branch_));
	sloth_code(x, "(?BRANCH)", sloth_primitive(x, &sloth_zbranch_));
	sloth_code(x, "(STRING)", sloth_primitive(x, &sloth_string_));
	sloth_code(x, "(CSTRING)", sloth_primitive(x, &sloth_c_string_));
	sloth_code(x, "(QUOTATION)", sloth_primitive(x, &sloth_quotation_));
	sloth_code(x, "(DOES)", sloth_primitive(x, &sloth_do_does_));
	sloth_code(x, "(DOLOOP)", sloth_primitive(x, &sloth_doloop_));
	sloth_code(x, "(ENVIRONMENT)", sloth_primitive(x, &sloth_environment_));

	/* Quotations */

	sloth_code(x, "[:", sloth_primitive(x, &sloth_start_quotation_)); sloth_immediate_(x);
	sloth_code(x, ";]", sloth_primitive(x, &sloth_end_quotation_)); sloth_immediate_(x);

	/* Commands that can help you start or end work sessions */

	sloth_code(x, "UNUSED", sloth_primitive(x, &sloth_unused_));
	sloth_code(x, "BYE", sloth_primitive(x, &sloth_bye_));

	/* Commands to inspect memory, debug & view code */

	sloth_code(x, "DEPTH", sloth_primitive(x, &sloth_depth_));
	sloth_code(x, "RDEPTH", sloth_primitive(x, &sloth_r_depth_));

	/* Source code preprocessing, interpreting & auditing commands */

	sloth_code(x, "INCLUDED", sloth_primitive(x, &sloth_included_));

	/* String operations */

	sloth_code(x, "MOVE", sloth_primitive(x, &sloth_move_));
	
	/* More input/output operations */

	sloth_code(x, "EMIT", sloth_primitive(x, &sloth_emit_));
	sloth_code(x, "KEY", sloth_primitive(x, &sloth_key_));

	/* Arithmetic and logical operations */

	sloth_code(x, "AND", sloth_primitive(x, &sloth_and_));
	sloth_code(x, "INVERT", sloth_primitive(x, &sloth_invert_));
	sloth_code(x, "LSHIFT", sloth_primitive(x, &sloth_l_shift_));
	sloth_code(x, "M*", sloth_primitive(x, &sloth_m_star_));
	sloth_code(x, "-", sloth_primitive(x, &sloth_minus_));
	sloth_code(x, "+", sloth_primitive(x, &sloth_plus_));
	sloth_code(x, "D+", sloth_primitive(x, &sloth_d_plus_));
	sloth_code(x, "RSHIFT", sloth_primitive(x, &sloth_r_shift_));
	sloth_code(x, "*", sloth_primitive(x, &sloth_star_));
	sloth_code(x, "2/", sloth_primitive(x, &sloth_two_slash_));
	sloth_code(x, "UM*", sloth_primitive(x, &sloth_u_m_star_));
	sloth_code(x, "UM/MOD", sloth_primitive(x, &sloth_u_m_slash_mod_));

	/* Memory-stack transfer operations */

	sloth_code(x, "C@", sloth_primitive(x, &sloth_c_fetch_));
	sloth_code(x, "C!", sloth_primitive(x, &sloth_c_store_));
	sloth_code(x, "@", sloth_primitive(x, &sloth_fetch_));
	sloth_code(x, "!", sloth_primitive(x, &sloth_store_));

	/* Comparison operations */

	sloth_code(x, "=", sloth_primitive(x, &sloth_equals_));
	sloth_code(x, "<", sloth_primitive(x, &sloth_less_than_));

	/* Forming definite loops */

	sloth_code(x, "UNLOOP", sloth_primitive(x, &sloth_unloop_));

	/* More facilities for defining routines (compiling-mode only) */

	sloth_code(x, ":", sloth_primitive(x, &sloth_colon_));
	sloth_code(x, ":NONAME", sloth_primitive(x, &sloth_colon_no_name_));
	sloth_code(x, ";", sloth_primitive(x, &sloth_semicolon_)); sloth_immediate_(x);

	sloth_code(x, "CATCH", sloth_primitive(x, &sloth_catch_));
	sloth_code(x, "THROW", sloth_primitive(x, &sloth_throw_));

	/* Manipulating stack items */

	sloth_code(x, "DROP", sloth_primitive(x, &sloth_drop_));
	sloth_code(x, "OVER", sloth_primitive(x, &sloth_over_));
	sloth_code(x, "PICK", sloth_primitive(x, &sloth_pick_));
	sloth_code(x, "RPICK", sloth_primitive(x, &sloth_r_pick_));
	sloth_code(x, ">R", sloth_primitive(x, &sloth_to_r_));
	sloth_code(x, "R>", sloth_primitive(x, &sloth_r_from_));
	sloth_code(x, "SWAP", sloth_primitive(x, &sloth_swap_));

	sloth_code(x, "RECURSE", sloth_primitive(x, &sloth_recurse_)); sloth_immediate_(x);

	/* Constructing compiler and interpreter system extensions */

	sloth_code(x, "ALLOT", sloth_primitive(x, &sloth_allot_));
	sloth_code(x, "CELLS", sloth_primitive(x, &sloth_cells_));
	sloth_code(x, "CHARS", sloth_primitive(x, &sloth_chars_));
	sloth_code(x, "COMPILE,", sloth_primitive(x, &sloth_compile_comma_));
	sloth_code(x, "CREATE", sloth_primitive(x, &sloth_create_));
	sloth_code(x, "DOES>", sloth_primitive(x, &sloth_does_)); sloth_immediate_(x);
	sloth_code(x, "EVALUATE", sloth_primitive(x, &sloth_evaluate_));
	sloth_code(x, "EXECUTE", sloth_primitive(x, &sloth_execute_));
	sloth_code(x, "HERE", sloth_primitive(x, &sloth_here_));
	sloth_code(x, "IMMEDIATE", sloth_primitive(x, &sloth_immediate_));
	/* sloth_code(x, ">IN", sloth_primitive(x, &sloth_to_in_)); */
	sloth_code(x, "POSTPONE", sloth_primitive(x, &sloth_postpone_)); sloth_immediate_(x);
	sloth_code(x, "REFILL", sloth_primitive(x, &sloth_refill_));
	sloth_code(x, "SOURCE", sloth_primitive(x, &sloth_source_));
	sloth_code(x, "WORD", sloth_primitive(x, &sloth_word_));

	sloth_code(x, "FIND", sloth_primitive(x, &sloth_find_));

	/* == Helpers ======================================== */

	sloth_code(x, "TO-ABS", sloth_primitive(x, &sloth_to_abs_));
	sloth_code(x, "TO-REL", sloth_primitive(x, &sloth_to_rel_));
	sloth_user_area_set(x, SLOTH_INTERPRET, sloth_primitive(x, &sloth_interpret_));
	sloth_code(x, "(EMPTY-RETURN-STACK)", sloth_primitive(x, &sloth_empty_rs_));

	sloth_code(x, "INTS", sloth_primitive(x, &sloth_ints_));
}

#ifndef SLOTH_FLOATING_POINT_WORD_SET_HEADER

	void sloth_bootstrap(X* x) {
		sloth_bootstrap_kernel(x);
	}

#endif

/* Helpers to work with files from C */

void sloth_include(X* x, char* f) {
	sloth_push(x, (CELL)f);
	sloth_push(x, strlen(f));
	sloth_included_(x);
}

void sloth_evaluate(X* x, char* s) {
	sloth_push(x, (CELL)s);
	sloth_push(x, (CELL)strlen(s));
	sloth_evaluate_(x);
}

/* Helper REPL */

void sloth_repl(X* x) {
	char buf[125];
	sloth_user_area_set(x, SLOTH_IBUF, (CELL)buf);
	sloth_user_area_set(x, SLOTH_IPOS, 0);
	sloth_user_area_set(x, SLOTH_ILEN, 80);
	sloth_eval(x, sloth_get_xt(x, sloth_find_word(x, "QUIT")));
}

#endif
#endif
