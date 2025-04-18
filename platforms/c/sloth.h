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
#ifndef SLOTH_NO_FLOATING_POINT
typedef double FLOAT;
typedef float SFLOAT;
typedef double DFLOAT;
#endif

#define sCELL sizeof(CELL)
#define suCHAR sizeof(uCHAR)
#define CELL_BITS sCELL*8
#ifndef SLOTH_NO_FLOATING_POINT
#define sFLOAT sizeof(FLOAT)
#define sSFLOAT sizeof(SFLOAT)
#define sDFLOAT sizeof(DFLOAT)
#endif

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
	#ifndef SLOTH_NO_FLOATING_POINT
	FLOAT f[SLOTH_FLOAT_STACK_SIZE]; 
	CELL fp;
	#endif
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

#ifndef SLOTH_NO_FLOATING_POINT
/* -- Floating point stack ----------------------------- */

void sloth_fpush(X* x, FLOAT v);
FLOAT sloth_fpop(X* x);
FLOAT sloth_fpick(X* x, CELL a);
#endif

/* -- Memory ------------------------------------------- */

void sloth_store(X* x, CELL a, CELL v);
CELL sloth_fetch(X* x, CELL a);
void sloth_cstore(X* x, CELL a, uCHAR v);
uCHAR sloth_cfetch(X* x, CELL a);
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fstore(X* x, CELL a, FLOAT v);
FLOAT sloth_ffetch(X* x, CELL a);
void sloth_sfstore(X* x, CELL a, SFLOAT v);
SFLOAT sloth_sffetch(X* x, CELL a);
void sloth_dfstore(X* x, CELL a, DFLOAT v);
DFLOAT sloth_dffetch(X* x, CELL a);
#endif

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
#define SLOTH_BASE							sCELL
#define SLOTH_FORTH_WORDLIST		2*sCELL	/* Not used in C */
#define SLOTH_INTERNAL_WORDLIST	3*sCELL
#define SLOTH_STATE							4*sCELL
#define SLOTH_IBUF							5*sCELL
#define SLOTH_IPOS							6*sCELL
#define SLOTH_ILEN							7*sCELL
#define SLOTH_SOURCE_ID					8*sCELL
#define SLOTH_HLD								9*sCELL /* Not used in C */
#define SLOTH_LATESTXT					10*sCELL
#define SLOTH_IX								11*sCELL
#define SLOTH_JX								12*sCELL
#define SLOTH_KX								13*sCELL
#define SLOTH_LX								14*sCELL
#define SLOTH_CURRENT						15*sCELL
#define SLOTH_ORDER							16*sCELL
#define SLOTH_CONTEXT						17*sCELL

/* Word statuses */

#define SLOTH_HIDDEN					1
#define SLOTH_IMMEDIATE				2

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void sloth_set(X* x, CELL a, CELL v);
CELL sloth_get(X* x, CELL a);

void sloth_cset(X* x, CELL a, uCHAR v);
uCHAR sloth_cget(X* x, CELL a);

#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fset(X* x, CELL a, FLOAT v);
FLOAT sloth_fget(X* x, CELL a);
#endif

/* Memory management */

CELL sloth_here(X* x);
void sloth_allot(X* x, CELL v);
CELL sloth_aligned(CELL a);
void sloth_align(X* x);

/* Compilation */

void sloth_comma(X* x, CELL v);
void sloth_ccomma(X* x, uCHAR v);
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fcomma(X* x, FLOAT v);
#endif

void sloth_compile(X* x, CELL xt);
void sloth_literal(X* x, CELL n);
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fliteral(X* x, FLOAT n);
#endif

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
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_flit_(X* x);
#endif
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
void sloth_s_m_slash_rem_(X* x);
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
void sloth_to_in_(X* x);
void sloth_postpone_(X* x);
void sloth_refill_(X* x);
void sloth_source_(X* x);

/* == Floating point word set ========================== */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x);
void sloth_f_aligned_(X* x);
void sloth_f_literal_(X* x);
void sloth_floats_(X* x);
void sloth_float_plus_(X* x);

void sloth_s_f_aligned_(X* x);
void sloth_d_f_aligned_(X* x);

void sloth_s_floats_(X* x);
void sloth_d_floats_(X* x);

/* Manipulating stack items */

void sloth_f_depth_(X* x);
void sloth_f_drop_(X* x);
void sloth_f_dup_(X* x);
void sloth_f_over_(X* x);
void sloth_f_rot_(X* x);
void sloth_f_swap_(X* x);

/* Comparison operations */

void sloth_f_less_than_(X* x);
void sloth_f_zero_less_than_(X* x);
void sloth_f_zero_equals_(X* x);

/* Memory-stack transfer operations */

void sloth_f_fetch_(X* x);
void sloth_f_store_(X* x);
void sloth_s_f_fetch_(X* x);
void sloth_s_f_store_(X* x);
void sloth_d_f_fetch_(X* x);
void sloth_d_f_store_(X* x);

/* Commands to define data structures */

void sloth_f_constant_(X* x);
void sloth_f_variable_(X* x);

