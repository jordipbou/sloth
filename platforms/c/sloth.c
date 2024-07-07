#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

/* --------------------------------------------------------------------------------------------- */
/* -- Virtual machine -------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

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
	CELL mp;
	F p[PSIZE];
	CELL pp;

	CELL latest, latestxt;
	CELL state;
	CELL ibuf, ipos, ilen;
	CELL tok, tlen;

	CELL EXIT;
	CELL LIT;
	CELL QUOTE;
	CELL STRING;
} X;

void init(X* x) { x->sp = 0; x->rp = 0; x->ip = -1; x->mp = 0; x->pp = 0; }

/* Data stack */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }

void place(X* x, CELL a, CELL v) { x->s[x->sp - a - 1] = v; }
int pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

void drop(X* x) { x->sp--; }
void swap(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); }
void _pick(X* x) { push(x, pick(x, pop(x))); }

/* Return stack */

void rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL rpop(X* x) { x->rp--; return x->r[x->rp]; }

CELL rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

void to_r(X* x) { rpush(x, pop(x)); }
void r_fetch(X* x) { push(x, rpick(x, 0)); }
void from_r(X* x) { push(x, rpop(x)); }

/* Memory */

#define STORE(x, a, v)	*((CELL*)(a)) = ((CELL)(v))
#define FETCH(x, a)			*((CELL*)(a))
#define CSTORE(x, a, v)	*((CHAR*)(a)) = ((CHAR)(v))
#define CFETCH(x, a)		*((CHAR*)(a))

void store(X* x) { CELL a = pop(x); STORE(x, a, pop(x)); }
void fetch(X* x) { push(x, FETCH(x, pop(x))); }

void cstore(X* x) { CELL a = pop(x); CSTORE(x, a, pop(x)); }
void cfetch(X* x) { push(x, CFETCH(x, pop(x))); }

#define HERE(x)			((x)->mp)
#define ALLOT(x, v)	((x)->mp += (v))
#define ALIGN(x)		HERE(x) = (HERE(x) + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1)
#define ALIGNED(x, a) (((a) + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1))

void here(X* x) { push(x, x->mp); }
void allot(X* x) { x->mp += pop(x); }
void align(X* x) { x->mp = (x->mp + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1); }

/* Inner interpreter */

void noop(X* x) { }

CELL opcode(X* x) { CELL o = FETCH(x, x->ip); x->ip += sizeof(CELL); return o; }

CELL noname(X* x, F f) { x->p[x->pp] = f; x->pp++; return 0 - x->pp; }

void do_prim(X* x, CELL p) { (x->p[-1 - p])(x); }
void call(X* x, CELL q) { if (x->ip >= 0) rpush(x, x->ip); x->ip = q; }

void execute(X* x, CELL q) { if (q < 0) do_prim(x, q); else call(x, q); }
void _execute(X* x) { execute(x, pop(x)); }

void inner(X* x) { CELL t = x->rp; while (t <= x->rp && x->ip >= 0) { execute(x, opcode(x)); } }

void eval(X* x, CELL q) { execute(x, q); inner(x); }

/* --------------------------------------------------------------------------------------------- */
/* -- Dictionary ------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

#define HIDDEN			1
#define COLON				2
#define IMMEDIATE		4

#define COMMA(x, v)			{ STORE((x), (x)->mp, (v)); (x)->mp += sizeof(CELL); }
#define CCOMMA(x, v)		{ CSTORE((x), (x)->mp, (v)); (x)->mp += sizeof(CHAR); }

void literal(X* x, CELL n) { COMMA(x, x->LIT); COMMA(x, n); }
void compile(X* x, CELL n) { COMMA(x, n); }

#define GET_LINK(x, w)			FETCH((x), (w))
#define SET_XT(x, w, v)			STORE((x), ((w) + sizeof(CELL)), (v))
#define GET_XT(x, w)				FETCH((x), ((w) + sizeof(CELL)))
#define SET_DT(x, w, v)			STORE((x), ((w) + 2*sizeof(CELL)), (v))
#define GET_DT(x, w)				FETCH((x), ((w) + 2*sizeof(CELL)))
#define SET_WL(x, w, v)			STORE((x), ((w) + 3*sizeof(CELL)))
#define GET_WL(x, w)				FETCH((x), ((w) + 3*sizeof(CELL)))
#define GET_FLAGS(x, w)			CFETCH((x), ((w) + 4*sizeof(CELL)))
#define SET_FLAGS(x, w, v)	CSTORE((x), ((w) + 4*sizeof(CELL)), GET_FLAGS((x), (w)) | (v))
#define UNSET_FLAGS(x, w, v)	CSTORE((x), ((w) + 4*sizeof(CELL)), GET_FLAGS((x), (w)) & ~(v))
#define GET_NAMELEN(x, w)		CFETCH((x), ((w) + 4*sizeof(CELL) + sizeof(CHAR)))
#define NAME_ADDR(x, w)			((w) + 4*sizeof(CELL) + 2*sizeof(CHAR))

#define HAS_FLAG(x, w, f)		((GET_FLAGS((x), (w)) & (f)) == f)

void set_hidden(X* x) { SET_FLAGS(x, x->latest, HIDDEN); }
void set_colon(X* x) { SET_FLAGS(x, x->latest, COLON); }
void set_immediate(X* x) { SET_FLAGS(x, x->latest, IMMEDIATE); }

void unset_hidden(X* x) { UNSET_FLAGS(x, x->latest, HIDDEN); }

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = HERE(x); COMMA(x, x->latest); x->latest = w;	/* link */
	COMMA(x, 0);	/* xt */
	COMMA(x, 0);	/* dt */
	COMMA(x, 1);	/* wordlist */
	CCOMMA(x, 0);	/* flags */
	CCOMMA(x, l);	/* namelen */
	for (i = 0; i < l; i++) CCOMMA(x, FETCH(x, n + i));
	align(x);
	SET_DT(x, w, HERE(x));
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

