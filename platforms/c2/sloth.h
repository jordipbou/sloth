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

/* Macros for defining internal, public api and primitives. */

#ifdef SLOTH_IMPLEMENTATION
#define I(ret, name, params, ...) static ret sloth_##name params { __VA_ARGS__ }
#define A(ret, name, params, ...) ret sloth_##name params { __VA_ARGS__ }
#define P(name, ...) void sloth_##name##_(X* x) { __VA_ARGS__ }
#else
#define I(ret, name, params, ...) 
#define A(ret, name, params, ...) ret sloth_##name params;
#define P(name, ...) void sloth_##name##_(X* x);
#endif

/* -- Context initialization and destruction ----------- */

I(void, init, (X* x, CELL d, CELL dz, CELL u, CELL uz), { 
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

A(X*, create, (int psize, int dsize, int usize), {
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

A(X*, new, (), { return sloth_create(512, 524288, 1024); })

A(void, free, (X* x), {
	free((void*)x->d);
	free(x->p->p);
	free(x->p);
	free(x);
})

/* -- Data and return stack ---------------------------- */

A(void, push, (X* x, CELL v), { x->s[x->sp] = v; x->sp++; })
A(CELL, pop, (X* x), { x->sp--; return x->s[x->sp]; })
A(void, rpush, (X* x, CELL v), { x->r[x->rp] = v; x->rp++; })
A(CELL, rpop, (X* x), { x->rp--; return x->r[x->rp]; })

P(dup, { CELL a = sloth_pop(x); sloth_push(x, a); sloth_push(x, a); })
P(drop, { sloth_pop(x); })
P(over, { CELL b = sloth_pop(x); CELL a = sloth_pop(x); sloth_push(x, a); sloth_push(x, b); sloth_push(x, a); })
P(swap, { CELL b = sloth_pop(x); CELL a = sloth_pop(x); sloth_push(x, b); sloth_push(x, a); })
P(to_r, { CELL a = sloth_pop(x); sloth_rpush(x, a); })
P(r_from, { CELL a = sloth_rpop(x); sloth_push(x, a); })

/* -- Memory management -------------------------------- */

#define SLOTH_HERE 0*sCELL

/* Transform from relative to absolute addresses. */
A(CELL, to_abs, (X* x, CELL a), { return (CELL)(x->d + a); })
A(CELL, to_rel, (X* x, CELL a), { return a - x->d; })

/* STORE/FETCH/CSTORE/cfetch work on absolute address units, */
/* not just inside SLOTH dictionary (memory block). */
A(void, c_store, (X* x, CELL a, uCHAR v), { *((uCHAR*)a) = v; })
A(uCHAR, c_fetch, (X* x, CELL a), { return *((uCHAR*)a); })
A(void, store, (X* x, CELL a, CELL v), { *((CELL*)a) = v; })
A(CELL, fetch, (X* x, CELL a), { return *((CELL*)a); })

/* Setting and getting cells in memory */

A(void, set, (X* x, CELL a, CELL v), {	sloth_store(x, sloth_to_abs(x, a), v); })
A(CELL, get, (X* x, CELL a), { return sloth_fetch(x, sloth_to_abs(x, a)); })

A(void, user_set, (X* x, CELL a, CELL v), { sloth_store(x, x->u + a, v); })
A(CELL, user_get, (X* x, CELL a), { return sloth_fetch(x, x->u + a); })

/* Working with the HERE pointer */

A(CELL, here, (X* x), { return sloth_get(x, SLOTH_HERE); })
A(void, allot, (X* x, CELL v), { sloth_set(x, SLOTH_HERE, sloth_here(x) + v); })
A(CELL, aligned, (CELL a), { return ALIGNED(a, sCELL); })

P(align, { sloth_set(x, SLOTH_HERE, ALIGNED(sloth_here(x), sCELL)); })

/* Moving data from stack to dictionary and viceversa */

P(c_fetch, { sloth_push(x, sloth_c_fetch(x, sloth_pop(x))); })
P(c_store, { CELL a = sloth_pop(x); sloth_c_store(x, a, sloth_pop(x)); })
P(fetch, { sloth_push(x, sloth_fetch(x, sloth_pop(x))); })
P(store, { CELL a = sloth_pop(x); sloth_store(x, a, sloth_pop(x)); })

P(cells, { sloth_push(x, sloth_pop(x)*sCELL); })
P(chars, { /* Does nothing */ })

/* -- Compilation -------------------------------------- */

A(void, comma, (X* x, CELL v), { 
	sloth_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + sCELL);
})
A(void, c_comma, (X* x, uCHAR v), { 
	sloth_c_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + suCHAR);
})

/* -- Headers ------------------------------------------ */

#define SLOTH_CURRENT						0*sCELL

A(CELL, get_latest, (X* x), { 
	return sloth_fetch(x, sloth_user_get(x, SLOTH_CURRENT));
})
A(void, set_latest, (X* x, CELL w), { 
	sloth_store(x, sloth_user_get(x, SLOTH_CURRENT), w);
})

A(void, set_xt, (X* x, CELL w, CELL xt), { 
	sloth_store(x, w + sCELL, xt); 
})

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

A(CELL, header, (X* x, CELL n, CELL l), {
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

A(CELL, primitive, (X* x, F f), { 
	assert(x->p->last < x->p->pz);
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
})

A(CELL, code, (X* x, char* name, CELL xt), {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, xt);
	return xt; 
})

/* -- Inner interpreter -------------------------------- */

A(CELL, op, (X* x), {	
	CELL o = sloth_fetch(x, x->ip);	
	x->ip += sCELL;	
	return o; 
})

I(void, do_prim, (X* x, CELL p), { (x->p->p[-1 - p])(x); })

I(void, call, (X* x, CELL q), { 
	if (x->ip >= 0) sloth_rpush(x, x->ip); 
	x->ip = q; 
})

I(void, execute, (X* x, CELL q), { 
	if (q < 0) sloth_do_prim(x, q); 
	else sloth_call(x, q); 
})

I(void, inner, (X* x), { 
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) {
		sloth_execute(x, sloth_op(x));
	}
})

A(void, eval, (X* x, CELL q), { 
	sloth_execute(x, q); 
	if (q > 0) sloth_inner(x); 
})

/* -- Tracing inner interpreter ------------------------ */

I(void, debug, (X* x, CELL debug_xt), {
	sloth_push(x, x->ip);
	sloth_eval(x, debug_xt);
})

I(void, debug_inner, (X* x, CELL debug_xt), {
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) {
		sloth_debug(x, debug_xt);
		sloth_execute(x, sloth_op(x));
	}
})

P(debug, {
	CELL post_xt = sloth_pop(x); 
	CELL inner_xt = sloth_pop(x);
	CELL pre_xt = sloth_pop(x);
	CELL q = sloth_pop(x);
	sloth_debug(x, pre_xt);
	sloth_execute(x, q);
	if (q > 0) sloth_debug_inner(x, inner_xt);
	sloth_debug(x, post_xt);
})

/* -- Exceptions --------------------------------------- */

A(void, sloth_catch, (X* x, CELL q), {
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
})

A(void, sloth_throw, (X* x, CELL e), {
	if (x->jmpbuf_idx >= 0) {
		longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
	} else {
		CELL ibuf = *((CELL*)(x->u+20*sCELL));
		CELL ipos = *((CELL*)(x->u+21*sCELL));
		CELL ilen = *((CELL*)(x->u+22*sCELL));
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
})

/* -- Bootstrapping ------------------------------------ */

A(void, bootstrap, (X* x), {
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
