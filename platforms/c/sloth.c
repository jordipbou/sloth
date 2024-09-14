#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<setjmp.h>

/* --------------------------------------------------------------- */
/* -- Virtual machine -------------------------------------------- */
/* --------------------------------------------------------------- */

typedef int8_t CHAR;
typedef intptr_t CELL;

#define SSIZE 64
#define RSIZE 64
#define MSIZE 65536
#define PSIZE 256

struct VM;
typedef void (*F)(struct VM*);

typedef struct VM { 
	CELL s[SSIZE], sp;
	CELL r[RSIZE], rp;
	CELL ip;
	CELL mp, ms;
	CELL tp;

	CELL ix, jx, kx;	/* Loop registers */
	CELL lx;					/* Leave register */

	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* I'm not sure if primitives array must be inside this struct */
	F p[PSIZE];	/* Array of primitives */
	CELL pp; /* Number of primitives */

	/* I'm totally sure these do not belong here */
	CELL latest, latestxt;
	CELL state;
	CELL base;
	CELL ibuf, ipos, ilen;
	CELL tok, tlen;
	CELL source_id;

	CELL trace;

	CELL EXIT;
	CELL LIT;
	CELL QUOTATION;
	CELL STRING;
	CELL COMPILE;
	CELL ORDER;

	/* Used to implement quotations */
	CELL level;
} X;

void init(X* x) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->mp = 0; 
	x->pp = 0; 

	x->jmpbuf_idx = -1;

	x->trace = 2;
}

/* Data stack */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }

void place(X* x, CELL a, CELL v) { x->s[x->sp - a - 1] = v; }
int pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

void drop(X* x) { x->sp--; }
void over(X* x) { push(x, pick(x, 1)); }
void swap(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); }

/* Return stack */

void rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL rpop(X* x) { x->rp--; return x->r[x->rp]; }

CELL rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

/* Memory */

#define STORE(x, a, v)	*((CELL*)(a)) = ((CELL)(v))
#define FETCH(x, a)			*((CELL*)(a))
#define CSTORE(x, a, v)	*((CHAR*)(a)) = ((CHAR)(v))
#define CFETCH(x, a)		*((CHAR*)(a))

#define HERE(x)			((x)->mp)
#define ALLOT(x, v)	((x)->mp += (v))
#define ALIGN(x)		HERE(x) = (HERE(x) + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1)
#define ALIGNED(x, a) (((a) + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1))

/* Inner interpreter */

void noop(X* x) { }

CELL opcode(X* x) { CELL o = FETCH(x, x->ip); x->ip += sizeof(CELL); return o; }

void do_prim(X* x, CELL p) { (x->p[-1 - p])(x); }
void call(X* x, CELL q) { if (x->ip >= 0) rpush(x, x->ip); x->ip = q; }

void execute(X* x, CELL q) { if (q < 0) do_prim(x, q); else call(x, q); }

void inner(X* x) { CELL t = x->rp; while (t <= x->rp && x->ip >= 0) { execute(x, opcode(x)); } }

void eval(X* x, CELL q) { execute(x, q); if (q > 0)	inner(x); }

/* --------------------------------------------------------------- */
/* -- Helpers for assembling CODE words (assembler?) ------------- */
/* --------------------------------------------------------------- */

/* Adds a new primitive to the primitives array and returns its index */
CELL noname(X* x, F f) { x->p[x->pp] = f; x->pp++; return 0 - x->pp; }

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

void _catch(X* x) { catch(x, pop(x)); }

void throw(X* x, CELL e) {
	longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
}

void _throw(X* x) { throw(x, pop(x)); }

/* --------------------------------------------------------------- */
/* -- Words that are not virtual machine basics ------------------ */
/* --------------------------------------------------------------- */

void _pick(X* x) { push(x, pick(x, pop(x))); }
void depth(X* x) { push(x, x->sp); }

void to_r(X* x) { rpush(x, pop(x)); }
/* void r_fetch(X* x) { push(x, rpick(x, 0)); } */
void from_r(X* x) { push(x, rpop(x)); }

