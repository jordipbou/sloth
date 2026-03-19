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

/* -- Exceptions --------------------------------------- */

#define SLOTH_STACK_OVERFLOW					-3
#define SLOTH_STACK_UNDERFLOW					-4
#define SLOTH_RETURN_STACK_OVERFLOW		-5
#define SLOTH_RETURN_STACK_UNDERFLOW	-6

/* -- Displacement of counted string buffer from here -- */

/* TODO CBuffer could be just another space of the normal */
/* strings circular buffer. */
#define SLOTH_CBUF							64

/* -- Dictionary variables ----------------------------- */

#define SLOTH_HERE							0	
#define SLOTH_INTERNAL_WL				1*sCELL
#define SLOTH_FORTH_WL					2*sCELL

/* -- User area variables and buffers ------------------ */

#define SLOTH_CURRENT						0*sCELL
#define SLOTH_ORDER							1*sCELL
#define SLOTH_LOCALS_WORDLIST		2*sCELL
#define SLOTH_CONTEXT						3*sCELL
/* There are 16 CELLS reserved to search order */
#define SLOTH_BASE							19*sCELL
#define SLOTH_STATE							20*sCELL
#define SLOTH_IBUF							21*sCELL
#define SLOTH_IPOS							22*sCELL
#define SLOTH_ILEN							23*sCELL
#define SLOTH_SOURCE_ID					24*sCELL
#define SLOTH_SOURCE_POS				25*sCELL
#define SLOTH_LATESTXT					26*sCELL
#define SLOTH_INTERPRET					27*sCELL

#define SLOTH_ROOT_PATH_LENGTH	28*sCELL
#define SLOTH_PATH_START				29*sCELL
#define SLOTH_PATH_END					30*sCELL
/* Continuous space to store path strings */
#define SLOTH_PATHS							31*sCELL

/* Space between SLOTH_PATHS and SLOTH_INCLUDED_FILES */
/* reserved to store paths. */

#define SLOTH_INCLUDED_FILES		95*sCELL

#define SLOTH_LAST_USER_VAR			96*sCELL

/* -- Flags for word status ---------------------------- */

#define SLOTH_HIDDEN					1
#define SLOTH_IMMEDIATE				2

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

/* -- Basic compilation -------------------------------- */

A(void, comma, (X* x, CELL v), { 
	sloth_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + sCELL);
})
A(void, c_comma, (X* x, uCHAR v), { 
	sloth_c_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + suCHAR);
})

/* -- Headers ------------------------------------------ */

A(CELL, get_latest, (X* x), { 
	return sloth_fetch(x, sloth_user_get(x, SLOTH_CURRENT));
})
A(void, set_latest, (X* x, CELL w), { 
	sloth_store(x, sloth_user_get(x, SLOTH_CURRENT), w);
})

A(CELL, get_link, (X* x, CELL w), {
	return sloth_fetch(x, w);
})

A(CELL, get_xt, (X* x, CELL w), {
	return sloth_fetch(x, w + sCELL);
})
A(void, set_xt, (X* x, CELL w, CELL xt), { 
	sloth_store(x, w + sCELL, xt); 
})

A(uCHAR, get_flags, (X* x, CELL w), {
	return sloth_c_fetch(x, w + 2*sCELL);
})

A(uCHAR, set_flags, (X* x, CELL w, uCHAR v), {
	sloth_c_store(x, w + 2*sCELL, v);
})

A(CELL, has_flag, (X* x, CELL w, CELL v), { 
	return sloth_get_flags(x, w) & v; 
})

A(void, set_flag, (X* x, CELL w, uCHAR v), {
	sloth_set_flags(x, w, sloth_get_flags(x, w) | v);
})

A(void, unset_flag, (X* x, CELL w, uCHAR v), {
	sloth_set_flags(x, w, sloth_get_flags(x, w) & ~v);
})

A(uCHAR, get_namelen, (X* x, CELL w), {
	return sloth_c_fetch(x, w + 2*sCELL + suCHAR);
})

