#ifndef SLOTH_HEADER
#define SLOTH_HEADER

#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<assert.h>

#include "kernel.h"

/* ----------------------------------------------------- */
/* ---------------- Virtual machine -------------------- */
/* ----------------------------------------------------- */
/* This is the 2nd reference implementation of the SLOTH */
/* Virtual Machine.                                      */
/* ----------------------------------------------------- */
/* This API defines how the virtual machine works and    */
/* allows access to its internals from the host.         */
/* ----------------------------------------------------- */
/* It uses a table of primitives (C functions that can   */
/* be called from Forth) that the bootstrapped           */
/* programming language can use to interact with the     */
/* virtual machine.                                      */
/* ----------------------------------------------------- */

/* -- Virtual Machine constants ------------------------ */

typedef uint8_t uCHAR; /* CHARs are always unsigned */
typedef intptr_t CELL;
typedef uintptr_t uCELL;

#define suCHAR sizeof(uCHAR)

#define ALIGNED(a, t) (((a) + ((t) - 1)) & ~((t) - 1))

#if UINTPTR_MAX == UINT64_MAX
	#define sCELL 8 
	#define CELL_BITS 64
	#define hCELL_MASK 0xFFFFFFFF
	#define hCELL_BITS 32
#endif
#if UINTPTR_MAX == UINT32_MAX
	#define sCELL 4
	#define CELL_BITS 32
	#define hCELL_MASK 0xFFFF
	#define hCELL_BITS 16 
#endif
#if UINTPTR_MAX == UINT16_MAX
	#define sCELL 2
	#define CELL_BITS 16
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

/* -- Virtual Machine context definition --------------- */

struct sloth_VM;
typedef void (*F)(struct sloth_VM*);

typedef struct sloth_PRIMITIVES {
	CELL pz;
	CELL last;
	F *p;
} sloth_P;

typedef struct sloth_VM { 
	CELL d, dz;	/* Dict base address, dict size */
	CELL u, uz; /* User area base address and size */
	CELL ip;

	CELL s[SLOTH_STACK_SIZE], sp;
	CELL r[SLOTH_RETURN_STACK_SIZE], rp;

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	sloth_P *p;
} X;

#ifdef SLOTH_IMPLEMENTATION
#define PUBLIC(ret, name, params, ...) ret sloth_##name params { __VA_ARGS__ }
#define PRIVATE(ret, name, params, ...) static ret sloth_##name params { __VA_ARGS__ }
#else
#define PUBLIC(ret, name, params, ...) ret sloth_##name params;
#define PRIVATE(ret, name, params, ...) 
#endif

/* -- Context initialization and destruction ----------- */

PRIVATE(void, init, (X* x, CELL d, CELL dz, CELL u, CELL uz), { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->dz = dz;
	x->u = u;
	x->uz = uz;

	x->jmpbuf_idx = -1;
})

PUBLIC(X*, create, (int psize, int dsize, int usize), {
	X* x;

	x = malloc(sizeof(X));
	x->p = malloc(sizeof(sloth_P));
	x->p->p = malloc(sizeof(F) * psize);
	x->p->last = 0;
	x->p->pz = psize;
	x->d = (CELL)malloc(dsize);
	x->u = (CELL)malloc(usize);

	sloth_init(x, x->d, dsize, x->u, usize);

	return x;
})

PUBLIC(X*, new, (), { return sloth_create(512, 524288, 1024); })

PUBLIC(void, free, (X* x), {
	free((void*)x->d);
	free(x->p->p);
	free(x->p);
	free(x);
})

/* -- Data and return stack ---------------------------- */

PUBLIC(void, push, (X* x, CELL v), { x->s[x->sp] = v; x->sp++; })
PUBLIC(CELL, pop, (X* x), { x->sp--; return x->s[x->sp]; })
PUBLIC(void, rpush, (X* x, CELL v), { x->r[x->rp] = v; x->rp++; })
PUBLIC(CELL, rpop, (X* x), { x->rp--; return x->r[x->rp]; })