void find(X* x) {
	CELL l = pop(x), t = pop(x);
	CELL w = x->latest;
	while (w != 0) {
		if (compare_without_case(x, w, t, l) && !HAS_FLAG(x, w, HIDDEN)) break;
		w = GET_LINK(x, w);
	}
	push(x, w);
}

/* --------------------------------------------------------------------------------------------- */
/* -- Debugging -------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

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

void trace(X* x) {
	printf("%ld [%ld] ", x->mp, x->state);
	dump_stack(x);
	dump_rstack(x);
	dump_input(x);
	printf("\n");
}

char dump_char(X* x, CELL a) { 
	CHAR c = CFETCH(x, a); 
	if (c >= 32 && c <= 127) return c; else return '.'; 
}

void dump(X* x) {
	CELL i,  n = pop(x), a = pop(x);
	for (i = a; i < a + n; i += 8) {
		printf("%08X  %02X %02X %02X %02X  %02X %02X %02X %02X  %c%c%c%c %c%c%c%c\n", (unsigned int)i, 
			CFETCH(x, i), CFETCH(x, i + 1), CFETCH(x, i + 2), CFETCH(x, i + 3), 
			CFETCH(x, i + 4), CFETCH(x, i + 5), CFETCH(x, i + 6), CFETCH(x, i + 7), 
			dump_char(x, i), dump_char(x, i + 1), dump_char(x, i + 2), dump_char(x, i + 3), 
			dump_char(x, i + 4), dump_char(x, i + 5), dump_char(x, i + 6), dump_char(x, i + 7));

	}
}

/* --------------------------------------------------------------------------------------------- */
/* -- Bootstrapping ---------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

CELL primitive(X* x, char* n, CELL l, CELL xt) { 
	CELL w = header(x, (CELL)n, l);
	SET_XT(x, w, xt);
	SET_FLAGS(x, w, COLON);
	return GET_XT(x, w);
}

void start_quotation(X* x) {
	x->state++;
	COMMA(x, x->QUOTE);
	push(x, HERE(x));
	COMMA(x, 0);
	x->latestxt = HERE(x);
	if (x->state == 0) { push(x, HERE(x)); swap(x); }
}

void end_quotation(X* x) {
	CELL r = pop(x);
	COMMA(x, x->EXIT);
	STORE(x, r, HERE(x) - r);
}

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

void token(X* x) {
	while (x->ipos < x->ilen && CFETCH(x, x->ibuf + x->ipos) < 33) x->ipos++;
	push(x, x->tok = x->ibuf + x->ipos);
	x->tlen = 0;
	while (x->ipos < x->ilen && CFETCH(x, x->ibuf + x->ipos) > 32) { x->ipos++; x->tlen++; }
	push(x, x->tlen);
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
		token(x);
		if (pick(x, 0) == 0) { pop(x); pop(x); return; }
		find(x);
		w = pop(x);
		if (w != 0) {
			if (x->state == 0 || HAS_FLAG(x, w, IMMEDIATE)) {
				if (!HAS_FLAG(x, w, COLON)) push(x, GET_DT(x, w));
				if (GET_XT(x, w) != 0) eval(x, GET_XT(x, w));
			} else {
				if (!HAS_FLAG(x, w, COLON)) literal(x, GET_DT(x, w));
				if (GET_XT(x, w) != 0) compile(x, GET_XT(x, w));
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
		trace(x);
	}
}

void evaluate(X* x, char* s, int l) {
	CELL ibuf = x->ibuf, ipos = x->ipos, ilen = x->ilen;
	x->ibuf = (CELL)s; x->ilen = (CELL)l; x->ipos = 0;
	interpret(x);
	x->ibuf = ibuf; x->ipos = ipos; x->ilen = ilen;
}

/* -- Basic primitives -- */

void _exit(X* x) { if (x->rp > 0) x->ip = rpop(x); else x->ip = -1; }
void lit(X* x) { push(x, opcode(x)); }
void quote(X* x) { CELL l = opcode(x); push(x, x->ip); x->ip += l; }
void string(X* x) { CELL l = opcode(x); push(x, x->ip); push(x, l); x->ip = ALIGNED(x, x->ip + l); }