/* Number-type conversion operators */

void sloth_d_to_f_(X* x);
void sloth_f_to_d_(X* x);
void sloth_s_to_f_(X* x);

/* Arithmetic and logical operations */

void sloth_f_plus_(X* x);
void sloth_f_minus_(X* x);
void sloth_f_star_(X* x);
void sloth_f_slash_(X* x);
void sloth_floor_(X* x);
void sloth_f_max_(X* x);
void sloth_f_min_(X* x);
void sloth_f_negate_(X* x);
void sloth_f_round_(X* x);

/* String/numeric conversion */

void sloth_to_float_(X* x);
void sloth_represent_(X* x);

/* Non ANS floating point helpers */

void sloth_f_dot_s_(X* x);

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

void sloth_init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;
	#ifndef SLOTH_NO_FLOATING_POINT
	x->fp = 0;
	#endif

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

#ifndef SLOTH_NO_FLOATING_POINT
/* -- Floating point stack ----------------------------- */

void sloth_fpush(X* x, FLOAT v) { x->f[x->fp] = v; x->fp++; }
FLOAT sloth_fpop(X* x) { x->fp--; return x->f[x->fp]; }
FLOAT sloth_fpick(X* x, CELL a) { return x->f[x->fp - a - 1]; }
#endif

/* -- Memory ------------------------------------------- */

/* 
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void sloth_store(X* x, CELL a, CELL v) { *((CELL*)a) = v; }
CELL sloth_fetch(X* x, CELL a) { return *((CELL*)a); }
void sloth_cstore(X* x, CELL a, uCHAR v) { *((uCHAR*)a) = v; }
uCHAR sloth_cfetch(X* x, CELL a) { return *((uCHAR*)a); }
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fstore(X* x, CELL a, FLOAT v) { *((FLOAT*)a) = v; }
FLOAT sloth_ffetch(X* x, CELL a) { return *((FLOAT*)a); }
void sloth_sfstore(X* x, CELL a, SFLOAT v) { *((SFLOAT*)a) = v; }
SFLOAT sloth_sffetch(X* x, CELL a) { return *((SFLOAT*)a); }
void sloth_dfstore(X* x, CELL a, DFLOAT v) { *((DFLOAT*)a) = v; }
DFLOAT sloth_dffetch(X* x, CELL a) { return *((DFLOAT*)a); }
#endif

/*
The next two macros allow transforming from relative to
absolute addresses.
*/
CELL sloth_to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL sloth_to_rel(X* x, CELL a) { return a - x->d; }

/* -- Inner interpreter -------------------------------- */

CELL sloth_op(X* x) { 
	CELL o = sloth_fetch(x, sloth_to_abs(x, x->ip));
	x->ip += sCELL;
	return o; 
}

#ifndef SLOTH_NO_FLOATING_POINT
FLOAT sloth_fop(X* x) {
	FLOAT n = sloth_ffetch(x, sloth_to_abs(x, x->ip));
	x->ip += sFLOAT;
	return n;
}
#endif

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
	int tsp = x->sp;
	int trp = x->rp;
	int tip = x->ip;
	int e;

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
		printf("Exception: %ld\n", e);
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

#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fset(X* x, CELL a, FLOAT v) { 
	sloth_fstore(x, sloth_to_abs(x, a), v); 
}
FLOAT sloth_fget(X* x, CELL a) { 
	return sloth_ffetch(x, sloth_to_abs(x, a)); 
}
#endif

/* Memory management */

CELL sloth_here(X* x) { return sloth_get(x, SLOTH_HERE); }
void sloth_allot(X* x, CELL v) { 
	sloth_set(x, SLOTH_HERE, sloth_get(x, SLOTH_HERE) + v); 
}
/* CELL sloth_aligned(CELL a) { return (a + (sCELL - 1)) & ~(sCELL - 1); } */
CELL sloth_aligned(CELL a) { return ALIGNED(a, sCELL); }
void sloth_align(X* x) { 
	sloth_set(
		x, 
		SLOTH_HERE, 
/*		(sloth_get(x, SLOTH_HERE) + (sCELL - 1)) & ~(sCELL - 1));  */
		ALIGNED(sloth_get(x, SLOTH_HERE), sCELL));
}

/* Compilation */

void sloth_comma(X* x, CELL v) { 
	sloth_set(x, sloth_here(x), v); 
	sloth_allot(x, sCELL); 
}
void sloth_ccomma(X* x, uCHAR v) { 
	sloth_set(x, sloth_here(x), v); 
	sloth_allot(x, suCHAR); 
}
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fcomma(X* x, FLOAT v) { 
	sloth_fset(x, sloth_here(x), v); 
	sloth_allot(x, sFLOAT); 
}
#endif


void sloth_compile(X* x, CELL xt) { sloth_comma(x, xt); }

void sloth_literal(X* x, CELL n) { 
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(LIT)")));
	sloth_comma(x, n); 
}

#ifndef SLOTH_NO_FLOATING_POINT
void sloth_fliteral(X* x, FLOAT n) {
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(FLIT)")));
	sloth_fcomma(x, n);
}
#endif

