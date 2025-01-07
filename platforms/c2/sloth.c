#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

/* ---------------------------------------------------- */
/* -- Virtual machine API ----------------------------- */
/* ---------------------------------------------------- */
/* This is the reference implementation of the SLOTH    */
/* Virtual Machine.                                     */
/* ---------------------------------------------------- */
/* This API defines how the virtual machine works.      */
/* It uses a table of primitives (C functions that can  */
/* be called from Forth).                               */
/* This implementation uses just the basic primitives   */
/* needed to have a working virtual machine, but it's   */
/* very easy to add new ones.                           */
/* ---------------------------------------------------- */

typedef int8_t CHAR;
typedef intptr_t CELL;

#define sCELL sizeof(CELL)
#define sCHAR sizeof(CHAR)

/* Predefined sizes */
/* TODO: they should be modifiable before context creation */
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

	/* These allow tracing of current token */
	CELL tok, tlen;
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
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void store(X* x, CELL a, CELL v) { *((CELL*)a) = v; }
CELL fetch(X* x, CELL a) { return *((CELL*)a); }
void cstore(X* x, CELL a, CHAR v) { *((CHAR*)a) = v; }
CHAR cfetch(X* x, CELL a) { return *((CHAR*)a); }

/*
HERE/ALLOT/ALIGN/ALIGNED work on relative address units.
*/
CELL here(X* x) { return x->h; }
void allot(X* x, CELL v) { x->h += v; }
void align(X* x) { x->h = (x->h + (sCELL - 1)) & ~(sCELL - 1); }
CELL aligned(X* x, CELL a) { return (a + (sCELL - 1)) & ~(sCELL - 1); }

/*
The next two macros allow transforming from relative to
absolute addresses.
*/
CELL to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL to_rel(X* x, CELL a) { return a - x->d; }

/* Inner interpreter */

CELL op(X* x) { 
	CELL o = fetch(x, to_abs(x, x->ip));
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

void trace(X* x, char * d, CELL l) {
	CELL i;
	if (l > 3) {
		printf("[[%s]] <%ld|%ld|%ld> [%ld] ", d, x->h, fetch(x, x->d), fetch(x, x->d + sCELL), x->sp);
		for (i = 0; i < x->sp; i++) {
			printf("%ld ", x->s[i]);
		}
		printf(": (%ld) [%ld] ", x->ip, x->rp);
		printf("<< %.*s (%ld)\n", (int)x->tlen, (char *)x->tok, x->tlen);
	}
}

/* TODO: Tracing should not be part of this implementation, */
/* as an inner interpreter with tracing can be implemented  */
/* in Forth, and speed is not a problem if we are tracing.   */
void inner(X* x) { 
	/* DEBUG */ CELL o; /* \DEBUG */
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) { 
		execute(x, op(x));
		/* trace(x, "INNER", 4); */
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
This primitives are implemented using only functions
and macros from the previous section thus making it
easier to port to other platforms.
TODO: Ensure there are enough functions to allow that.
*/

void _noop(X* x) { }
void _exit(X* x) { x->ip = (x->rp > 0) ? rpop(x) : -1; }
void _lit(X* x) { push(x, op(x)); }
void _rip(X* x) { push(x, to_abs(x, x->ip) + op(x) - sCELL); }
void _branch(X* x) { x->ip += op(x) - sCELL; }
void _zbranch(X* x) { x->ip += pop(x) == 0 ? (op(x) - sCELL) : sCELL; }

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
TODO To implement this words, check:
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
	x->p->p[3] = &_rip;
	x->p->p[4] = &_branch;
	x->p->p[5] = &_zbranch;

	x->p->p[6] = &_drop;
	x->p->p[7] = &_pick;
	x->p->p[8] = &_over;
	x->p->p[9] = &_swap;

	x->p->p[10] = &_r_to;
	x->p->p[11] = &_r_fetch;
	x->p->p[12] = &_r_from;

	x->p->p[13] = &_add;
	x->p->p[14] = &_sub;
	x->p->p[15] = &_mul;
	x->p->p[16] = &_div;
	x->p->p[17] = &_mod;

	x->p->p[18] = &_two_slash;
	x->p->p[19] = &_rshift;
	x->p->p[20] = &_lshift;

	x->p->p[21] = &_times_slash_mod;
	x->p->p[22] = &_um_times;
	x->p->p[23] = &_um_slash_mod;

	x->p->p[24] = &_lt;
	x->p->p[25] = &_eq;
	x->p->p[26] = &_gt;

	x->p->p[27] = &_and;
	x->p->p[28] = &_or;
	x->p->p[29] = &_xor;
	x->p->p[30] = &_invert;

	x->p->p[31] = &_fetch;
	x->p->p[32] = &_store;
	x->p->p[33] = &_cfetch;
	x->p->p[34] = &_cstore;

	x->p->p[35] = &_catch;
	x->p->p[36] = &_throw;

	x->p->last = 37;
}

/* Loading and saving already bootstrapped images */

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
/* This functions are not needed for execution of a     */
/* previously bootstrapped image. But they are          */
/* necessary to bootstrap a Forth system from source.   */
/* ---------------------------------------------------- */

/* Displacement of counted string buffer from here */
#define CBUF					64

/* These constants represent primitives needed in C code */
#define NOOP					-1	/* Not used in C code? */
#define EXIT					-2
#define LIT						-3
#define RIP						-4
#define BRANCH				-5  /* Not used in C code? */
#define ZBRANCH				-6  /* Not used in C code? */
#define COMPILE				-7

/* These constants represent variable positions needed */
/* in C code */
#define LATEST				0
#define STATE					sCELL
#define IBUF					2*sCELL
#define IPOS					3*sCELL
#define ILEN					4*sCELL
/* Not used in this impl. #define ORDER					5*sCELL */
#define SOURCE_ID			6*sCELL
#define DOES					7*sCELL

/* Word flags */
#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANT				4

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v) { store(x, to_abs(x, a), v); }
CELL get(X* x, CELL a) { return fetch(x, to_abs(x, a)); }