void store(X* x) { CELL a = pop(x); STORE(x, a, pop(x)); }
void fetch(X* x) { push(x, FETCH(x, pop(x))); }

void cstore(X* x) { CELL a = pop(x); CSTORE(x, a, pop(x)); }
void cfetch(X* x) { push(x, CFETCH(x, pop(x))); }

/* TODO: This are not basic but code words (primitives) */
void here(X* x) { push(x, x->mp); }
void allot(X* x) { x->mp += pop(x); }
void align(X* x) { x->mp = (x->mp + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1); }
void unused(X* x) { push(x, x->ms - x->mp); }

void there(X* x) { push(x, x->tp); }
void tallot(X* x) { x->tp -= pop(x); push(x, x->tp); }

void _execute(X* x) { execute(x, pop(x)); }

/* --------------------------------------------------------------- */
/* -- Dictionary ------------------------------------------------- */
/* --------------------------------------------------------------- */

#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANTANEOUS	4

void get_latest(X* x) { push(x, x->latest); }
void set_latest(X* x) { x->latest = pop(x); }

#define COMMA(x, v)			{ STORE((x), (x)->mp, (v)); (x)->mp += sizeof(CELL); }
#define CCOMMA(x, v)		{ CSTORE((x), (x)->mp, (v)); (x)->mp += sizeof(CHAR); }

#define GET_LINK(x, w)			FETCH((x), (w))
#define SET_XT(x, w, v)			STORE((x), ((w) + sizeof(CELL)), (v))
#define GET_XT(x, w)				FETCH((x), ((w) + sizeof(CELL)))
#define SET_WL(x, w, v)			STORE((x), ((w) + 2*sizeof(CELL)))
#define GET_WL(x, w)				FETCH((x), ((w) + 2*sizeof(CELL)))
#define GET_FLAGS(x, w)			CFETCH((x), ((w) + 3*sizeof(CELL)))
#define SET_FLAGS(x, w, v)	CSTORE((x), ((w) + 3*sizeof(CELL)), GET_FLAGS((x), (w)) | (v))
#define UNSET_FLAGS(x, w, v)	CSTORE((x), ((w) + 3*sizeof(CELL)), GET_FLAGS((x), (w)) & ~(v))
#define GET_NAMELEN(x, w)		CFETCH((x), ((w) + 3*sizeof(CELL) + sizeof(CHAR)))
#define NAME_ADDR(x, w)			((w) + 3*sizeof(CELL) + 2*sizeof(CHAR))

#define HAS_FLAG(x, w, f)		((GET_FLAGS((x), (w)) & (f)) == f)

void set_hidden(X* x) { SET_FLAGS(x, x->latest, HIDDEN); }
void set_immediate(X* x) { SET_FLAGS(x, x->latest, IMMEDIATE); }
void set_instantaneous(X* x) { SET_FLAGS(x, x->latest, INSTANTANEOUS); }

void unset_hidden(X* x) { UNSET_FLAGS(x, x->latest, HIDDEN); }

void literal(X* x, CELL n) { COMMA(x, x->LIT); COMMA(x, n); }
void _literal(X* x) { literal(x, pop(x)); }
void compile(X* x, CELL n) { COMMA(x, n); }
void _compile(X* x) { compile(x, pop(x)); }
void postpone(X* x, CELL w) {
	if (HAS_FLAG(x, w, IMMEDIATE)) compile(x, GET_XT(x, w));
	else {
		literal(x, GET_XT(x, w));
		compile(x, x->COMPILE);
	}
}
void end_postpone_mode(X* x) { x->state = 1; }

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = HERE(x); COMMA(x, x->latest); x->latest = w;	/* link */
	COMMA(x, 0);	/* xt */
	COMMA(x, 1);	/* wordlist */
	CCOMMA(x, 0);	/* flags */
	CCOMMA(x, l);	/* namelen */
	for (i = 0; i < l; i++) CCOMMA(x, FETCH(x, n + i));
	align(x);
	SET_XT(x, w, HERE(x));
	return w;
}

