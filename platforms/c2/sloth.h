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
#define PRIVATE(ret, name, params, ...) static ret sloth_##name params { __VA_ARGS__ }
#define API(ret, name, params, ...) ret sloth_##name params { __VA_ARGS__ }
#define PRIM(name, ...) void sloth_##name##_(X* x) { __VA_ARGS__ }
#else
#define PRIVATE(ret, name, params, ...) 
#define API(ret, name, params, ...) ret sloth_##name params;
#define PRIM(name, ...) void sloth_##name##_(X* x);
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

	/* Initialize HERE */
	*((CELL*)(x->d + 0*sCELL)) = x->d + 3*sCELL;
	/* Initialize FORTH-WORDLIST (the default wordlist) */
	*((CELL*)(x->d + 1*sCELL)) = 0;
	/* Initialize INTERNAL-WORDLIST */
	*((CELL*)(x->d + 2*sCELL)) = 0;

	/* Initialize CURRENT to point to FORTH-WORDLIST */
	*((CELL*)(x->u + 0*sCELL)) = x->d + 1*sCELL;
})

API(X*, create, (int psize, int dsize, int usize), {
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

API(X*, new, (), { return sloth_create(512, 524288, 1024); })

API(void, free, (X* x), {
	free((void*)x->d);
	free(x->p->p);
	free(x->p);
	free(x);
})

/* -- Data and return stack ---------------------------- */

API(void, push, (X* x, CELL v), { x->s[x->sp] = v; x->sp++; })
API(CELL, pop, (X* x), { x->sp--; return x->s[x->sp]; })
API(void, rpush, (X* x, CELL v), { x->r[x->rp] = v; x->rp++; })
API(CELL, rpop, (X* x), { x->rp--; return x->r[x->rp]; })

PRIM(dup, { CELL a = sloth_pop(x); sloth_push(x, a); sloth_push(x, a); })
PRIM(drop, { sloth_pop(x); })
PRIM(over, { CELL b = sloth_pop(x); CELL a = sloth_pop(x); sloth_push(x, a); sloth_push(x, b); sloth_push(x, a); })
PRIM(swap, { CELL b = sloth_pop(x); CELL a = sloth_pop(x); sloth_push(x, b); sloth_push(x, a); })
PRIM(to_r, { CELL a = sloth_pop(x); sloth_rpush(x, a); })
PRIM(r_from, { CELL a = sloth_rpop(x); sloth_push(x, a); })

/* -- Memory management -------------------------------- */

#define SLOTH_HERE 0*sCELL

/* Transform from relative to absolute addresses. */
API(CELL, to_abs, (X* x, CELL a), { return (CELL)(x->d + a); })
API(CELL, to_rel, (X* x, CELL a), { return a - x->d; })

/* STORE/FETCH/CSTORE/cfetch work on absolute address units, */
/* not just inside SLOTH dictionary (memory block). */
API(void, c_store, (X* x, CELL a, uCHAR v), { *((uCHAR*)a) = v; })
API(uCHAR, c_fetch, (X* x, CELL a), { return *((uCHAR*)a); })
API(void, store, (X* x, CELL a, CELL v), { *((CELL*)a) = v; })
API(CELL, fetch, (X* x, CELL a), { return *((CELL*)a); })

/* Setting and getting cells in memory */

API(void, set, (X* x, CELL a, CELL v), {	sloth_store(x, sloth_to_abs(x, a), v); })
API(CELL, get, (X* x, CELL a), { return sloth_fetch(x, sloth_to_abs(x, a)); })

API(void, user_set, (X* x, CELL a, CELL v), { sloth_store(x, x->u + a, v); })
API(CELL, user_get, (X* x, CELL a), { return sloth_fetch(x, x->u + a); })

/* Working with the HERE pointer */

API(CELL, here, (X* x), { return sloth_get(x, SLOTH_HERE); })
API(void, allot, (X* x, CELL v), { sloth_set(x, SLOTH_HERE, sloth_here(x) + v); })
API(CELL, aligned, (CELL a), { return ALIGNED(a, sCELL); })

PRIM(align, { sloth_set(x, SLOTH_HERE, ALIGNED(sloth_here(x), sCELL)); })

/* Moving data from stack to dictionary and viceversa */

PRIM(c_fetch, { sloth_push(x, sloth_c_fetch(x, sloth_pop(x))); })
PRIM(c_store, { CELL a = sloth_pop(x); sloth_c_store(x, a, sloth_pop(x)); })
PRIM(fetch, { sloth_push(x, sloth_fetch(x, sloth_pop(x))); })
PRIM(store, { CELL a = sloth_pop(x); sloth_store(x, a, sloth_pop(x)); })

PRIM(cells, { sloth_push(x, sloth_pop(x)*sCELL); })
PRIM(chars, { /* Does nothing */ })

/* -- Compilation -------------------------------------- */

API(void, comma, (X* x, CELL v), { 
	sloth_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + sCELL);
})
API(void, c_comma, (X* x, uCHAR v), { 
	sloth_c_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + suCHAR);
})

/* -- Headers ------------------------------------------ */

#define SLOTH_CURRENT						0*sCELL

API(CELL, get_latest, (X* x), { 
	return sloth_fetch(x, sloth_user_get(x, SLOTH_CURRENT));
})
API(void, set_latest, (X* x, CELL w), { 
	sloth_store(x, sloth_user_get(x, SLOTH_CURRENT), w);
})

API(void, set_xt, (X* x, CELL w, CELL xt), { 
	sloth_store(x, w + sCELL, xt); 
})

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

API(CELL, header, (X* x, CELL n, CELL l), {
	CELL w, i;
	sloth_align_(x);
	w = sloth_here(x); /* NT address */
	sloth_comma(x, sloth_get_latest(x));
	sloth_set_latest(x, w);
	sloth_comma(x, 0); /* Reserve space for XT */
	sloth_c_comma(x, 0); /* Flags (default flags: 0) */
	sloth_c_comma(x, l); /* Name length */
	for (i = 0; i < l; i++) sloth_c_comma(x, sloth_c_fetch(x, n + i)); /* Name */
	sloth_align_(x); /* Align XT address */
	sloth_set_xt(x, w, sloth_here(x));
	return w;
})

/* -- Primitive and word creation ---------------------- */

API(CELL, primitive, (X* x, F f), { 
	assert(x->p->last < x->p->pz);
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
})

API(CELL, code, (X* x, char* name, CELL xt), {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, xt);
	return xt; 
})

/* -- Bootstrapping ------------------------------------ */

API(void, bootstrap, (X* x), {
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