void bye(X* x) { exit(0); }

void add(X* x) { int a = pop(x); int b = pop(x); push(x, b + a); }
void sub(X* x) { int a = pop(x); int b = pop(x); push(x, b - a); }
void mul(X* x) { int a = pop(x); int b = pop(x); push(x, b * a); }
void _div(X* x) { int a = pop(x); int b = pop(x); push(x, b / a); }
void mod(X* x) { int a = pop(x); int b = pop(x); push(x, b % a); }

void lt(X* x) { int a = pop(x); int b = pop(x); push(x, b < a ? -1 : 0); }
void eq(X* x) { int a = pop(x); int b = pop(x); push(x, b == a ? -1 : 0); }
void gt(X* x) { int a = pop(x); int b = pop(x); push(x, b > a ? -1 : 0); }

void colon(X* x) { 
	CELL l, t; 
	token(x); l = pop(x); t = pop(x);
	header(x, t, l); 
	SET_XT(x, x->latest, GET_DT(x, x->latest));
	set_colon(x);
	set_hidden(x);
	x->state = 1 ;
}

void semicolon(X* x) { 
	compile(x, x->EXIT); 
	x->state = 0; 
	unset_hidden(x);
}

void get_ibuf(X* x) { push(x, x->ibuf); }
void set_ibuf(X* x) { x->ibuf = pop(x); }
void get_ipos(X* x) { push(x, x->ipos); }
void set_ipos(X* x) { x->ipos = pop(x); }
void get_ilen(X* x) { push(x, x->ilen); }
void set_ilen(X* x) { x->ilen = pop(x); }

void bootstrap(X* x) {
	primitive(x, "NOOP", 4, noname(x, &noop));

	x->EXIT = primitive(x, "EXIT", 4, noname(x, &_exit));

	x->LIT = primitive(x, "LIT", 3, noname(x, &lit));
	/* colon(x, "FLIT", 4, noname(x, &flit)); */
	x->QUOTE = primitive(x, "QUOTE", 5, noname(x, &quote));
	x->STRING = primitive(x, "STRING", 6, noname(x, &string));

	primitive(x, "DROP", 4, noname(x, &drop));
	primitive(x, "SWAP", 4, noname(x, &swap));
	primitive(x, "PICK", 4, noname(x, &_pick));

	primitive(x, ">R", 2, noname(x, &to_r));
	primitive(x, "R@", 2, noname(x, &r_fetch));
	primitive(x, "R>", 2, noname(x, &from_r));

	primitive(x, "@", 1, noname(x, &fetch));
	primitive(x, "!", 1, noname(x, &store));
	primitive(x, "C@", 2, noname(x, &cfetch));
	primitive(x, "C!", 2, noname(x, &cstore));

	primitive(x, "HERE", 4, noname(x, &here));
	primitive(x, "ALLOT", 5, noname(x, &allot));
	primitive(x, "ALIGN", 5, noname(x, &align));

	primitive(x, "EXECUTE", 7, noname(x, &_execute));

	primitive(x, "[", 1, noname(x, &start_quotation));
	primitive(x, "]", 1, noname(x, &end_quotation));

	primitive(x, "+", 1, noname(x, &add));
	primitive(x, "-", 1, noname(x, &sub));
	primitive(x, "*", 1, noname(x, &mul));
	primitive(x, "/", 1, noname(x, &_div));
	primitive(x, "MOD", 3, noname(x, &mod));

	primitive(x, "<", 1, noname(x, &lt));
	primitive(x, "=", 1, noname(x, &eq));
	primitive(x, ">", 1, noname(x, &gt));

	primitive(x, ":", 1, noname(x, &colon));
	primitive(x, ";", 1, noname(x, &semicolon)); set_immediate(x);

	primitive(x, "IBUF", 4, noname(x, &get_ibuf));
	primitive(x, "IBUF!", 5, noname(x, &set_ibuf));
	primitive(x, "IPOS", 4, noname(x, &get_ipos));
	primitive(x, "IPOS!", 5, noname(x, &set_ipos));
	primitive(x, "ILEN", 4, noname(x, &get_ilen));
	primitive(x, "ILEN!", 5, noname(x, &set_ilen));

	primitive(x, "S\"", 2, noname(x, &start_string));
	primitive(x, "TYPE", 4, noname(x, &type));

	primitive(x, "INCLUDED", 8, noname(x, &included));

	primitive(x, "DUMP", 4, noname(x, &dump));

	primitive(x, "BYE", 3, noname(x, &bye));
}

int main() {
	X vm;
	CHAR m[65536];
	char ibuf[256];

	init(&vm);
	vm.mp = (CELL)m;
	bootstrap(&vm);

	while (fgets(ibuf, sizeof(ibuf), stdin) != 0) {
		evaluate(&vm, ibuf, strlen(ibuf));
	}
}