int compare_without_case(X* x, CELL w, CELL t, CELL l) {
	int i;
	if (GET_NAMELEN(x, w) != l) return 0;
	for (i = 0; i < l; i++) {
		CHAR a = CFETCH(x, t + i);
		CHAR b = CFETCH(x, NAME_ADDR(x, w) + i);
		if (a >= 97 && a <= 122) a -= 32;
		if (b >= 97 && b <= 122) b -= 32;
		if (a != b) return 0;
	}
	return 1;
}

/*
void find(X* x) {
	CELL l = pop(x), t = pop(x);
	CELL w = x->latest;
	while (w != 0) {
		if (compare_without_case(x, w, t, l) && !HAS_FLAG(x, w, HIDDEN)) break;
		w = GET_LINK(x, w);
	}
	push(x, w);
}
*/

void find_name_in(X* x) {
	CELL wid = pop(x), l = pop(x), a = pop(x), w = x->latest;
	while (w != 0) {
		if (GET_WL(x, w) == wid && compare_without_case(x, w, a, l) && !HAS_FLAG(x, w, HIDDEN)) break;
		w = GET_LINK(x, w);
	}
	push(x, w);
}

void find_name(X* x) { eval(x, x->ORDER); }

void default_order(X* x) { push(x, 1); find_name_in(x); }

/* --------------------------------------------------------------- */
/* -- Debugging -------------------------------------------------- */
/* --------------------------------------------------------------- */

void dump_stack(X* x) { 
	int i;
	printf("[");
	for (i = 0; i < x->sp; i++) 
		printf("%ld ", x->s[i]); 
	printf("] ");
}

void dump_rstack(X* x) {
	CELL i;
	if (x->ip > 0) printf(": [%ld] ", x->ip);
	for (i = x->rp - 1; i >= 0; i--) {
		printf(": [%ld] ", x->r[i]);
	}
}

void dump_input(X* x) {
	printf("\033[93m%.*s\033[0m ", (int)x->tlen, (char*)x->tok);
	printf("%.*s", (int)x->ilen, (char*)(x->ibuf + x->ipos));
}

void trace(X* x, int l) {
	if (x->trace >= l) {
		printf("%ld %ld [%ld %ld] ", x->mp, x->tp, x->state, x->level);
		dump_stack(x);
		dump_rstack(x);
		dump_input(x);
		printf("\n");
	}
}

/* --------------------------------------------------------------- */
/* -- Bootstrapping ---------------------------------------------- */
/* --------------------------------------------------------------- */

CELL primitive(X* x, char* n, CELL l, CELL xt) { 
	CELL w = header(x, (CELL)n, l);
	SET_XT(x, w, xt);
	return xt; 
}

CELL constant(X* x, char* n, CELL l, CELL v) {
	CELL w = header(x, (CELL)n, l);
	literal(x, v);
	compile(x, x->EXIT);
	return HERE(x) - 2*sizeof(CELL);
}

void start_quotation(X* x) {
	x->level++;
	if (x->level == 1) x->state = 1;
	push(x, x->latestxt);
	compile(x, x->QUOTATION);
	here(x);
	COMMA(x, 0);
	x->latestxt = HERE(x);
}

void end_quotation(X* x) {
	CELL a = pop(x);
	COMMA(x, x->EXIT);
	STORE(x, a, HERE(x) - a - sizeof(CELL));
	x->latestxt = pop(x);
	x->level--;
	if (x->level == 0) x->state = 0;
}

/*
void start_string(X* x) {
	CELL a, l;
	COMMA(x, x->STRING);
	a = HERE(x);
	COMMA(x, 0);
	if (x->state == 0) { push(x, HERE(x)); }
	x->ipos++;
	while (x->ipos < x->ilen && CFETCH(x, x->ibuf + x->ipos) != '"') {
		CCOMMA(x, CFETCH(x, x->ibuf + x->ipos));
		x->ipos++;
	}
	x->ipos++;
	l = HERE(x) - a - sizeof(CELL);
	STORE(x, a, l);
	align(x);
	if (x->state == 0) { push(x, l); }
}
*/