void cset(X* x, CELL a, CHAR v) { cstore(x, to_abs(x, a), v); }
CHAR cget(X* x, CELL a) { return cfetch(x, to_abs(x, a)); }

/* Compilation */

void comma(X* x, CELL v) { set(x, here(x), v); allot(x, sCELL); }
void ccomma(X* x, CHAR v) { set(x, here(x), v); allot(x, sCHAR); }

void compile(X* x, CELL xt) { comma(x, xt); }
void _compile(X* x) { compile(x, pop(x)); }
void literal(X* x, CELL n) { comma(x, LIT); comma(x, n); }

/* Word headers */

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags CHAR					@ NT + 3*sCELL */
/* Namelen CHAR				@ NT + 3*sCELL + sCHAR */
/* Name CHAR*namelen	@ NT + 3*sCELL + 2*sCHAR */

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = here(x); /* NT address */
	comma(x, get(x, LATEST)); /* Store link to latest */
	set(x, LATEST, w); /* Set NT as latest */
	comma(x, 0); /* Reserve space for XT */
	comma(x, 1); /* Store wordlist (default wordlist id: 1) */
	ccomma(x, 0); /* Flags (default flags: 0) */
	ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) ccomma(x, fetch(x, n + i)); /* Name */
	align(x); /* Align XT address */
	set(x, w + sCELL, here(x));
	return w;
}

CELL get_link(X* x, CELL w) { return get(x, w); }

CELL get_xt(X* x, CELL w) { return get(x, w + sCELL); }
void set_xt(X* x, CELL w, CELL xt) { set(x, w + sCELL, xt); }

CELL get_wordlist(X* x, CELL w) { return get(x, w + 2*sCELL); }

CHAR get_flags(X* x, CELL w) { return cget(x, w + 3*sCELL); }
CELL has_flag(X* x, CELL w, CELL v) { return get_flags(x, w) & v; }

CHAR get_namelen(X* x, CELL w) { return cget(x, w + 3*sCELL + sCHAR); }
CELL get_name_addr(X* x, CELL w) { return to_abs(x, w + 3*sCELL + 2*sCHAR); }

/* Setting flags */