/* -- Memory management -------------------------------- */

#define SLOTH_HERE x->d

/* Transform from relative to absolute addresses. */
PUBLIC(CELL, to_abs, (X* x, CELL a), { return (CELL)(x->d + a); })
PUBLIC(CELL, to_rel, (X* x, CELL a), { return a - x->d; })

PUBLIC(void, cstore, (X* x, CELL a, uCHAR v), { *((uCHAR*)a) = v; })
PUBLIC(uCHAR, cfetch, (X* x, CELL a), { return *((uCHAR*)a); })
PUBLIC(void, store, (X* x, CELL a, CELL v), { *((CELL*)a) = v; })
PUBLIC(CELL, fetch, (X* x, CELL a), { return *((CELL*)a); })

/* Setting and getting variables (cell and char sized) */

PUBLIC(void, set, (X* x, CELL a, CELL v), {	sloth_store(x, sloth_to_abs(x, a), v); })
PUBLIC(CELL, get, (X* x, CELL a), { return sloth_fetch(x, sloth_to_abs(x, a)); })

PUBLIC(void, user_set, (X* x, CELL a, CELL v), { sloth_store(x, x->u + a, v); })
PUBLIC(CELL, user_get, (X* x, CELL a), { return sloth_fetch(x, x->u + a); })

PUBLIC(CELL, here, (X* x), { sloth_push(x, sloth_get(x, SLOTH_HERE)); })
PUBLIC(void, allot, (X* x, CELL v), { sloth_set(x, SLOTH_HERE, sloth_here(x) + v); })
PUBLIC(CELL, aligned, (CELL a), { return ALIGNED(a, sCELL); })
PUBLIC(void, align, (X* x), { sloth_set(x, SLOTH_HERE, ALIGNED(sloth_here(x), sCELL)); })

/* -- Compilation -------------------------------------- */

PUBLIC(void, comma, (X* x, CELL v), { 
	sloth_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + sCELL);
})
PUBLIC(void, ccomma, (X* x, uCHAR v), { 
	sloth_cstore(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + suCHAR);
})

/* -- Headers ------------------------------------------ */

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

PUBLIC(CELL, header, (X* x, CELL n, CELL l), {
	CELL w, i;
	sloth_align(x);
	w = sloth_here(x); /* NT address */
	/* Latest is stored at x->d + sCELL */
	sloth_comma(x, sloth_fetch(x, x->d + sCELL)); /* Compile latest */
	sloth_store(x, x->d + sCELL, w); /* Set new latest to this word */
	sloth_comma(x, 0); /* Reserve space for XT */
	sloth_ccomma(x, 0); /* Flags (default flags: 0) */
	sloth_ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) sloth_ccomma(x, sloth_cfetch(x, n + i)); /* Name */
	sloth_align(x); /* Align XT address */
	sloth_store(x, w + sCELL, sloth_here(x));
	return w;
})

PUBLIC(void, set_xt, (X* x, CELL w, CELL xt), { 
	sloth_store(x, w + sCELL, xt); 
})

/* -- Primitive and word creation ---------------------- */

PUBLIC(CELL, primitive, (X* x, F f), { 
	assert(x->p->last < x->p->pz);
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
})

PUBLIC(CELL, code, (X* x, char* name, CELL xt), {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, xt);
	return xt; 
})

/* -- Basic primitives --------------------------------- */

PUBLIC(void, context_, (X* x), { sloth_push(x, (CELL)x); })

PUBLIC(void, drop_, (X* x), { sloth_pop(x); })
PUBLIC(void, dup_, (X* x), { CELL a = sloth_pop(x); sloth_push(x, a); sloth_push(x, a); })
PUBLIC(void, over_, (X* x), { CELL b = sloth_pop(x); CELL a = sloth_pop(x); sloth_push(x, a); sloth_push(x, b); sloth_push(x, a); })
PUBLIC(void, to_r_, (X* x), { CELL a = sloth_pop(x); sloth_rpush(x, a); })
PUBLIC(void, r_from_, (X* x), { CELL a = sloth_rpop(x); sloth_push(x, a); })
PUBLIC(void, swap_, (X* x), { CELL b = sloth_pop(x); CELL a = sloth_pop(x); sloth_push(x, b); sloth_push(x, a); })