void parse_name(X* x) {
	while (x->ipos < x->ilen && CFETCH(x, x->ibuf + x->ipos) < 33) x->ipos++;
	push(x, x->tok = x->ibuf + x->ipos);
	x->tlen = 0;
	while (x->ipos < x->ilen && CFETCH(x, x->ibuf + x->ipos) > 32) { x->ipos++; x->tlen++; }
	push(x, x->tlen);
	if (x->ipos < x->ilen) x->ipos++;
}

void type(X* x) {
	CELL l = pop(x); CELL s = pop(x); CELL i;
	for (i = 0; i < l; i++) printf("%c", CFETCH(x, s + i));
}

void interpret(X* x) {
	CELL w, n;
	char buf[15]; char* endptr;
	int r;
	while (x->ipos < x->ilen) {
		parse_name(x);
		trace(x, 2);
		if (pick(x, 0) == 0) { pop(x); pop(x); return; }
		find_name(x);
		w = pop(x);
		if (w != 0) {
			if (x->state == 0
			|| (x->state == 1 && HAS_FLAG(x, w, IMMEDIATE))
			|| (x->state == 2 && HAS_FLAG(x, w, INSTANTANEOUS))) {
				eval(x, GET_XT(x, w));
			} else {
				if (x->state == 1) compile(x, GET_XT(x, w));
				else /* x->state == 2 */ postpone(x, w);
			}
		} else {
			strncpy(buf, (char*)x->tok, x->tlen);
			buf[x->tlen] = 0;
			n = strtol(buf, &endptr, 10);	
			if (*endptr == '\0') {
				if (x->state == 0) push(x, n);
				else literal(x, n);
			} else {
				printf("%.*s ?\n", (int)x->tlen, (char*)x->tok);
			}
		}
		trace(x, 1);
	}
}

/*
void evaluate(X* x, char* s, int l) {
	CELL ibuf = x->ibuf, ipos = x->ipos, ilen = x->ilen;
	x->ibuf = (CELL)s; x->ilen = (CELL)l; x->ipos = 0;
	interpret(x);
	x->ibuf = ibuf; x->ipos = ipos; x->ilen = ilen;
}
*/

void evaluate(X* x, int sid) {
	CELL l = pop(x), a = pop(x);
	CELL previbuf = x->ibuf, previpos = x->ipos, previlen = x->ilen;
	CELL last_source_id = x->source_id;
	CELL prevtok = x->tok, prevtlen = x->tlen;

	x->source_id = sid;
	x->ibuf = a;
	x->ipos = 0;
	x->ilen = l;

	interpret(x);

	x->source_id = last_source_id;
	x->ibuf = previbuf;
	x->ipos = previpos;
	x->ilen = previlen;
	x->tok = prevtok;
	x->tlen = prevtlen;
}

void __evaluate(X* x, char* a, CELL l, CELL sid) {
	push(x, (CELL)a);
	push(x, l);
	evaluate(x, sid);
}

void _evaluate(X* x) { evaluate(x, -1); }

/* -- Basic primitives -- */

void _exit(X* x) { if (x->rp > 0) x->ip = rpop(x); else x->ip = -1; }
void lit(X* x) { push(x, opcode(x)); }
void quote(X* x) { CELL l = opcode(x); push(x, x->ip); x->ip += l; }
void string(X* x) { CELL l = opcode(x); push(x, x->ip); push(x, l); x->ip = ALIGNED(x, x->ip + l); }

void branch(X* x) { CELL l = opcode(x); x->ip += l; }
void zbranch(X* x) { CELL l = opcode(x); if (pop(x) == 0) x->ip += l - sizeof(CELL); }

void does(X* x) { STORE(x, GET_XT(x, x->latest) + 2*sizeof(CELL), pop(x)); }

void bye(X* x) { exit(0); }