/* Headers */

CELL sloth_get_latest(X* x) { 
	return sloth_fetch(x, sloth_get(x, SLOTH_CURRENT)); 
}
void sloth_set_latest(X* x, CELL w) { 
	sloth_store(x, sloth_get(x, SLOTH_CURRENT), w); 
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
	sloth_set(x, w + sCELL, sloth_here(x));
	return w;
}

CELL sloth_get_link(X* x, CELL w) { return sloth_get(x, w); }

CELL sloth_get_xt(X* x, CELL w) { return sloth_get(x, w + sCELL); }
void sloth_set_xt(X* x, CELL w, CELL xt) { sloth_set(x, w + sCELL, xt); }

uCHAR sloth_get_flags(X* x, CELL w) { return sloth_cget(x, w + 2*sCELL); }
CELL sloth_has_flag(X* x, CELL w, CELL v) { return sloth_get_flags(x, w) & v; }

uCHAR sloth_get_namelen(X* x, CELL w) { 
	return sloth_cget(x, w + 2*sCELL + suCHAR); 
}
CELL sloth_get_name_addr(X* x, CELL w) { 
	return sloth_to_abs(x, w + 2*sCELL + 2*suCHAR); 
}

/* Setting flags */

void sloth_set_flag(X* x, CELL w, uCHAR v) { 
	sloth_cset(x, w + 2*sCELL, sloth_get_flags(x, w) | v); 
}
void sloth_unset_flag(X* x, CELL w, uCHAR v) { 
	sloth_cset(x, w + 2*sCELL, sloth_get_flags(x, w) & ~v); 
}

/* -- Primitives -------------------------------------- */

/* _exit does exist, so p_exit is used (from sloth_primitive) */

void sloth_exit_(X* x) { 
	x->ip = (x->rp > 0) ? sloth_rpop(x) : -1; 
}
void sloth_lit_(X* x) { sloth_push(x, sloth_op(x)); }
#ifndef SLOTH_NO_FLOATING_POINT
void sloth_flit_(X* x) { sloth_fpush(x, sloth_fop(x)); }
#endif
void sloth_rip_(X* x) { 
	sloth_push(
		x, 
		sloth_to_abs(x, x->ip) + sloth_op(x) - sCELL); 
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
	sloth_push(x, sloth_to_abs(x, x->ip)); 
	sloth_push(x, l); 
	x->ip = sloth_aligned(x->ip + l + 1); 
}

void sloth_c_string_(X* x) { 
	uCHAR l = sloth_cfetch(x, sloth_to_abs(x, x->ip)); 
	sloth_push(x, sloth_to_abs(x, x->ip)); 
	x->ip = sloth_aligned(x->ip + l + 2);
}

/* Quotations (not in ANS Forth yet) */

void sloth_quotation_(X* x) { 
	CELL d = sloth_op(x); 
	sloth_push(x, x->ip); 
	x->ip += d; 
}
void sloth_start_quotation_(X* x) {
	CELL s = sloth_get(x, SLOTH_STATE);
	sloth_set(x, SLOTH_STATE, s <= 0 ? s - 1 : s + 1);
	if (sloth_get(x, SLOTH_STATE) == -1) 
		sloth_push(x, sloth_here(x) + 2*sCELL);
	sloth_push(x, sloth_get(x, SLOTH_LATESTXT));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(QUOTATION)")));
	sloth_push(x, sloth_to_abs(x, sloth_here(x)));
	sloth_comma(x, 0);
	sloth_set(x, SLOTH_LATESTXT, sloth_here(x));
}

void sloth_end_quotation_(X* x) {
	CELL s = sloth_get(x, SLOTH_STATE), a = sloth_pop(x);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_store(x, a, sloth_to_abs(x, sloth_here(x)) - a - sCELL);
	sloth_set(x, SLOTH_LATESTXT, sloth_pop(x));
	sloth_set(x, SLOTH_STATE, s < 0 ? s + 1 : s - 1);
}

/* Loop helpers */

void sloth__ipush(X* x) { 
	sloth_rpush(x, sloth_get(x, SLOTH_KX));
	sloth_set(x, SLOTH_KX, sloth_get(x, SLOTH_JX));
	sloth_set(x, SLOTH_JX, sloth_get(x, SLOTH_IX));
	sloth_set(x, SLOTH_LX, 0);
}
void sloth__ipop(X* x) { 
	sloth_set(x, SLOTH_LX, 0);
	sloth_set(x, SLOTH_IX, sloth_get(x, SLOTH_JX));
	sloth_set(x, SLOTH_JX, sloth_get(x, SLOTH_KX));
	sloth_set(x, SLOTH_KX, sloth_rpop(x));
}

