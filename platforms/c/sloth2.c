#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

/* ---------------------------------------------------- */
/* -- Virtual machine API ----------------------------- */
/* ---------------------------------------------------- */

typedef int8_t CHAR;
typedef intptr_t CELL;

#define sCELL sizeof(CELL)
#define sCHAR sizeof(CHAR)

/* Predefined sizes, they should be modifiable */
/* before context creation */
#define SSIZE 64
#define RSIZE 64
#define DSIZE 65536
#define PSIZE 256

struct VM;
typedef void (*F)(struct VM*);

typedef struct PRIMITIVES {
	CELL sz;
	CELL last;
	F *p;
} P;

typedef struct VM { 
	CELL s[SSIZE], sp;
	CELL r[RSIZE], rp;
	CELL ip;
	CELL d, h, sz;	/* Dict base address, here, size */

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	/* F *p; */
	P *p;
} X;

void init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->h = 0;
	x->sz = sz;

	x->jmpbuf_idx = -1;
}

/* Data stack */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }
CELL pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

/* Return stack */

void rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL rpop(X* x) { x->rp--; return x->r[x->rp]; }
CELL rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

/* Memory */

/* 
STORE/FETCH/CSTORE/cfetch work on to_absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void store(X* x, CELL a, CELL v) { *((CELL*)a) = v; }
CELL fetch(X* x, CELL a) { return *((CELL*)a); }
void cstore(X* x, CELL a, CHAR v) { *((CHAR*)a) = v; }
CHAR cfetch(X* x, CELL a) { return *((CHAR*)a); }

/*
here/ALLOT/align/alignED word on to_relative address units.
*/
CELL here(X* x) { return x->h; }
void allot(X* x, CELL v) { x->h += v; }
void align(X* x) { x->h = (x->h + (sCELL - 1)) & ~(sCELL - 1); }
CELL aligned(X* x, CELL a) { return (a + (sCELL - 1)) & ~(sCELL - 1); }

/*
The next two macros allow transforming from to_relative to
to_absolute address.
*/
CELL to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL to_rel(X* x, CELL a) { return a - x->d; }

/* Inner interpreter */