/* void add(X* x) { int a = pop(x); int b = pop(x); push(x, b + a); } */
void sub(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, b - a); }
/*
void mul(X* x) { int a = pop(x); int b = pop(x); push(x, b * a); }
void _div(X* x) { int a = pop(x); int b = pop(x); push(x, b / a); }
void mod(X* x) { int a = pop(x); int b = pop(x); push(x, b % a); }
*/
void times_div_mod(X* x) { /* TODO */ }
void u_m_times(X* x) { /* TODO */ }
void u_m_slash_mod(X* x) { /* TODO */ }

void zlt(X* x) { CELL a = pop(x); push(x, a < 0 ? -1 : 0); }
/*
void lt(X* x) { int a = pop(x); int b = pop(x); push(x, b < a ? -1 : 0); }
void eq(X* x) { int a = pop(x); int b = pop(x); push(x, b == a ? -1 : 0); }
void gt(X* x) { int a = pop(x); int b = pop(x); push(x, b > a ? -1 : 0); }
*/

void and(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, b & a); }
void invert(X* x) { push(x, ~pop(x)); }

void two_slash(X* x) { push(x, pop(x) >> 1); }
/* TODO Check how to ensure arithmetic right shift always */
void rshift(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, b >> a); }
void lshift(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, b << a); }

void colon(X* x) { 
	CELL l, t; 
	parse_name(x); l = pop(x); t = pop(x);
	header(x, t, l); 
	set_hidden(x);
	x->state = 1;
	x->level = 1;
}

void semicolon(X* x) { 
	compile(x, x->EXIT); 
	x->state = 0; 
	x->level = 0;
	unset_hidden(x);
}

void get_ibuf(X* x) { push(x, x->ibuf); }
void set_ibuf(X* x) { x->ibuf = pop(x); }
void get_ipos(X* x) { push(x, x->ipos); }
void set_ipos(X* x) { x->ipos = pop(x); }
void get_ilen(X* x) { push(x, x->ilen); }
void set_ilen(X* x) { x->ilen = pop(x); }

void source(X* x) { push(x, x->ibuf); push(x, x->ilen); }
void source_exc(X* x) { x->ilen = pop(x); x->ibuf = pop(x); }
void source_id(X* x) { push(x, x->source_id); }

void refill(X* x) {
	switch (x->source_id) {
		case -1: push(x, 0); break;
		case 0:
			/* Accept new line from terminal
			push(x->ibuf); push(x, 80);
			accept(x);
			x->ilen = pop(x);
			x->ipos = 0;
			push(-1);
			*/
			break;
		default:
			/* Read new line from file */
			push(x, -1);
			break;
	}
}

void _postpone(X* x) { parse_name(x); find_name(x); postpone(x, pop(x)); }
void _noname(X* x) { here(x); x->latestxt = HERE(x); x->state = 1; }
void recurse(X* x) { compile(x, x->latestxt); }

void cells(X* x) { push(x, pop(x) * sizeof(CELL)); }
void chars(X* x) { /* do nothing */ }

void included(X* x) {
	FILE *fp;
	char linebuf[1024];

	CELL l = pop(x);
	CELL a = pop(x);
	char fname[255];
	int i;

	for (i = 0; i < l; i++) {
		fname[i] = CFETCH(x, a + i*sizeof(CHAR));
	}
	fname[l] = 0;

	fp = fopen(fname, "r");

	if (!fp) { /* Error */ }

	while (fgets(linebuf, 1024, fp)) {
		__evaluate(x, linebuf, strlen(linebuf), (CELL)fp);
	}
	
	fclose(fp);
}

void parse(X* x) {
	char c = (char)pop(x);
	int l = 0;
	push(x, x->ibuf + x->ipos);
	while (x->ipos < x->ilen && CFETCH(x, x->ibuf + x->ipos) != c) {
		x->ipos++;
		l++;
	}
	push(x, l);
	if (x->ipos < x->ilen) x->ipos++;
}

void start_string(X* x) {
	CELL a, l;
	CELL i, d;
	push(x, '"'); parse(x);
	l = pop(x);
	a = pop(x);
	if (x->state != 0) {
		COMMA(x, x->STRING);
		COMMA(x, l);
		for (i = 0; i < l; i++) CCOMMA(x, CFETCH(x, a + i));
		align(x);	
	} else {
		push(x, l);
		tallot(x);
		d = pop(x);
		push(x, d); push(x, l);
		for (i = 0; i < l; i++) CSTORE(x, d + i, CFETCH(x, a + i));
	}
}