/* -- Memory-stack transfer operations */

PUBLIC(void, c_fetch_, (X* x), { sloth_push(x, sloth_cfetch(x, sloth_pop(x))); })
PUBLIC(void, c_store_, (X* x), { CELL a = sloth_pop(x); sloth_cstore(x, a, sloth_pop(x)); })
PUBLIC(void, fetch_, (X* x), { sloth_push(x, sloth_fetch(x, sloth_pop(x))); })
PUBLIC(void, store_, (X* x), { CELL a = sloth_pop(x); sloth_store(x, a, sloth_pop(x)); })

PUBLIC(void, cells_, (X* x), { sloth_push(x, sloth_pop(x)*sCELL); })
PUBLIC(void, chars_, (X* x), { /* Does nothing */ })

/* -- Bootstrapping ------------------------------------ */

PUBLIC(void, bootstrap, (X* x), {
	/* Initializes the required primitives to bootstrap from */
	/* Forth code. */
	sloth_code(x, "CONTEXT", sloth_primitive(x, &sloth_context_));

	sloth_code(x, "DROP", sloth_primitive(x, &sloth_drop_));
	sloth_code(x, "DUP", sloth_primitive(x, &sloth_dup_));
	sloth_code(x, "OVER", sloth_primitive(x, &sloth_over_));
	sloth_code(x, ">R", sloth_primitive(x, &sloth_to_r_));
	sloth_code(x, "R>", sloth_primitive(x, &sloth_r_from_));
	sloth_code(x, "SWAP", sloth_primitive(x, &sloth_swap_));

	sloth_code(x, "C@", sloth_primitive(x, &sloth_c_fetch_));
	sloth_code(x, "C!", sloth_primitive(x, &sloth_c_store_));
	sloth_code(x, "@", sloth_primitive(x, &sloth_fetch_));
	sloth_code(x, "!", sloth_primitive(x, &sloth_store_));

	sloth_code(x, "CELLS", sloth_primitive(x, &sloth_cells_));
	sloth_code(x, "CHARS", sloth_primitive(x, &sloth_chars_));
/*

	sloth_code(x, "AND", sloth_primitive(x, &sloth_and_));
	sloth_code(x, "INVERT", sloth_primitive(x, &sloth_invert_));
	sloth_code(x, "LSHIFT", sloth_primitive(x, &sloth_l_shift_));
	sloth_code(x, "-", sloth_primitive(x, &sloth_minus_));
	sloth_code(x, "+", sloth_primitive(x, &sloth_plus_));
	sloth_code(x, "RSHIFT", sloth_primitive(x, &sloth_r_shift_));
	sloth_code(x, "*", sloth_primitive(x, &sloth_star_));
	sloth_code(x, "2/", sloth_primitive(x, &sloth_two_slash_));
	sloth_code(x, "UM*", sloth_primitive(x, &sloth_u_m_star_));
	sloth_code(x, "UM/MOD", sloth_primitive(x, &sloth_u_m_slash_mod_));

	sloth_code(x, "=", sloth_primitive(x, &sloth_equals_));
	sloth_code(x, "<", sloth_primitive(x, &sloth_less_than_));

	sloth_code(x, "?:", sloth_primitive(x, &sloth_conditional_colon_));
	sloth_code(x, "?\\", sloth_primitive(x, &sloth_conditional_comment_));
	sloth_code(x, ";", sloth_primitive(x, &sloth_semicolon_));

	sloth_code(x, "CATCH", sloth_primitive(x, &sloth_catch_));
	sloth_code(x, "THROW", sloth_primitive(x, &sloth_throw_));

	sloth_code(x, "ALLOT", sloth_primitive(x, &sloth_allot_));
*/
})

#endif