void set_flag(X* x, CELL w, CHAR v) { cset(x, w + 3*sCELL, get_flags(x, w) | v); }
void unset_flag(X* x, CELL w, CHAR v) { cset(x, w + 3*sCELL, get_flags(x, w) & ~v); }

void set_hidden(X* x) { set_flag(x, get(x, LATEST), HIDDEN); }
void _immediate(X* x) { set_flag(x, get(x, LATEST), IMMEDIATE); }
/* void set_instant(X* x) { set_flag(x, get(x, LATEST), INSTANT); } */

void unset_hidden(X* x) { unset_flag(x, get(x, LATEST), HIDDEN); }

/* Finding words */

/* Helper function to compare a string and a word's name */
/* without case sensitivity. */
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

/* I would prefer to use FIND-NAME but its not yet */
/* standarized and there is no need to implement two */
/* words with very similar functionality. */
void _find(X* x) {
	/* Let's get the address and length from the counted */
	/* string on the stack. */
	CELL cstring = pop(x);
	CHAR l = cfetch(x, cstring);
	CELL a = cstring + sCHAR;
	/* Let's find the word, starting from LATEST */
	CELL w = get(x, LATEST);
	while (w != 0) {
		if (compare_without_case(x, w, a, l) && !has_flag(x, w, HIDDEN)) break;
		w = get_link(x, w);
	}
	if (w == 0) {
		push(x, cstring);
		push(x, 0);
	} else if (has_flag(x, w, IMMEDIATE)) {
		push(x, get_xt(x, w));
		push(x, 1);
	} else {
		push(x, get_xt(x, w));
		push(x, -1);
	}
}

void _to_name(X* x) {
	CELL xt = pop(x);
	CELL w = get(x, LATEST);
	while (w != 0) {
		if (get_xt(x, w) == xt) {
			push(x, w);
			return;
		}
		w = get_link(x, w);
	}
	push(x, 0);
}

/* Parsing */

void _source(X* x) {
	push(x, get(x, IBUF));
	push(x, get(x, ILEN));
}

void _in(X* x) {
	push(x, to_abs(x, IPOS));
}

/* I would prefer using PARSE-NAME but its not yet */
/* standarized and there's no need to implement two */
/* words with almost the same functionality. */
void _word(X* x) {
	/* The region to store WORD counted strings starts */
	/* at here + CBUF. */
	CHAR c = (CHAR)pop(x);
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	CELL start, end, i;
	/* First, ignore c until not c is found */
	/* The Forth Standard says that if the control character is */
	/* the space (hex 20) then control characters may be treated */
	/* as delimiters. */
	if (c == 32) {
		while (ipos < ilen && cfetch(x, ibuf + ipos) <= c) ipos++;
	} else {
		while (ipos < ilen && cfetch(x, ibuf + ipos) == c) ipos++;
	}
	start = ibuf + ipos;
	/* Next, continue parsing until c is found again */
	if (c == 32) {
		while (ipos < ilen && cfetch(x, ibuf + ipos) > c) ipos++;
	} else {
		while (ipos < ilen && cfetch(x, ibuf + ipos) != c) ipos++;
	}
	end = ibuf + ipos;	
	/* Now, copy it to the counted string buffer */
	cstore(x, to_abs(x, here(x) + CBUF), end - start);
	for (i = 0; i < (end - start); i++) {
		cstore(x, to_abs(x, here(x) + CBUF + sCHAR + i*sCHAR), cfetch(x, start + i*sCHAR));
	}
	push(x, to_abs(x, here(x) + CBUF));
	/* If we are not at the end of the input buffer, */
	/* skip c after the word, but its not part of the counted */
	/* string */
	if (ipos < ilen) ipos++;
	set(x, IPOS, ipos);
}

/* Postponing words */

void _postpone(X* x) {
	push(x, 32); _word(x);
	_find(x);
	if (pick(x, 0) == 0) { /* TODO THROW Undefined word */ }
	if (pop(x) == 1 /* IMMEDIATE WORD */) {
		compile(x, pop(x));
	} else /* NORMAL WORD */ {
		literal(x, pop(x));
		compile(x, COMPILE);
	}
}

/* Outer interpreter */