void choose(X* x) {
	CELL f = pop(x);
	CELL t = pop(x);
	if (pop(x) != 0) eval(x, t);
	else eval(x, f);
}

void ipush(X* x) { 
	rpush(x, x->kx); 
	x->kx = x->jx;
	x->jx = x->ix;
	x->lx = 0;
}

void ipop(X* x) {
	x->lx = 0;
	x->ix = x->jx;
	x->jx = x->kx;
	x->kx = rpop(x);
}

void times(X* x) {
	CELL q = pop(x);
	CELL l = pop(x);
	ipush(x);
	for (x->ix = 0; x->ix < l && x->lx == 0; x->ix++) {
		eval(x, q);
	}
	ipop(x);
}

void _trace(X* x) {
	x->trace = pop(x);
}

void bootstrap(X* x) {
	x->level = 0;

	ALLOT(x, 2*sizeof(CELL));

	x->EXIT = primitive(x, "EXIT", 4, noname(x, &_exit));

	primitive(x, "NOOP", 4, noname(x, &noop));

	x->LIT = primitive(x, "LIT", 3, noname(x, &lit));
	/* colon(x, "FLIT", 4, noname(x, &flit)); */
	x->STRING = primitive(x, "STRING", 6, noname(x, &string));
	x->QUOTATION = primitive(x, "QUOTATION", 5, noname(x, &quote));

	primitive(x, "BRANCH", 6, noname(x, &branch));
	primitive(x, "?BRANCH", 7, noname(x, &zbranch));

	x->COMPILE = primitive(x, "COMPILE", 7, noname(x, &_compile));

	primitive(x, "DOES", 4, noname(x, &does));

	x->ORDER = noname(x, &default_order);

	constant(x, ">IN", 3, (CELL)&x->ipos);
	constant(x, "STATE", 5, (CELL)&x->state);
	constant(x, "BASE", 4, (CELL)&x->base);

	primitive(x, "DROP", 4, noname(x, &drop));
	primitive(x, "PICK", 4, noname(x, &_pick));
	primitive(x, "OVER", 4, noname(x, &over));
	primitive(x, "SWAP", 4, noname(x, &swap));

	primitive(x, ">R", 2, noname(x, &to_r));
	/* primitive(x, "R@", 2, noname(x, &r_fetch)); */
	primitive(x, "R>", 2, noname(x, &from_r));

	primitive(x, "DEPTH", 5, noname(x, &depth));

	/* primitive(x, "+", 1, noname(x, &add)); */
	primitive(x, "-", 1, noname(x, &sub));
	/*
	primitive(x, "*", 1, noname(x, &mul));
	primitive(x, "/", 1, noname(x, &_div));
	primitive(x, "MOD", 3, noname(x, &mod));
	*/
	primitive(x, "*/MOD", 5, noname(x, &times_div_mod));
	primitive(x, "UM*", 3, noname(x, &u_m_times));
	primitive(x, "UM/MOD", 6, noname(x, &u_m_slash_mod));

	primitive(x, "0<", 2, noname(x, &zlt));
	/*
	primitive(x, "<", 1, noname(x, &lt));
	primitive(x, "=", 1, noname(x, &eq));
	primitive(x, ">", 1, noname(x, &gt));
	*/

	primitive(x, "AND", 3, noname(x, &and));
	primitive(x, "INVERT", 6, noname(x, &invert));

	primitive(x, "2/", 2, noname(x, &two_slash));
	primitive(x, "RSHIFT", 6, noname(x, &rshift));
	primitive(x, "LSHIFT", 6, noname(x, &lshift));

	primitive(x, "LATEST@", 7, noname(x, &get_latest));
	primitive(x, "LATEST!", 7, noname(x, &set_latest));

	primitive(x, ":", 1, noname(x, &colon));
	primitive(x, ";", 1, noname(x, &semicolon)); set_immediate(x);
	primitive(x, "POSTPONE", 8, noname(x, &_postpone)); set_immediate(x);
	primitive(x, "IMMEDIATE", 9, noname(x, &set_immediate));
	primitive(x, ":NONAME", 7, noname(x, &_noname));
	primitive(x, "[:", 2, noname(x, &start_quotation)); set_immediate(x);
	primitive(x, ";]", 2, noname(x, &end_quotation)); set_immediate(x);
	primitive(x, "EXECUTE", 7, noname(x, &_execute));
	primitive(x, "RECURSE", 7, noname(x, &recurse)); set_immediate(x);
	primitive(x, "[[", 2, noname(x, &end_postpone_mode)); set_instantaneous(x);

	primitive(x, "EVALUATE", 8, noname(x, &_evaluate));

	primitive(x, "THROW", 5, noname(x, &_throw));
	primitive(x, "CATCH", 5, noname(x, &_catch));

	primitive(x, "@", 1, noname(x, &fetch));
	primitive(x, "!", 1, noname(x, &store));
	primitive(x, "C@", 2, noname(x, &cfetch));
	primitive(x, "C!", 2, noname(x, &cstore));
	primitive(x, "B@", 2, noname(x, &cfetch));
	primitive(x, "B!", 2, noname(x, &cstore));

	primitive(x, "HERE", 4, noname(x, &here));
	primitive(x, "ALLOT", 5, noname(x, &allot));
	/* primitive(x, "ALIGN", 5, noname(x, &align)); */
	primitive(x, "UNUSED", 6, noname(x, &unused));

	primitive(x, "THERE", 5, noname(x, &there));
	primitive(x, "TALLOT", 6, noname(x, &tallot));
	/* primitive(x, "TFREE", 5, noname(x, &tfree)); */

	primitive(x, "CELLS", 5, noname(x, &cells));
	primitive(x, "CHARS", 5, noname(x, &chars));

	primitive(x, "SOURCE", 6, noname(x, &source));
	primitive(x, "SOURCE!", 7, noname(x, &source_exc));

	primitive(x, "REFILL", 6, noname(x, &refill));
	primitive(x, "SOURCE-ID", 9, noname(x, &source_id));

	primitive(x, "PARSE-NAME", 10, noname(x, &parse_name));
	primitive(x, "PARSE", 5, noname(x, &parse));

	primitive(x, "FIND-NAME", 9, noname(x, &find_name));

	/*
	primitive(x, "IBUF", 4, noname(x, &get_ibuf));
	primitive(x, "IBUF!", 5, noname(x, &set_ibuf));
	primitive(x, "IPOS", 4, noname(x, &get_ipos));
	primitive(x, "IPOS!", 5, noname(x, &set_ipos));
	primitive(x, "ILEN", 4, noname(x, &get_ilen));
	primitive(x, "ILEN!", 5, noname(x, &set_ilen));

	primitive(x, "S\"", 2, noname(x, &start_string));
	primitive(x, "TYPE", 4, noname(x, &type));
	*/

	primitive(x, "INCLUDED", 8, noname(x, &included));

	primitive(x, "S\"", 2, noname(x, &start_string)); set_immediate(x);

	/* primitive(x, "DUMP", 4, noname(x, &dump)); */

	primitive(x, "CHOOSE", 6, noname(x, &choose));
	primitive(x, "TIMES", 5, noname(x, &times));

	primitive(x, "TRACE!", 6, noname(x, &_trace));

	primitive(x, "BYE", 3, noname(x, &bye));
}

int main() {
	X vm;
	CHAR m[65536];
	char ibuf[256];
	char *filename = "../../4th/sloth.4th";

	init(&vm);
	vm.mp = (CELL)m;
	vm.ms = 65536;
	vm.tp = vm.mp + vm.ms;
	bootstrap(&vm);

	while (fgets(ibuf, sizeof(ibuf), stdin) != 0) {
		__evaluate(&vm, ibuf, strlen(ibuf), -1);
	}
}