void sloth_unloop_(X* x) { 
	sloth_set(x, SLOTH_LX, sloth_get(x, SLOTH_LX) - 1);
	if (sloth_get(x, SLOTH_LX) == -1) {
		sloth_set(x, SLOTH_IX, sloth_get(x, SLOTH_JX));
		sloth_set(x, SLOTH_JX, sloth_get(x, SLOTH_KX));
		sloth_set(x, SLOTH_KX, sloth_rpick(x, 1));
	} else if (sloth_get(x, SLOTH_LX) == -2) {
		sloth_set(x, SLOTH_IX, sloth_get(x, SLOTH_JX));
		sloth_set(x, SLOTH_JX, sloth_get(x, SLOTH_KX));
		sloth_set(x, SLOTH_KX, sloth_rpick(x, 3));
	}
}

/* Algorithm for doloop taken from pForth */
/* (pf_inner.c case ID_PLUS_LOOP) */
void sloth_doloop_(X* x) {
	CELL q, do_first_loop, l, o, d;
	sloth__ipush(x);
	q = sloth_pop(x);
	do_first_loop = sloth_pop(x);
	sloth_set(x, SLOTH_IX, sloth_pop(x));
	l = sloth_pop(x);

	o = sloth_get(x, SLOTH_IX) - l;
	d = 0;

	/* First iteration is executed always on a DO */
	if (do_first_loop == 1) {
		sloth_eval(x, q);
		if (sloth_get(x, SLOTH_LX) == 0) {
			d = sloth_pop(x);
			o = sloth_get(x, SLOTH_IX) - l;
			sloth_set(x, SLOTH_IX, sloth_get(x, SLOTH_IX) + d);
			/* printf("LX == 0 l %ld o %ld d %ld\n", l, o, d); */
		}
	}

	if (!(do_first_loop == 0 && o == 0)) {
		while (((o ^ (o + d)) & (o ^ d)) >= 0 && sloth_get(x, SLOTH_LX) == 0) {
			sloth_eval(x, q);
			if (sloth_get(x, SLOTH_LX) == 0) { /* Avoid pop if we're leaving */
				d = sloth_pop(x);
				o = sloth_get(x, SLOTH_IX) - l;
				sloth_set(x, SLOTH_IX, sloth_get(x, SLOTH_IX) + d);
			}
		}
	}

	if (sloth_get(x, SLOTH_LX) == 0 || sloth_get(x, SLOTH_LX) == 1) { 
		/* Leave case */
		sloth__ipop(x);
	} else if (sloth_get(x, SLOTH_LX) < 0) {
		/* Unloop case */
		sloth_set(x, SLOTH_LX, sloth_get(x, SLOTH_LX) + 1);
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
	/* Obsolescent word set queries */
	case 24: /* FLOATING */
		#ifndef SLOTH_NO_FLOATING_POINT
		sloth_push(x, -1);
		#else
		sloth_push(x, 0);
		#endif
	case 25: /* FLOATING-EXT */
		/* TODO */
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
	CELL ibuf = sloth_get(x, SLOTH_IBUF);
	CELL ilen = sloth_get(x, SLOTH_ILEN);
	CELL ipos = sloth_get(x, SLOTH_IPOS);
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
	sloth_cstore(x, sloth_to_abs(x, sloth_here(x) + SLOTH_CBUF), end - start);
	for (i = 0; i < (end - start); i++) {
		sloth_cstore(x, sloth_to_abs(x, sloth_here(x) + SLOTH_CBUF + suCHAR + i*suCHAR), sloth_cfetch(x, start + i*suCHAR));
	}
	sloth_push(x, sloth_to_abs(x, sloth_here(x) + SLOTH_CBUF));
	/* If we are not at the end of the input buffer, */
	/* skip c after the word, but its not part of the counted */
	/* string */
	if (ipos < ilen) ipos++;
	sloth_set(x, SLOTH_IPOS, ipos);
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
	for (i = 0; i < sloth_get(x, SLOTH_ORDER); i++) {
		wl = sloth_get(x, SLOTH_CONTEXT + i*sCELL);
		w = sloth_fetch(x, wl);
		while (w != 0) {
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
	char buf[15]; char *endptr;
	while (sloth_get(x, SLOTH_IPOS) < sloth_get(x, SLOTH_ILEN)) {
		sloth_push(x, 32); sloth_word_(x);
		tok = (char*)(sloth_pick(x, 0) + suCHAR);
		tlen = sloth_cfetch(x, sloth_pick(x, 0));
		if (tlen == 0) { sloth_pop(x); return; }
		sloth_find_(x);
		if ((flag = sloth_pop(x)) != 0) {
			if (sloth_get(x, SLOTH_STATE) == 0
			|| (sloth_get(x, SLOTH_STATE) != 0 && flag == 1)) {
				sloth_eval(x, sloth_pop(x));	
			} else {
				sloth_compile(x, sloth_pop(x));
			}
		} else {
			CELL temp_base = sloth_get(x, SLOTH_BASE);
			sloth_pop(x);
			if (tlen == 3 && *tok == '\'' && (*(tok + 2)) == '\'') {
				/* Character literal */
				if (sloth_get(x, SLOTH_STATE) == 0)	
					sloth_push(x, *(tok + 1));
				else 
					sloth_literal(x, *(tok + 1));
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
					if (sloth_get(x, SLOTH_STATE) == 0) sloth_push(x, n);
					else sloth_literal(x, n);
				} else {
					#ifndef SLOTH_NO_FLOATING_POINT
					if (sloth_get(x, SLOTH_BASE) == 10) {
						sloth_push(x, (CELL)tok);
						sloth_push(x, (CELL)tlen);
						sloth_to_float_(x);
						if (sloth_pop(x) == 0) {
							/* TODO Word not found, throw an exception? */
							/* printf("%.*s ?\n", tlen, tok); */
							sloth_throw(x, -13);
						} else {
							if (sloth_get(x, SLOTH_STATE) != 0) {
								sloth_fliteral(x, sloth_fpop(x));
							}
						}
					}
					#else
					/* TODO Word not found, throw an exception? */
					printf("%.*s ?\n", tlen, tok);
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
void sloth_unused_(X* x) { 
	sloth_push(x, x->sz - sloth_get(x, SLOTH_HERE)); 
}

/* Source code preprocessing, interpreting & auditing commands */

void sloth_included_(X* x) {
	FILE *f;
	char filename[1024];
	char linebuf[1024];
	CELL INTERPRET, e;

	CELL previbuf = sloth_get(x, SLOTH_IBUF);
	CELL previpos = sloth_get(x, SLOTH_IPOS);
	CELL previlen = sloth_get(x, SLOTH_ILEN);

	CELL prevsourceid = sloth_get(x, SLOTH_SOURCE_ID);

	CELL l = sloth_pop(x);
	CELL a = sloth_pop(x);

	strncpy(filename, (char*)a, (size_t)l);
	filename[l] = 0;

	f = fopen(filename, "r");

	if (f) {
		INTERPRET = sloth_get_xt(x, sloth_find_word(x, "INTERPRET"));

		sloth_set(x, SLOTH_SOURCE_ID, (CELL)f);

		while (fgets(linebuf, 1024, f)) {
			/* printf(">>>> %s\n", linebuf); */
			/* I tried to use _refill from here as the next */
			/* lines of code do exactly the same but, the */
			/* input buffer of the included file is overwritten */
			/* when doing some REFILL from Forth (for an [IF] */
			/* for example). So I left this here to be able to */
			/* use linebuf here. */
			sloth_set(x, SLOTH_IBUF, (CELL)linebuf);
			sloth_set(x, SLOTH_IPOS, 0);
			if (linebuf[strlen(linebuf) - 1] < ' ') {
				sloth_set(x, SLOTH_ILEN, strlen(linebuf) - 1);
			} else {
				sloth_set(x, SLOTH_ILEN, strlen(linebuf));
			}

			sloth_eval(x, INTERPRET);
		}

		sloth_set(x, SLOTH_SOURCE_ID, prevsourceid);

		fclose(f);
	} else {
		printf("ERROR: Can't open file (%.*s)\n", (int)l, (char*)a);
	}

	sloth_set(x, SLOTH_IBUF, previbuf);
	sloth_set(x, SLOTH_IPOS, previpos);
	sloth_set(x, SLOTH_ILEN, previlen);
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

void sloth_s_m_slash_rem_(X* x) {
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
void sloth_fetch_(X* x) { sloth_push(x, sloth_fetch(x, sloth_pop(x))); }
void sloth_store_(X* x) { CELL a = sloth_pop(x); sloth_store(x, a, sloth_pop(x)); }

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
	sloth_set(x, SLOTH_LATESTXT, sloth_get_xt(x, sloth_get_latest(x)));
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_HIDDEN);
	sloth_set(x, SLOTH_STATE, 1);
}
void sloth_colon_no_name_(X* x) { 
	sloth_push(x, sloth_here(x));
	sloth_set(x, SLOTH_LATESTXT, sloth_here(x));
	sloth_set(x, SLOTH_STATE, 1);
}
void sloth_semicolon_(X* x) {
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_set(x, SLOTH_STATE, 0);
	/* Don't change flags for nonames */
	if (sloth_get_xt(x, sloth_get_latest(x)) == sloth_get(x, SLOTH_LATESTXT))
		sloth_unset_flag(x, sloth_get_latest(x), SLOTH_HIDDEN);
}

void sloth_recurse_(X* x) { sloth_compile(x, sloth_get(x, SLOTH_LATESTXT)); }
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
	sloth_set(x, sloth_get_xt(x, sloth_get_latest(x)) + 2*sCELL, sloth_pop(x));
}
void sloth_does_(X* x) {
	sloth_literal(x, sloth_here(x) + 4*sCELL);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(DOES)")));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
}
void sloth_evaluate_(X* x) {
	CELL l = sloth_pop(x), a = sloth_pop(x);

	CELL previbuf = sloth_get(x, SLOTH_IBUF);
	CELL previpos = sloth_get(x, SLOTH_IPOS);
	CELL previlen = sloth_get(x, SLOTH_ILEN);

	CELL prevsourceid = sloth_get(x, SLOTH_SOURCE_ID);

	sloth_set(x, SLOTH_SOURCE_ID, -1);

	sloth_set(x, SLOTH_IBUF, a);
	sloth_set(x, SLOTH_IPOS, 0);
	sloth_set(x, SLOTH_ILEN, l);

	sloth_interpret_(x);

	sloth_set(x, SLOTH_SOURCE_ID, prevsourceid);

	sloth_set(x, SLOTH_IBUF, previbuf);
	sloth_set(x, SLOTH_IPOS, previpos);
	sloth_set(x, SLOTH_ILEN, previlen);
}
void sloth_execute_(X* x) { sloth_eval(x, sloth_pop(x)); }
void sloth_here_(X* x) { sloth_push(x, sloth_to_abs(x, sloth_here(x))); }
void sloth_immediate_(X* x) { 
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_IMMEDIATE); 
}
void sloth_to_in_(X* x) { 
	sloth_push(x, sloth_to_abs(x, SLOTH_IPOS)); 
}
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
	switch (sloth_get(x, SLOTH_SOURCE_ID)) {
	case -1: 
		sloth_push(x, 0);
		break;
	case 0:
		sloth_push(x, sloth_get(x, SLOTH_IBUF)); sloth_push(x, 80);
		sloth_eval(x, sloth_get_xt(x, sloth_find_word(x, "ACCEPT")));
		sloth_set(x, SLOTH_ILEN, sloth_pop(x));
		sloth_set(x, SLOTH_IPOS, 0);
		sloth_push(x, -1); 
		break;
	default: 
		if (fgets(linebuf, 1024, (FILE *)sloth_get(x, SLOTH_SOURCE_ID))) {
			sloth_set(x, SLOTH_IBUF, (CELL)linebuf);
			sloth_set(x, SLOTH_IPOS, 0);
			/* Although I haven't found anywhere that \n should */
			/* not be part of the input buffer when reading from */
			/* a file, the results from preliminary tests when */
			/* using SOURCE ... TYPE add newlines (because they */
			/* are present) and on some other Forths they do not. */
			/* So I just added a check to remove the \n at then */
			/* end. */
			if (linebuf[strlen(linebuf) - 1] < ' ') {
				sloth_set(x, SLOTH_ILEN, strlen(linebuf) - 1);
			} else {
				sloth_set(x, SLOTH_ILEN, strlen(linebuf));
			}
			sloth_push(x, -1);
		} else {
			sloth_push(x, 0);
		}
		break;
	}	
}

void sloth_source_(X* x) { 
	sloth_push(x, sloth_get(x, SLOTH_IBUF)); 
	sloth_push(x, sloth_get(x, SLOTH_ILEN)); 
}

#ifndef SLOTH_NO_FLOATING_POINT

/* == Floating point word set ========================== */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x) { 
	sloth_set(
		x, 
		SLOTH_HERE, 
		ALIGNED(sloth_get(x, SLOTH_HERE), sFLOAT));
}
void sloth_f_aligned_(X* x) { 
	sloth_push(x, ALIGNED(sloth_pop(x), sFLOAT)); 
}
void sloth_f_literal_(X* x) { /* TODO */ }
void sloth_floats_(X* x) { sloth_push(x, sloth_pop(x) * sFLOAT); }
void sloth_float_plus_(X* x) { /* TODO */ }

void sloth_s_f_aligned_(X* x) { 
	sloth_push(x, ALIGNED(sloth_pop(x), sSFLOAT)); 
}
void sloth_d_f_aligned_(X* x) {
	sloth_push(x, ALIGNED(sloth_pop(x), sDFLOAT)); 
}

void sloth_s_floats_(X* x) { sloth_push(x, sloth_pop(x) * sSFLOAT); }
void sloth_d_floats_(X* x) { sloth_push(x, sloth_pop(x) * sDFLOAT); }

/* Manipulating stack items */

void sloth_f_depth_(X* x) { sloth_push(x, x->fp); }
void sloth_f_drop_(X* x) { sloth_fpop(x); }
void sloth_f_dup_(X* x) { sloth_fpush(x, sloth_fpick(x, 0)); }
void sloth_f_over_(X* x) { sloth_fpush(x, sloth_fpick(x, 1)); }
void sloth_f_rot_(X* x) { 
	FLOAT c = sloth_fpop(x);
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_fpush(x, b);
	sloth_fpush(x, c);
	sloth_fpush(x, a);
}
void sloth_f_swap_(X* x) { 
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_fpush(x, b);
	sloth_fpush(x, a);
}

/* Comparison operations */

void sloth_f_less_than_(X* x) { 
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_push(x, a < b ? -1 : 0);
}
void sloth_f_zero_less_than_(X* x) { 
	sloth_push(x, sloth_fpop(x) < 0.0 ? -1 : 0); 
}
void sloth_f_zero_equals_(X* x) {
	sloth_push(x, sloth_fpop(x) == 0.0 ? -1 : 0);
}

/* Memory-stack transfer operations */

void sloth_f_fetch_(X* x) { 
	sloth_fpush(x, sloth_ffetch(x, sloth_pop(x))); 
}
void sloth_f_store_(X* x) { 
	sloth_fstore(x, sloth_pop(x), sloth_fpop(x)); 
}

void sloth_s_f_fetch_(X* x) { 
	sloth_fpush(x, sloth_sffetch(x, sloth_pop(x))); 
}
void sloth_s_f_store_(X* x) { 
	sloth_sfstore(x, sloth_pop(x), sloth_fpop(x)); 
}

void sloth_d_f_fetch_(X* x) { 
	sloth_fpush(x, sloth_dffetch(x, sloth_pop(x))); 
}
void sloth_d_f_store_(X* x) { 
	sloth_dfstore(x, sloth_pop(x), sloth_fpop(x)); 
}

/* Commands to define data structures */

void sloth_f_constant_(X* x) { /* TODO */}
void sloth_f_variable_(X* x) { /* TODO */}

/* Number-type conversion operators */

void sloth_d_to_f_(X* x) { /* TODO */}
void sloth_f_to_d_(X* x) { /* TODO */}
void sloth_s_to_f_(X* x) { /* TODO */}

/* Arithmetic and logical operations */

void sloth_f_plus_(X* x) { /* TODO */}
void sloth_f_minus_(X* x) { /* TODO */}
void sloth_f_star_(X* x) { /* TODO */}
void sloth_f_slash_(X* x) { /* TODO */}
void sloth_floor_(X* x) { /* TODO */}
void sloth_f_max_(X* x) { /* TODO */}
void sloth_f_min_(X* x) { /* TODO */}
void sloth_f_negate_(X* x) { /* TODO */}
void sloth_f_round_(X* x) { /* TODO */}

/* String/numeric conversion */

void sloth_to_float_(X* x) {
	char buf[15]; char *endptr;
	int tlen = (int)sloth_pop(x);
	char* tok = (char*)sloth_pop(x);
	double n;
	strncpy(buf, tok, tlen);
	buf[tlen] = 0;
	n = strtod(buf, &endptr);
	if (n == 0 && endptr == buf) {
		sloth_push(x, 0);
	} else {
		sloth_fpush(x, (float)n);
		sloth_push(x, -1);
	}
}

void sloth_represent_(X* x) { /* TODO */}

/* Non ANS floating point helpers */

void sloth_f_dot_s_(X* x) {
	int i;
	printf("F:<%ld> ", x->fp);
	for (i = 0; i < x->fp; i++) printf("%f ", x->f[i]);
}

#endif

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

/* Helper to work with absolute/relative memory addresses */

void sloth_to_abs_(X* x) { sloth_push(x, sloth_pop(x) + x->d); }
void sloth_to_rel_(X* x) { sloth_push(x, sloth_pop(x) - x->d); }

/* Helper to empty the return stack */

void sloth_empty_rs_(X* x) { x->rp = 0; }

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap(X* x) {

	/* Variables commonly shared from C and Forth */

	/* comma can not be used for first variable because it */
	/* needs sloth_here(x) and that function gets the value from */
	/* the first variable. */

	*((CELL*)x->d) = sCELL; /* HERE */

	sloth_comma(x, 10); /* BASE */
	sloth_comma(x, 0); /* FORTH-WORDLIST */
	sloth_comma(x, 0); /* INTERNAL-WORDLIST */
	sloth_comma(x, 0); /* STATE */
	sloth_comma(x, 0); /* IBUF */
	sloth_comma(x, 0); /* IPOS */
	sloth_comma(x, 0); /* ILEN */
	sloth_comma(x, 0); /* SOURCE_ID */
	sloth_comma(x, 0); /* HLD */
	sloth_comma(x, 0); /* LATESTXT */
	sloth_comma(x, 0); /* IX */
	sloth_comma(x, 0); /* JX */
	sloth_comma(x, 0); /* KX */
	sloth_comma(x, 0); /* LX */
	sloth_comma(x, sloth_to_abs(x, SLOTH_FORTH_WORDLIST)); /* CURRENT */
	sloth_comma(x, 2); /* #ORDER */
	sloth_comma(x, sloth_to_abs(x, SLOTH_FORTH_WORDLIST)); /* CONTEXT 0 */
	sloth_comma(x, sloth_to_abs(x, SLOTH_INTERNAL_WORDLIST)); /* CONTEXT 1 */
	sloth_allot(x, 14*sCELL);
	/* Variable indicating if we are on Linux or Windows */
	/*
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	sloth_comma(x, 1);
#else
	sloth_comma(x, 0);
#endif
*/

	/* Basic primitives */

	sloth_code(x, "EXIT", sloth_primitive(x, &sloth_exit_));

	sloth_set(x, SLOTH_CURRENT, sloth_to_abs(x, SLOTH_INTERNAL_WORDLIST));

	sloth_code(x, "(LIT)", sloth_primitive(x, &sloth_lit_));
	#ifndef SLOTH_NO_FLOATING_POINT
	sloth_code(x, "(FLIT)", sloth_primitive(x, &sloth_flit_));
	#endif
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

	sloth_set(x, SLOTH_CURRENT, sloth_to_abs(x, SLOTH_FORTH_WORDLIST));

	/* Quotations */

	sloth_code(x, "[:", sloth_primitive(x, &sloth_start_quotation_)); sloth_immediate_(x);
	sloth_code(x, ";]", sloth_primitive(x, &sloth_end_quotation_)); sloth_immediate_(x);

	/* Commands that can help you start or end work sessions */

	sloth_code(x, "UNUSED", sloth_primitive(x, &sloth_unused_));
	sloth_code(x, "BYE", sloth_primitive(x, &sloth_bye_));

	/* Commands to inspect memory, debug & view code */

	sloth_code(x, "DEPTH", sloth_primitive(x, &sloth_depth_));

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
	sloth_code(x, "SM/REM", sloth_primitive(x, &sloth_s_m_slash_rem_));
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
	sloth_code(x, ">IN", sloth_primitive(x, &sloth_to_in_));
	sloth_code(x, "POSTPONE", sloth_primitive(x, &sloth_postpone_)); sloth_immediate_(x);
	sloth_code(x, "REFILL", sloth_primitive(x, &sloth_refill_));
	sloth_code(x, "SOURCE", sloth_primitive(x, &sloth_source_));
	sloth_code(x, "WORD", sloth_primitive(x, &sloth_word_));

	sloth_code(x, "FIND", sloth_primitive(x, &sloth_find_));

	#ifndef SLOTH_NO_FLOATING_POINT

	/* == Floating point word set ======================== */

	/* Constructing compiler and interpreter system extensions */

	sloth_code(x, "FALIGN", sloth_primitive(x, &sloth_f_align_));
	sloth_code(x, "FALIGNED", sloth_primitive(x, &sloth_f_aligned_));
	sloth_code(x, "FLITERAL", sloth_primitive(x, &sloth_f_literal_));
	sloth_code(x, "FLOATS", sloth_primitive(x, &sloth_floats_));
	sloth_code(x, "FLOAT+", sloth_primitive(x, &sloth_float_plus_));

	sloth_code(x, "SFALIGNED", sloth_primitive(x, &sloth_s_f_aligned_));
	sloth_code(x, "DFALIGNED", sloth_primitive(x, &sloth_d_f_aligned_));

	sloth_code(x, "SFLOATS", sloth_primitive(x, &sloth_s_floats_));
	sloth_code(x, "DFLOATS", sloth_primitive(x, &sloth_d_floats_));

	/* Manipulating stack items */

	sloth_code(x, "FDEPTH", sloth_primitive(x, &sloth_f_depth_));
	sloth_code(x, "FDROP", sloth_primitive(x, &sloth_f_drop_));
	sloth_code(x, "FDUP", sloth_primitive(x, &sloth_f_dup_));
	sloth_code(x, "FOVER", sloth_primitive(x, &sloth_f_over_));
	sloth_code(x, "FROT", sloth_primitive(x, &sloth_f_rot_));
	sloth_code(x, "FSWAP", sloth_primitive(x, &sloth_f_swap_));

	/* Comparison operations */

	sloth_code(x, "F<", sloth_primitive(x, &sloth_f_less_than_));
	sloth_code(x, "F0<", sloth_primitive(x, &sloth_f_zero_less_than_));
	sloth_code(x, "F0=", sloth_primitive(x, &sloth_f_zero_equals_));

	/* Memory-stack transfer operations */

	sloth_code(x, "F@", sloth_primitive(x, &sloth_f_fetch_));
	sloth_code(x, "F!", sloth_primitive(x, &sloth_f_store_));

	sloth_code(x, "SF@", sloth_primitive(x, &sloth_f_fetch_));
	sloth_code(x, "SF!", sloth_primitive(x, &sloth_f_store_));

	sloth_code(x, "DF@", sloth_primitive(x, &sloth_d_f_fetch_));
	sloth_code(x, "DF!", sloth_primitive(x, &sloth_d_f_store_));

	/* Non ANS floating point helpers */

	sloth_code(x, "F.S", sloth_primitive(x, &sloth_f_dot_s_));

	#endif

	/* == Helpers ======================================== */

	sloth_code(x, "TO-ABS", sloth_primitive(x, &sloth_to_abs_));
	sloth_code(x, "TO-REL", sloth_primitive(x, &sloth_to_rel_));
	/* TODO Maybe INTERPRET should be deferred or as a */
	/* variable to be able to create an INTERPRET for */
	/* debugging in Forth itself. */
	sloth_code(x, "INTERPRET", sloth_primitive(x, &sloth_interpret_));
	sloth_code(x, "(EMPTY-RETURN-STACK)", sloth_primitive(x, &sloth_empty_rs_));
}

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
	char buf[80];
	sloth_set(x, SLOTH_IBUF, (CELL)buf);
	sloth_set(x, SLOTH_IPOS, 0);
	sloth_set(x, SLOTH_ILEN, 80);
	sloth_eval(x, sloth_get_xt(x, sloth_find_word(x, "QUIT")));
}

#endif
#endif