void _interpret(X* x) {
	CELL nt, res, n;
	char buf[15]; char *endptr;
	while (get(x, IPOS) < get(x, ILEN)) {
		push(x, 32); _word(x);
		x->tok = pick(x, 0) + sCHAR;
		x->tlen = cfetch(x, pick(x, 0));
		trace(x, "INTERPRET1", 3);
		if (x->tlen == 0) { _drop(x); return; }
		_find(x);
		trace(x, "INTERPRET2", 3);
		if ((res = pop(x)) != 0) {

			if (x->tlen == 2 && *(char*)x->tok == '?' && *(char*)(x->tok + 1) == ':') {
				/* Just for adding a breakpoint */
				n = 15;
			}

		
			if (get(x, STATE) == 0
			|| (get(x, STATE) == 1 && res == 1)) {
				/* TODO Left to do the INSTANT case, maybe its not a good  */
				/* idea as it goes away of Forth Standard */
				eval(x, pop(x));	
			} else {
				compile(x, pop(x));
			}
		} else {
			_drop(x);
			strncpy(buf, (char*)x->tok, x->tlen);
			buf[x->tlen] = 0;
			n = strtol(buf, &endptr, 10);	
			if (*endptr == '\0') {
				if (get(x, STATE) == 0) push(x, n);
				else literal(x, n);
			} else {
				/* TODO Word not found, should throw an exception? */
				printf("%.*s ?\n", (int)x->tlen, (char*)x->tok);
			}
		}
	}
}

/* Evaluate and include */

void evaluate(X* x, CELL sid) {
	CELL l = pop(x), a = pop(x);

	CELL previbuf = get(x, IBUF);
	CELL previpos = get(x, IPOS);
	CELL previlen = get(x, ILEN);

	CELL prevsourceid = get(x, SOURCE_ID);

	set(x, SOURCE_ID, sid);

	set(x, IBUF, a);
	set(x, IPOS, 0);
	set(x, ILEN, l);

	_interpret(x);

	set(x, SOURCE_ID, prevsourceid);

	set(x, IBUF, previbuf);
	set(x, IPOS, previpos);
	set(x, ILEN, previlen);
}

void evaluate_str(X* x, char* a, CELL l, CELL sid) {
	push(x, (CELL)a);
	push(x, l);
	evaluate(x, sid);
}

void _evaluate(X* x) { evaluate(x, -1); }

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
			evaluate_str(x, linebuf, strlen(linebuf), (CELL)f);
		}
	} else {
		/* TODO: Manage error */
	}

	fclose(f);
}

/* Other words needed to bootstrapping */

void _colon(X* x) {
	push(x, 32); _word(x);
	x->tok = pick(x, 0) + sCHAR;
	x->tlen = cfetch(x, pop(x));
	trace(x, "COLON1", 3);
	header(x, x->tok, x->tlen);
	set_hidden(x);
	set(x, STATE, 1);
	trace(x, "COLON2", 3);
}

void _semicolon(X* x) {
	compile(x, EXIT);
	set(x, STATE, 0);
	unset_hidden(x);
}

void _here(X* x) { push(x, to_abs(x, here(x))); }
void _allot(X* x) { allot(x, pop(x)); }

void _cells(X* x) { push(x, pop(x) * sCELL); }
void _chars(X* x) { push(x, pop(x) * sCHAR); }

void _recurse(X* x) {
	CELL xt = get_xt(x, get(x, LATEST));
	compile(x, xt);
}

/* DOES compiles a call in the new CREATEd word, replacing */
/* the first EXIT compiled on CREATE, to call to the part */
/* after the DOES> compiled in the defining word. */

void _does(X* x) {
	set(x, get_xt(x, get(x, LATEST)) + 2*sCELL, pop(x));
}

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
	push(x, 32); _word(x);
	x->tok = pick(x, 0) + sCHAR;
	x->tlen = cfetch(x, pop(x));
	header(x, x->tok, x->tlen);
	compile(x, RIP); compile(x, 4*sCELL); 
	compile(x, EXIT); compile(x, EXIT);
}

/* DOES> its an immediate word that compiles a literal */
/* to the relative address of the words after the DOES>, */
/* and then compiles a DOES and an EXIT instructions. */