A(CELL, get_name_addr, (X* x, CELL w), {
	return w + 2*sCELL + 2*suCHAR;
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

P(immediate, { 
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_IMMEDIATE); 
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

/* Inner interpreter primitives */

P(exit, { x->ip = (x->rp > 0) ? sloth_rpop(x) : -1; })

P(lit, { sloth_push(x, sloth_op(x)); })
P(rip, {
	CELL ip = x->ip;
	CELL o = sloth_op(x);
	sloth_push(x, ip + o - sCELL);
})

P(branch, { x->ip += sloth_op(x) - sCELL; })
P(zbranch, { 
	x->ip += sloth_pop(x) == 0 ? 
		(sloth_op(x) - sCELL) 
		: sCELL; 
})

/* -- Exceptions --------------------------------------- */

A(void, catch, (X* x, CELL q), {
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

A(void, throw, (X* x, CELL e), {
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

P(catch, { sloth_catch(x, sloth_pop(x)); })
P(throw, { CELL e = sloth_pop(x); if (e) sloth_throw(x, e); })

/* -- Strings ------------------------------------------ */

P(string, {
	CELL l = sloth_op(x);
	sloth_push(x, x->ip);
	sloth_push(x, l);
	x->ip = sloth_aligned(x->ip + l + 1);
})

/* TODO (CSTRING) is used only by CLITERAL that is used only */
/* by C" that is never used. It could be taken from here by */
/* making a Forth version that stores a normal string literal */
/* and copies it to a transient region or to the CBuffer, */
/* or even to the normal string buffer. */
P(c_string, {
	uCHAR l = sloth_c_fetch(x, x->ip);
	sloth_push(x, x->ip);
	x->ip = sloth_aligned(x->ip + l + 2);
})

/* -- Searching for words ------------------------------ */

I(int, compare_no_case, (X* x, CELL a1, uCELL u1, CELL a2, uCELL u2), {
	int i;
	if (u1 != u2) return 0;
	for (i = 0; i < u2; i++) {
		uCHAR a = sloth_c_fetch(x, a1 + i);
		uCHAR b = sloth_c_fetch(x, a2 + i);
		if (a >= 97 && a <= 122) a -= 32;
		if (b >= 97 && b <= 122) b -= 32;
		if (a != b) return 0;
	}
	return 1;
})

I(CELL, search_word, (X* x, CELL n, int l), {
	CELL wl, w, i;
	/* The wordlist iteration starts at -1 to always search */
	/* on (LOCALS-WORDLIST) first, even if the search order */
	/* is completely empty. */
	for (i = -1; i < sloth_user_get(x, SLOTH_ORDER); i++) {
		wl = sloth_user_get(x, SLOTH_CONTEXT + i*sCELL);
		if (wl != 0) {
			w = sloth_fetch(x, wl);
			while (w > 0) {
				if (!sloth_has_flag(x, w, SLOTH_HIDDEN) 
				 && sloth_compare_no_case(
							x, 
							sloth_get_name_addr(x, w), sloth_get_namelen(x, w),
							n, l)) 
					return w;
				w = sloth_get_link(x, w);
			}
		}
	}
	return 0;
})

P(find, {
	CELL cstring = sloth_pop(x);
	CELL w = sloth_search_word(
		x, 
		cstring + suCHAR, 
		sloth_c_fetch(x, cstring)
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
})

A(CELL, find_word, (X* x, char* name), {
	return sloth_search_word(x, (CELL)name, strlen(name));
})

/* -- More compilation --------------------------------- */

A(void,  compile, (X* x, CELL xt), { sloth_comma(x, xt); })

A(void, literal, (X* x, CELL n), { 
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(LIT)")));
	sloth_comma(x, n); 
})

/* -- Quotations --------------------------------------- */

P(quotation, { 
	CELL d = sloth_op(x); 
	sloth_push(x, x->ip); 
	x->ip += d; 
})

P(start_quotation, {
	CELL s = sloth_user_get(x, SLOTH_STATE);
	/* If in interpret mode (state<=0), substract 1 from state. */
	/* If in compile mode (state>0), add 1 to state. */
	sloth_user_set(x, SLOTH_STATE, s <= 0 ? s - 1 : s + 1);
	/* If in interpret mode and this is a first level quotation, */
	/* push its starting address to be able to execute it later. */
	if (sloth_user_get(x, SLOTH_STATE) == -1) 
		sloth_push(x, sloth_here(x) + 2*sCELL);
	/* Push latestXT to be save it while quotation is compiled. */
	/* LatestXT is needed by recurse. */
	sloth_push(x, sloth_user_get(x, SLOTH_LATESTXT));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(QUOTATION)")));
	/* Push HERE to be able to patch it later with the correct */
	/* jump distance, and reserve 1 cell for it. */
	sloth_push(x, sloth_here(x));
	sloth_comma(x, 0);
	/* Set latestXT to start of quotation, after (QUOTATION) and */
	/* the cell with the jump distance. */
	sloth_user_set(x, SLOTH_LATESTXT, sloth_here(x));
})

P(end_quotation, {
	CELL s = sloth_user_get(x, SLOTH_STATE), a = sloth_pop(x);
	/* Compile an EXIT at the end of the quotation. */
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	/* Store the correct jump distance in the reserved cell after */
	/* the (QUOTATION) word. */
	sloth_store(x, a, sloth_here(x) - a - sCELL);
	/* Restore previous latestXT */
	sloth_user_set(x, SLOTH_LATESTXT, sloth_pop(x));
	/* Restore previous state */
	sloth_user_set(x, SLOTH_STATE, s < 0 ? s + 1 : s - 1);
})

/* -- Bootstrapping ------------------------------------ */

A(void, bootstrap, (X* x), {
	/* Basic primitives */

	sloth_code(x, "EXIT", sloth_primitive(x, &sloth_exit_));
	sloth_code(x, "(LIT)", sloth_primitive(x, &sloth_lit_));
	sloth_code(x, "(RIP)", sloth_primitive(x, &sloth_rip_));
	sloth_code(x, "(BRANCH)", sloth_primitive(x, &sloth_branch_));
	sloth_code(x, "(?BRANCH)", sloth_primitive(x, &sloth_zbranch_));

	/* Data and return stack */

	sloth_code(x, "DROP", sloth_primitive(x, &sloth_drop_));
	sloth_code(x, "DUP", sloth_primitive(x, &sloth_dup_));
	sloth_code(x, "OVER", sloth_primitive(x, &sloth_over_));
	sloth_code(x, ">R", sloth_primitive(x, &sloth_to_r_));
	sloth_code(x, "R>", sloth_primitive(x, &sloth_r_from_));
	sloth_code(x, "SWAP", sloth_primitive(x, &sloth_swap_));

	/* Memory */

	sloth_code(x, "C@", sloth_primitive(x, &sloth_c_fetch_));
	sloth_code(x, "C!", sloth_primitive(x, &sloth_c_store_));
	sloth_code(x, "@", sloth_primitive(x, &sloth_fetch_));
	sloth_code(x, "!", sloth_primitive(x, &sloth_store_));

	sloth_code(x, "CELLS", sloth_primitive(x, &sloth_cells_));
	sloth_code(x, "CHARS", sloth_primitive(x, &sloth_chars_));

	/* Exceptions */

	sloth_code(x, "CATCH", sloth_primitive(x, &sloth_catch_));
	sloth_code(x, "THROW", sloth_primitive(x, &sloth_throw_));

	/* Strings */

	sloth_code(x, "(STRING)", sloth_primitive(x, &sloth_string_));
	sloth_code(x, "(CSTRING)", sloth_primitive(x, &sloth_c_string_));

	/* Quotations */

	sloth_code(x, "(QUOTATION)", sloth_primitive(x, &sloth_quotation_));
	sloth_code(x, "[:", sloth_primitive(x, &sloth_start_quotation_)); sloth_immediate_(x);
	sloth_code(x, ";]", sloth_primitive(x, &sloth_end_quotation_)); sloth_immediate_(x);

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
*/

/*
	sloth_code(x, "ALLOT", sloth_primitive(x, &sloth_allot_));
*/
})

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
	/* Initialize INTERNAL-WORDLIST */
	*((CELL*)(x->d + 1*sCELL)) = 0;
	/* Initialize FORTH-WORDLIST (the default wordlist) */
	*((CELL*)(x->d + 2*sCELL)) = 0;

	/* Initialize CURRENT to point to FORTH-WORDLIST */
	*((CELL*)x->u) = sloth_to_abs(x, SLOTH_FORTH_WL); /* CURRENT */
	*((CELL*)(x->u + 1*sCELL)) = 2; /* #ORDER */
	*((CELL*)(x->u + 2*sCELL)) = 0; /* LOCALS-WORDLIST */
	/* CONTEXT 0 */
	*((CELL*)(x->u + 3*sCELL)) = sloth_to_abs(x, SLOTH_FORTH_WL);
	/* CONTEXT 1 */
	*((CELL*)(x->u + 4*sCELL)) = sloth_to_abs(x, SLOTH_INTERNAL_WL);
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

#endif