CELL op(X* x) { 
	CELL o = fetch(x, x->ip);
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

void trace(X* x, CELL l) {
	CELL i;
	printf("[%ld] ", x->sp);
	for (i = 0; i < x->sp; i++) {
		printf("%ld ", x->s[i]);
	}
	printf("\n");
}

void inner(X* x) { 
	CELL t = x->rp; 
	trace(x, 3);
	while (t <= x->rp && x->ip >= 0) { 
		execute(x, op(x)); 
		trace(x, 3);
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

	if (!(e = setjmp(x->jmpbuf[x->jmpbuf_idx++]))) {
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
/* -- Virtual machine primitives ---------------------- */
/* ---------------------------------------------------- */

/* 
TODO: This primitives are implemented using only functions
and macros from the previous section thus making it
easier to port to other platforms.
TODO: Ensure there are enough functions to allow that.
*/

void _noop(X* x) { }
void _exit(X* x) { x->ip = x->rp > 0 ? rpop(x) : -1; }
void _lit(X* x) { push(x, op(x)); }
void _branch(X* x) { x->ip += op(x); }
void _zbranch(X* x) { x->ip += pop(x) > 0 ? op(x) - sCELL : 0; }

void _drop(X* x) { pop(x); }
void _pick(X* x) { push(x, pick(x, pop(x))); }
void _over(X* x) { push(x, pick(x, 1)); }
void _swap(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); }

void _r_to(X* x) { rpush(x, pop(x)); }
void _r_fetch(X* x) { push(x, rpick(x, 0)); }
void _r_from(X* x) { push(x, rpop(x)); }

void _add(X* x) { CELL a = pop(x); push(x, pop(x) + a); }
void _sub(X* x) { CELL a = pop(x); push(x, pop(x) - a); }
void _mul(X* x) { CELL a = pop(x); push(x, pop(x) * a); }
void _div(X* x) { CELL a = pop(x); push(x, pop(x) / a); }
void _mod(X* x) { CELL a = pop(x); push(x, pop(x) % a); }

void _two_slash(X* x) { push(x, pop(x) >> 1); }
void _rshift(X* x) { CELL a = pop(x); push(x, pop(x) >> a); }
void _lshift(X* x) { CELL a = pop(x); push(x, pop(x) << a); }

/* 
To implement this words, check:
pforth/csrc/pf_inner.c (around line 700)
*/
void _times_slash_mod(X* x) { /* TODO */ }
void _um_times(X* x) { /* TODO */ }
void _um_slash_mod(X* x) { /* TODO */ }

void _lt(X* x) { CELL a = pop(x); push(x, pop(x) < a ? -1 : 0); }
void _eq(X* x) { CELL a = pop(x); push(x, pop(x) == a ? -1 : 0); }
void _gt(X* x) { CELL a = pop(x); push(x, pop(x) > a ? -1 : 0); }

void _and(X* x) { CELL a = pop(x); push(x, pop(x) & a); }
void _or(X* x) { CELL a = pop(x); push(x, pop(x) | a); }
void _xor(X* x) { CELL a = pop(x); push(x, pop(x) ^ a); }
void _invert(X* x) { push(x, ~pop(x)); }

void _fetch(X* x) { push(x, fetch(x, pop(x))); }
void _store(X* x) { CELL a = pop(x); store(x, a, pop(x)); }
void _cfetch(X* x) { push(x, cfetch(x, pop(x))); }
void _cstore(X* x) { CELL a = pop(x); cstore(x, a, pop(x)); }

void _catch(X* x) { catch(x, pop(x)); }
void _throw(X* x) { throw(x, pop(x)); }

void primitives(X* x) {
	x->p->p[0] = &_noop;
	x->p->p[1] = &_exit;
	x->p->p[2] = &_lit;
	x->p->p[3] = &_branch;
	x->p->p[4] = &_zbranch;

	x->p->p[5] = &_drop;
	x->p->p[6] = &_pick;
	x->p->p[7] = &_over;
	x->p->p[8] = &_swap;

	x->p->p[9] = &_r_to;
	x->p->p[10] = &_r_fetch;
	x->p->p[11] = &_r_from;

	x->p->p[12] = &_add;
	x->p->p[13] = &_sub;
	x->p->p[14] = &_mul;
	x->p->p[15] = &_div;
	x->p->p[16] = &_mod;

	x->p->p[17] = &_two_slash;
	x->p->p[18] = &_rshift;
	x->p->p[19] = &_lshift;

	x->p->p[20] = &_times_slash_mod;
	x->p->p[21] = &_um_times;
	x->p->p[22] = &_um_slash_mod;

	x->p->p[23] = &_lt;
	x->p->p[24] = &_eq;
	x->p->p[25] = &_gt;

	x->p->p[26] = &_and;
	x->p->p[27] = &_or;
	x->p->p[28] = &_xor;
	x->p->p[29] = &_invert;

	x->p->p[30] = &_fetch;
	x->p->p[31] = &_store;
	x->p->p[32] = &_cfetch;
	x->p->p[33] = &_cstore;

	x->p->p[34] = &_catch;
	x->p->p[35] = &_throw;

	x->p->last = 36;
}

void load_image(X* x, char* filename) {
	FILE *f = fopen(filename, "rb");
	CELL sz;
	if (f) {
		fseek(f, 0L, SEEK_END);
		sz = ftell(f);
		rewind(f);
		if (x->sz < sz) { return; /* TODO: Error */ }
		fread((void*)x->d, 1, sz, f);
		fclose(f);
	} else {
		return; /* TODO: Error */
	}
}

void save_image(X* x, char* filename) {
	FILE *f = fopen(filename, "wb");
	if (f) {
		fwrite((void*)x->d, 1, x->h, f);
		fclose(f);
	} else {
		return; /* TODO: Error */
	}
}

/* ---------------------------------------------------- */
/* -- Bootstrapping ----------------------------------- */
/* ---------------------------------------------------- */

#define EXIT					-2
#define LIT						-3
#define COMPILE				-4

#define LATEST				0
#define STATE					sCELL
#define IBUF					2*sCELL
#define IPOS					3*sCELL
#define ILEN					4*sCELL
#define ORDER					5*sCELL

void set(X* x, CELL a, CELL v) { store(x, to_abs(x, a), v); }
CELL get(X* x, CELL a) { return fetch(x, to_abs(x, a)); }

void cset(X* x, CELL a, CHAR v) { cstore(x, to_abs(x, a), v); }
CHAR cget(X* x, CELL a) { return cfetch(x, to_abs(x, a)); }

void comma(X* x, CELL v) { set(x, here(x), v); allot(x, sCELL); }
void ccomma(X* x, CHAR v) { set(x, here(x), v); allot(x, sCHAR); }

void compile(X* x, CELL xt) { comma(x, xt); }
void literal(X* x, CELL n) { comma(x, LIT); comma(x, n); }

void _compile(X* x) { compile(x, pop(x)); }

#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANT				4

CELL get_link(X* x, CELL w) { return get(x, w); }

void set_xt(X* x, CELL w, CELL xt) { set(x, w + sCELL, xt); }
CELL get_xt(X* x, CELL w) { return get(x, w + sCELL); }

CELL get_wl(X* x, CELL w) { return get(x, w + 2*sCELL); }

CHAR get_flags(X* x, CELL w) { return cget(x, w + 3*sCELL); }
void set_flag(X* x, CELL w, CHAR v) { cset(x, w + 3*sCELL, get_flags(x, w) | v); }
void unset_flag(X* x, CELL w, CHAR v) { cset(x, w + 3*sCELL, get_flags(x, w) & ~v); }

CELL has_flag(X* x, CELL w, CELL v) { return get_flags(x, w) & v; }

CHAR get_namelen(X* x, CELL w) { return cget(x, w + 3*sCELL + sCHAR); }

CELL get_name_addr(X* x, CELL w) { return to_abs(x, w + 3*sCELL + 2*sCHAR); }

void set_hidden(X* x) { set_flag(x, get(x, LATEST), HIDDEN); }
void set_immediate(X* x) { set_flag(x, get(x, LATEST), IMMEDIATE); }
void set_instant(X* x) { set_flag(x, get(x, LATEST), INSTANT); }

void unset_hidden(X* x) { unset_flag(x, get(x, LATEST), HIDDEN); }

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = here(x); 
	comma(x, get(x, LATEST)); /* link */
	set(x, LATEST, w);
	comma(x, 0);	/* xt */
	comma(x, 1);	/* wordlist */
	ccomma(x, 0);	/* flags */
	ccomma(x, l);	/* namelen */
	for (i = 0; i < l; i++) ccomma(x, fetch(x, n + i));
	align(x);
	set_xt(x, w, here(x));
	return w;
}

CELL primitive(X* x, F f) { 
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL code(X* x, char* name, CELL xt) {
	CELL w = header(x, (CELL)name, strlen(name));
	set_xt(x, w, xt);
	return xt; 
}

/* -- Additional code words -----------------------------*/

void _parse_name(X* x) {
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	while (ipos < ilen && cfetch(x, ibuf + ipos) < 33) ipos++;
	push(x, ibuf + ipos);
	while (ipos < ilen && cfetch(x, ibuf + ipos) > 32) ipos++;
	push(x, ibuf + ipos - pick(x, 0));
	if (ipos < ilen) ipos++;
	set(x, IPOS, ipos);
}

void _colon(X* x) {
	CELL l, t; 
	_parse_name(x); l = pop(x); t = pop(x);
	header(x, t, l); 
	set_hidden(x);
	set(x, STATE, 1);
	/* x->level = 1; */
}

void _semicolon(X* x) {
	compile(x, EXIT); 
	set(x, STATE, 0);
	/* x->level = 0; */
	unset_hidden(x);
}

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

void _find_name_in(X* x) {
	CELL wid = pop(x), l = pop(x), a = pop(x), w = get(x, LATEST);
	while (w != 0) {
		if (get_wl(x, w) == wid && compare_without_case(x, w, a, l) && !has_flag(x, w, HIDDEN)) break;
		w = get_link(x, w);
	}
	push(x, w);
}

void _find_name(X* x) { eval(x, get(x, ORDER)); }

void _default_order(X* x) { push(x, 1); _find_name_in(x); }

void postpone(X* x, CELL nt) {
	if (has_flag(x, nt, IMMEDIATE)) compile(x, get_xt(x, nt));
	else {
		literal(x, get_xt(x, nt));
		compile(x, COMPILE);
	}
}

/* -- Outer interpreter --------------------------------*/

void interpret(X* x) {
	CELL w, tok, tlen, n;
	char buf[15]; char *endptr;
	printf("------- Interpreting >> [%ld] %.*s", get(x, IPOS), (int)get(x, ILEN), (char*)get(x, IBUF));
	while ((get(x, IPOS)) < (get(x, ILEN))) {
		_parse_name(x);
		if (pick(x, 0) == 0) { pop(x); pop(x); return; }
		tok = pick(x, 1); tlen = pick(x, 0);
		printf("TOKEN: [%.*s]\n", tlen, tok);
		_find_name(x);
		w = pop(x);
		if (w != 0) {
			if (get(x, STATE) == 0
			|| (get(x, STATE) == 1 && has_flag(x, w, IMMEDIATE))
			|| (get(x, STATE) == 2 && has_flag(x, w, INSTANT))) {
				eval(x, get_xt(x, w));
			} else {
				if (get(x, STATE) == 1) { 
					compile(x, get_xt(x, w));
				} else { /* STATE == 2 */ 
					postpone(x, w);
				}
			}
		} else {
			printf("TOK: %ld TLEN: %d\n", tok, tlen);
			printf("Converting to number the string [%.*s]\n", tlen, (char*)tok);
			strncpy(buf, (char*)tok, tlen);
			buf[tlen] = 0;
			n = strtol(buf, &endptr, 10);	
			if (*endptr == '\0') {
				if (get(x, STATE) == 0) push(x, n);
				else literal(x, n);
			} else {
				printf("%.*s ?\n", (int)tlen, (char*)tok);
			}
		}
		trace(x, 2);
	}
}

/* -- Include file -------------------------------------*/

void evaluate(X* x, char* buf, CELL len, CELL sid) {
	/* Save previous source */
	char source[1024];
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);

	strncpy(source, (char*)ibuf, ilen);

	set(x, IBUF, (CELL)buf);
	set(x, ILEN, len);	
	set(x, IPOS, 0);

	interpret(x);

	/* Restore previous source */
	set(x, IBUF, ibuf);
	set(x, ILEN, ilen);
	set(x, IPOS, ipos);
}

void _included(X* x) {
	FILE *f;
	char filename[1024];
	char linebuf[1024];

	CELL l = pop(x);
	CELL a = pop(x);
	strncpy(filename, (char*)a, (size_t)l);
	filename[l] = 0;

	f = fopen(filename, "r");

	if (f) {
		while (fgets(linebuf, 1024, f)) {
			printf(">> %s", linebuf);
			evaluate(x, linebuf, strlen(linebuf), (CELL)f);
		}

		fclose(f);
	} else {
		/* TODO: Manage error */
	}
}

void bootstrap(X* x) {
	comma(x, 0); /* LATEST */
	comma(x, 0); /* STATE */
	comma(x, 0); /* IBUF */
	comma(x, 0); /* ILEN */
	comma(x, 0); /* IPOS */
	comma(x, primitive(x, &_default_order)); /* ORDER */

	code(x, "NOOP", primitive(x, &_noop));
	code(x, "EXIT", primitive(x, &_exit));
	code(x, "LIT", primitive(x, &_lit));
	code(x, "COMPILE,", primitive(x, &_compile));
	code(x, "BRANCH", primitive(x, &_branch));
	code(x, "ZBRANCH", primitive(x, &_zbranch));

	code(x, "DROP", primitive(x, &_drop));
	code(x, "PICK", primitive(x, &_pick));
	code(x, "OVER", primitive(x, &_over));
	code(x, "SWAP", primitive(x, &_swap));

	code(x, ">R", primitive(x, &_r_to));
	code(x, "R@", primitive(x, &_r_fetch));
	code(x, "R>", primitive(x, &_r_from));

	code(x, "+", primitive(x, &_add));
	code(x, "-", primitive(x, &_sub));
	code(x, "*", primitive(x, &_mul));
	code(x, "/", primitive(x, &_div));
	code(x, "MOD", primitive(x, &_mod));

	code(x, "2/", primitive(x, &_two_slash));
	code(x, "RSHIFT", primitive(x, &_rshift));
	code(x, "LSFHIT", primitive(x, &_lshift));

	code(x, "*/MOD", primitive(x, &_times_slash_mod));
	code(x, "UM*", primitive(x, &_um_times));
	code(x, "UM/MOD", primitive(x, &_um_slash_mod));

	code(x, "<", primitive(x, &_lt));
	code(x, "=", primitive(x, &_eq));
	code(x, ">", primitive(x, &_gt));

	code(x, "AND", primitive(x, &_and));
	code(x, "OR", primitive(x, &_or));
	code(x, "XOR", primitive(x, &_xor));
	code(x, "INVERT", primitive(x, &_invert));

	code(x, "@", primitive(x, &_fetch));
	code(x, "!", primitive(x, &_store));
	code(x, "C@", primitive(x, &_cfetch));
	code(x, "C!", primitive(x, &_cstore));

	code(x, "CATCH", primitive(x, &_catch));
	code(x, "THROW", primitive(x, &_throw));

	/* This code words are not in the basic primitive set */

	code(x, "PARSE-NAME", primitive(x, &_parse_name));
	code(x, "FIND-NAME", primitive(x, &_find_name));

	code(x, ":", primitive(x, &_colon));
	code(x, ";", primitive(x, &_semicolon)); set_immediate(x);

	code(x, "INCLUDED", primitive(x, &_included));
}

/* ---------------------------------------------------- */
/* -- Standalone -------------------------------------- */
/* ---------------------------------------------------- */

int main() {
	/* Image loading */
	/*
	X* x = malloc(sizeof(X));
	x->p = malloc(sizeof(P));
	x->p->p = malloc(sizeof(F) * PSIZE);
	x->p->last = 0;
	x->p->sz = PSIZE;
	primitives(x);
	x->d = (CELL)malloc(DSIZE);
	init(x, x->d, DSIZE);

	load_image(x, "test.slimg");
	x->ip = x->d;
	inner(x);

	free((void*)x->d);
	free(x->p);
	free(x);
	*/
	/* Image creation */
	/*
	X* x = malloc(sizeof(X));
	x->p = malloc(sizeof(P));
	x->p->p = malloc(sizeof(F) * PSIZE);
	x->p->last = 0;
	x->p->sz = PSIZE;
	x->d = (CELL)malloc(DSIZE);
	init(x, x->d, DSIZE);
	bootstrap(x);

	save_image(x, "test2.slimg");

	free((void*)x->d);
	free(x->p);
	free(x);
	*/
	/* REPL */
	char *filename = "../../4th/sloth.4th";
	X* x = malloc(sizeof(X));
	x->p = malloc(sizeof(P));
	x->p->p = malloc(sizeof(F) * PSIZE);
	x->p->last = 0;
	x->p->sz = PSIZE;
	x->d = (CELL)malloc(DSIZE);
	init(x, x->d, DSIZE);
	bootstrap(x);

	push(x, (CELL)filename);
	push(x, strlen(filename));
	_included(x);

	free((void*)x->d);
	free(x->p);
	free(x);
}