void _does_gt(X* x) {
	literal(x, here(x) + 4*sCELL);
	compile(x, get(x, DOES)); compile(x, EXIT);
}

/* Words needed for testing, will not be needed after */

void _see(X* x) {
	CELL nt, ip, op;
	_to_name(x);
	nt = pop(x);
	printf("LINK: %ld\n", get_link(x, nt));
	printf("XT: %ld\n", get_xt(x, nt));
	printf("WORDLIST: %ld\n", get_wordlist(x, nt));
	printf("FLAGS: %d\n", (char)get_flags(x, nt));
	printf("NAME: %.*s\n", (int)get_namelen(x, nt), (char *)get_name_addr(x, nt));
	if (get_xt(x, nt) > 0) {
		ip = to_abs(x, get_xt(x, nt));
		while ((op = fetch(x, ip)) != EXIT) {
			printf("%ld ", op);
			ip += sCELL;
		}
		printf("%d\n", EXIT);
	}
}

void _see_colon(X* x) {
	push(x, 32); _word(x);
	_find(x); _drop(x);
	_see(x);
}

/* Helpers to add functions to the dictionary as words */

CELL primitive(X* x, F f) { 
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL code(X* x, char* name, CELL xt) {
	CELL w = header(x, (CELL)name, strlen(name));
	set_xt(x, w, xt);
	return xt; 
}

/* This is the one and only bootstrapping function */
void bootstrap(X* x) {
	/* Reserve space for required variables */

	comma(x, 0); /* LATEST */
	comma(x, 0); /* STATE */
	comma(x, 0); /* IBUF */
	comma(x, 0); /* IPOS */
	comma(x, 0); /* ILEN */
	comma(x, 0); /* SOURCE_ID */
	comma(x, 0); /* DOES */

	/* Insert primitives into the dictionary */

	code(x, "NOOP", primitive(x, &_noop));
	code(x, "EXIT", primitive(x, &_exit));
	code(x, "LIT", primitive(x, &_lit));
	code(x, "RIP", primitive(x, &_rip));
	code(x, "BRANCH", primitive(x, &_branch));
	code(x, "?BRANCH", primitive(x, &_zbranch));
	code(x, "COMPILE,", primitive(x, &_compile));

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

	/* Words required for bootstrapping */

	code(x, ":", primitive(x, &_colon));
	code(x, "WORD", primitive(x, &_word));
	code(x, "FIND", primitive(x, &_find));
	code(x, "SOURCE", primitive(x, &_source));
	code(x, ">IN", primitive(x, &_in));
	code(x, ";", primitive(x, &_semicolon)); _immediate(x);
	code(x, "IMMEDIATE", primitive(x, &_immediate));
	code(x, "POSTPONE", primitive(x, &_postpone)); _immediate(x);
	code(x, "HERE", primitive(x, &_here));
	code(x, "ALLOT", primitive(x, &_allot));
	code(x, "CELLS", primitive(x, &_cells));
	code(x, "CHARS", primitive(x, &_chars));
	code(x, "RECURSE", primitive(x, &_recurse)); _immediate(x);
	set(x, DOES, code(x, "DOES", primitive(x, &_does)));
	code(x, "CREATE", primitive(x, &_create));
	code(x, "DOES>", primitive(x, &_does_gt)); _immediate(x);

	/* Words for testing, will not be needed after */

	code(x, "SEE", primitive(x, &_see));
	code(x, "SEE:", primitive(x, &_see_colon));
}

/* ---------------------------------------------------- */
/* -- REPL -- only for testing purposes --------------- */
/* ---------------------------------------------------- */

void repl(X* x) {
	char buf[80];
	while (1) {
		printf("[IN] >> ");
		if (fgets(buf, 80, stdin) != 0) {
			evaluate_str(x, buf, strlen(buf), -1);	
			trace(x, "[OUT] ", 4);
		} else {
			/* TODO: Error */
		}
	}
}

/* ---------------------------------------------------- */
/* -- Main -------------------------------------------- */
/* ---------------------------------------------------- */

int main() {
	CELL w;
	char *filename = "ans.sloth";

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

	repl(x);

	free((void*)x->d);
	free(x->p);
	free(x);
}
