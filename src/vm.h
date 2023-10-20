/* TODO: Add some king of : ; basic, just to start creating */
/* TODO: Try to not add anything else */


/* TODO: Add literal to step */
/* TODO: Add create/colon/variable/constant and semicolon */

/* LEARN BY MAKING PFORTH FTH CODE WORK ON DODO */
/* - pForth is NOT case sensitive. I like that and will use that for now. */

/* TODO: Use key/emit for input/output and for dumping context */
/* TODO: Create tests based on key/emit and dump context */
/* TODO: Errors */
/* TODO: Variables, constants, create >does */
/* TODO: Quotations outside colon definitions */
/* TODO: Maybe dual words? I don't know if I need them */
/* TODO: Clean every line not needed */

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

typedef void V;
typedef char C;
typedef int8_t B;
typedef int16_t S;
typedef int32_t I;
typedef int64_t L;
typedef intptr_t P;

#define DSTACK_SIZE 64 
#define RSTACK_SIZE 64
#define ASTACK_SIZE 64

typedef struct _Context {
	P dp;
	P rp;
	P ip;
	P err;
	B* b;
	V (**x)(struct _Context*);
  P d[DSTACK_SIZE];
	P r[RSTACK_SIZE];
} X;

V inner(X*);

#define EXT(x, l) (x->x[l - 'A'])

#define PUSH(x, v) (x->d[x->dp++] = (P)(v))
#define POP(x) (x->d[--x->dp])
#define DROP(x) (--x->dp)

#define T(x) (x->d[x->dp - 1])
#define N(x) (x->d[x->dp - 2])
#define NN(x) (x->d[x->dp - 3])

#define L1(x, t, v) t v = (t)POP(x)
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)
#define L4(x, t1, v1, t2, v2, t3, v3, t4, v4) L3(x, t1, v1, t2, v2, t3, v3); L1(x, t4, v4)

#define DO(x, f) { f(x); if (x->err) return; }
#define ERR(x, c, e) if (c) { x->err = e; return; }

#define ERR_OK 0
#define ERR_DICT_OVERFLOW -8
#define ERR_UNDEFINED_WORD -13
#define ERR_ZERO_LEN_NAME -16
#define ERR_SYMBOL_ALLOCATION -256

typedef struct _Header {
	P sz;
	P l;
  P h;
	C* ibuf;
	P ipos;
	P ilen;
	B s;
} H;

#define SIZE(x) (((H*)(x->b))->sz)
#define LATEST(x) (((H*)(x->b))->l)
#define HERE(x) (((H*)(x->b))->h)
#define IBUF(x) (((H*)(x->b))->ibuf)
#define IPOS(x) (((H*)(x->b))->ipos)
#define ILEN(x) (((H*)(x->b))->ilen)
#define STATE(x) (((H*)(x->b))->s)

#define REL_TO_ABS(x, a) ((V*)((P)x->b + (P)a))
#define ABS_TO_REL(x, a) ((P)a - (P)x->b)

typedef struct _Word {
	P p;
	P c;
	B f;
	B l;
	C n[1];
} W;

#define VARIABLE 1
#define HIDDEN 2
#define IMMEDIATE 4
#define CONSTANT 8

V dup(X* x) { PUSH(x, T(x)); }
V over(X* x) { PUSH(x, N(x)); }
V swap(X* x) { P t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { P t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }
V nip(X* x) { N(x) = T(x); DROP(x); }

V to_r(X* x) { x->r[x->rp++] = x->d[--x->dp]; }
V from_r(X* x) { x->d[x->dp++] = x->r[--x->rp]; }

V add(X* x) { N(x) = N(x) + T(x); DROP(x); }
V sub(X* x) { N(x) = N(x) - T(x); DROP(x); }
V mul(X* x) { N(x) = N(x) * T(x); DROP(x); }
V division(X* x) { N(x) = N(x) / T(x); DROP(x); }
V mod(X* x) { N(x) = N(x) % T(x); DROP(x); }

V and(X* x) { N(x) = N(x) & T(x); DROP(x); }
V or(X* x) { N(x) = N(x) | T(x); DROP(x); }
V xor(X* x) { N(x) = N(x) ^ T(x); DROP(x); }
V not(X* x) { T(x) = !T(x); }
V inverse(X* x) { T(x) = ~T(x); }

V lt(X* x) { N(x) = (N(x) < T(x)) ? -1 : 0; DROP(x); }
V eq(X* x) { N(x) = (N(x) == T(x)) ? -1 : 0; DROP(x); }
V gt(X* x) { N(x) = (N(x) > T(x)) ? -1 : 0; DROP(x); }

V pstore(X* x) { L2(x, P*, a, P, b); *a = b; }
V pfetch(X* x) { L1(x, P*, a); PUSH(x, *a); }
V bstore(X* x) { L2(x, B*, a, B, b); *a = b; }
V bfetch(X* x) { L1(x, B*, a); PUSH(x, *a); }

V bcompile(X* x) { L1(x, B, b); x->b[HERE(x)++] = b; }

#define TAIL(x) (x->ip >= SIZE(x) || x->b[x->ip] == ']' || x->b[x->ip] == '}')
V call(X* x) { L1(x, P, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = SIZE(x); }
V jump(X* x) { L1(x, P, d); x->ip += d - 1; }
V zjump(X* x) { L2(x, P, d, P, b); if (!b) x->ip += d - 1; }
V eval(X* x, P q) { PUSH(x, q); call(x); inner(x); }

V parse_spaces(X* x) { while (IPOS(x) < ILEN(x) && isspace(IBUF(x)[IPOS(x)])) IPOS(x)++; }
V parse(X* x) { L1(x, C, c); while (IPOS(x) < ILEN(x) && IBUF(x)[IPOS(x)] != c) IPOS(x)++; }
V parse_non_spaces(X* x) { while (IPOS(x) < ILEN(x) && !isspace(IBUF(x)[IPOS(x)])) IPOS(x)++; }
V parse_name(X* x) {
	parse_spaces(x);
	PUSH(x, &IBUF(x)[IPOS(x)]);
	parse_non_spaces(x);
	PUSH(x, ((P)(&IBUF(x)[IPOS(x)])) - T(x));
}

V create(X* x) {
	parse_name(x);
	ERR(x, T(x) == 0, ERR_ZERO_LEN_NAME);
	{
		L2(x, P, l, C*, n);
		/* Check if there's enough space */
		W* w = REL_TO_ABS(x, HERE(x));
		w->p = LATEST(x);
		LATEST(x) = HERE(x);
		HERE(x) += sizeof(W) + l;
		w->c = HERE(x);
		w->f = HIDDEN;
		w->l = l;
		strncpy(w->n, n, l);
		w->n[l] = 0;
	}
}

V reveal(X* x) {
	if (LATEST(x)) {
		W* l = REL_TO_ABS(x, LATEST(x));
		l->f &= ~HIDDEN;
	}
}

#define PEEK(x) (x->b[x->ip])
#define TOKEN(x) (x->b[x->ip++])

V step(X* x) {
	printf("STEP [%c]\n", PEEK(x));
	switch (PEEK(x)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
		EXT(x, PEEK(x))(x);
		break;
	default:
		switch (TOKEN(x)) {
		case 'e': EXT(x, 'E')(x); break;
		case 'k': EXT(x, 'K')(x); break;

		case '_': DROP(x); break;
		case 'd': dup(x); break;
		case 'o': over(x); break;
		case 's': swap(x); break;
		case 'r': rot(x); break;
		case 'n': nip(x); break;

		case '+': add(x); break;
		case '-': sub(x); break;
		case '*': mul(x); break;
		case '/': division(x); break;
		case '%': mod(x); break;

		case '&': and(x); break;
		case '|': or(x); break;
		case '^': xor(x); break;
		case '!': not(x); break;
		case '~': inverse(x); break;

		case '<': lt(x); break;
		case '=': eq(x); break;
		case '>': gt(x); break;

		case ',': pstore(x); break;
		case '.': pfetch(x); break;
		case ';': bstore(x); break;
		case ':': bfetch(x); break;

		case '$': PUSH(x, x->b[x->ip++]); bcompile(x); break;

		case 'x': call(x); break;
		case 'j': jump(x); break;
		case 'z': zjump(x); break;
		/*case '[': quot(x); break;*/
		case ']': ret(x); break;

		case 'b': PUSH(x, &x->b); break;
		case 'c': PUSH(x, sizeof(P)); break;

		case '0': PUSH(x, 0); break;
		case '1': PUSH(x, 1); break;
		case '#': PUSH(x, *((B*)(x->b + x->ip))); x->ip += 1; break;
		case '2': PUSH(x, *((S*)(x->b + x->ip))); x->ip += 2; break;
		case '4': PUSH(x, *((I*)(x->b + x->ip))); x->ip += 4; break;
		case '8': PUSH(x, *((L*)(x->b + x->ip))); x->ip += 8; break;

		case 'w': create(x); break;
		}
	}
}

V inner(X* x) { 
	P rp = x->rp; 
	while(x->rp >= rp && x->ip < SIZE(x) && !x->err) { 
		step(x); 
		/* Manage errors */
	} 
}

V literal(X* x) {
	L1(x, P, n);
	if (n == 0) { x->b[HERE(x)] = '0'; HERE(x) += 1; }
	else if (n == 1) { x->b[HERE(x)] = '1'; HERE(x) += 1; }
	else if (n >= INT8_MIN && n <= INT8_MAX) { }
	else if (n >= INT16_MIN && n <= INT16_MAX) { /* compile '2' short */ }
	else if (n >= INT32_MIN && n <= INT32_MAX) { /* compile '4' int */ }
	else { /* compile '8' long */ }
}

V find_name(X* x) {
	L2(x, P, l, B*, t);
	P wp = LATEST(x);
	while (wp) {
		W* w = REL_TO_ABS(x, wp);
		if (w->l == l) {
			P i;
			P f = 1;
			for (i = 0; i < l; i++) {
				if (t[i] >= 97 && t[i] <= 122) {
					if (t[i] != w->n[i] && t[i] != (w->n[i] + 32)) { f = 0; break; }
				} else if (t[i] >= 65 && t[i] <= 90) {
					if (t[i] != w->n[i] && t[i] != (w->n[i] - 32)) { f = 0; break; }
				} else {
					if (t[i] != w->n[i]) { f = 0; break; }
				}
			}
			if (f) break;
		}
		wp = w->p;
	}
	PUSH(x, t);
	PUSH(x, l);
	PUSH(x, wp);
}

V evaluate(X* x, C* s) {
	IBUF(x) = s;
	IPOS(x) = 0;
	ILEN(x) = strlen(s);
	while (IPOS(x) < ILEN(x)) {
		parse_name(x);
		if (T(x) == 0) { DROP(x); DROP(x); return; }
		find_name(x);
		if (T(x)) {
			L3(x, W*, w, P, _, C*, __);
		} else {
			L3(x, W*, _, P, l, C*, t);
			if (t[0] == '\\') {
				int i;
				printf("Evaluating assembler [%.*s]\n", (int)l, t);
				for (i = 1; i < l; i++) { x->b[SIZE(x) - l + i] = t[i]; }
				eval(x, SIZE(x) - l + 1);
			} else {
				char * end;
				int n = strtol(t, &end, 10);
				if ((n == 0 && end == t) || end < (t + l)) {
					PUSH(x, t);
					PUSH(x, l);
					x->err = ERR_UNDEFINED_WORD;
					return;
				}
				PUSH(x, n);
				if (STATE(x)) literal(x);
			}
		}
	}
}

X* init() {
	X* x = malloc(sizeof(X));
	x->b = malloc(65536);
	printf("x->b %ld\n", (P)&x->b);
	SIZE(x) = 65536;
	LATEST(x) = 0;
	HERE(x) = sizeof(H);
	printf("Size of block: %ld\n", SIZE(x));
	printf("Latest: %ld Absolute latest: %ld\n", (P)LATEST(x), (P)REL_TO_ABS(x, LATEST(x)));
	return x;
}

/*

#define VARIABLE 1
#define HIDDEN 2
#define IMMEDIATE 4
#define CONSTANT 8

typedef struct _Symbol {
	struct _Symbol* p;
	C c;
	B f;
	B nl;
	B n[1];
} S;

typedef struct _Context X;
typedef V (*F)(X*);

#define MEM_SIZE 65536

typedef struct _Memory {
	C s;
	S* l;
	C h;
	F x[26];
	B c;
	B* ibuf;
	C ilen;
	C ipos;
	B cstr[255];
	B pad[255];
	C base;
	B d[1];
} M;

#define DSTACK_SIZE 256
#define RSTACK_SIZE 256

#define EXT(x, l) (x->m->x[l - 'A'])

#define PUSH(x, v) (x->d[x->dp++] = (C)v)
#define POP(x) (x->d[--x->dp])
#define DROP(x) (--x->dp)

#define T(x) (x->d[x->dp - 1])
#define N(x) (x->d[x->dp - 2])
#define NN(x) (x->d[x->dp - 3])

#define L1(x, t, v) t v = (t)POP(x)
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)
#define L4(x, t1, v1, t2, v2, t3, v3, t4, v4) L3(x, t1, v1, t2, v2, t3, v3); L1(x, t4, v4)

#define DO(x, f) { f(x); if (x->err) return; }
#define ERR(x, c, e) if (c) { x->err = e; return; }

#define ERR_OK 0
#define ERR_DICT_OVERFLOW -8
#define ERR_UNDEFINED_WORD -13
#define ERR_ZERO_LEN_NAME -16
#define ERR_SYMBOL_ALLOCATION -256

V parse(X* x) {
  L1(x, C, v);
  while (x->m->ipos < x->m->ilen && x->m->ibuf[x->m->ipos++] != v) {}
}

V parse_name(X* x) {
	while (x->m->ipos < x->m->ilen && isspace(x->m->ibuf[x->m->ipos])) x->m->ipos++;
	PUSH(x, &x->m->ibuf[x->m->ipos]);
	while (x->m->ipos < x->m->ilen && !isspace(x->m->ibuf[x->m->ipos])) x->m->ipos++;
	PUSH(x, &x->m->ibuf[x->m->ipos] - T(x));
}

V to_upper(X* x) {
	L2(x, C, l, B*, t);
	C i;
	for (i = 0; i < l; i++) {
		if (t[i] >= 97 && t[i] <= 122) t[i] = t[i] - 32;
	}
	PUSH(x, t);
	PUSH(x, l);
}

V find_name(X* x) {
	L2(x, C, l, B*, t);
	S* s = x->m->l;
	while (s) {
		if (s->nl == l) {
			C i;
			C f = 1;
			for (i = 0; i < l; i++) {
				if (t[i] >= 97 && t[i] <= 122) {
					if (t[i] != s->n[i] && t[i] != (s->n[i] + 32)) { f = 0; break; }
				} else if (t[i] >= 65 && t[i] <= 90) {
					if (t[i] != s->n[i] && t[i] != (s->n[i] - 32)) { f = 0; break; }
				} else {
					if (t[i] != s->n[i]) { f = 0; break; }
				}
			}
			if (f) break;
		}
		s = s->p;
	}
	PUSH(x, t);
	PUSH(x, l);
	PUSH(x, s);
}

V literal(X* x) {
	L1(x, C, v);
	x->m->d[x->m->h++] = '#';
	*((C*)(&x->m->d[x->m->h])) = v;
	x->m->h += sizeof(C);
}

C code_length(X* x, B* c) {
	int t = 1;
	C n = 0;
	while (t) {
		if (*c == '[') t++;
		if (*c == ']') t--;
		c++;
		n++;
	}
	return n - 1;
}

C code_length(X* x, S* s) {
	int t = 1;
	B* c = &x->m->d[s->c];
	C n = 0;
	while (t) {
		if (*c == '[') t++;
		if (*c == ']') t--;
		c++;
		n++;
	}
	return n - 1;
}

V compile(X* x) {
	L1(x, C, c);
	C cl = code_length(x, &x->m->d[c]);
	if (cl < sizeof(C) + 2) {
		C i;
		for (i = 0; i < cl; i++) 
			x->m->d[x->m->h++] = x->m->d[c + i];
	} else {
		PUSH(x, c);
		literal(x);
		x->m->d[x->m->h++] = 'x';
	}
}

V compile(X* x) {
	L1(x, S*, s);
	C cl = code_length(x, s);
	C i;
	if (cl < sizeof(C) + 2) {
		for (i = 0; i < cl; i++) x->m->d[x->m->h++] = x->m->d[s->c + i];
	} else {
		PUSH(x, s->c);
		literal(x);
		x->m->d[x->m->h++] = 'x';
	}
}

V create(X* x) {
  DO(x, parse_name);
  ERR(x, T(x) == 0, ERR_ZERO_LEN_NAME);
  {
    L2(x, C, l, B*, n);
    S* s = malloc(sizeof(S) + l);
    ERR(x, !s, ERR_SYMBOL_ALLOCATION);
    s->p = x->m->l;
    x->m->l = s;
    s->f = VARIABLE;
    s->c = x->m->h;
    s->nl = l;
    strncpy((char*)s->n, (char*)n, l);
    s->n[l] = 0;
  }
}

V does(X* x) { x->m->l->c = x->m->h; }
V end(X* x) { x->m->d[x->m->h++] = ']'; }

V hide(X* x) { x->m->l->f = HIDDEN; }
V reveal(X* x) { x->m->l->f &= ~HIDDEN; }
V immediate(X* x) { x->m->l->f |= IMMEDIATE; }
V variable(X* x) { x->m->l->f |= VARIABLE; }

V constant(X* x) { L1(x, C, v); create(x); x->m->l->c = v; x->m->l->f = CONSTANT; }

V compilation(X* x) { x->m->c = -1; }
V interpretation(X* x) { x->m->c = 0; }

V allot(X* x) { L1(x, C, v); ERR(x, x->m->h + v >= x->m->s, ERR_DICT_OVERFLOW); x->m->h += v; }
V here(X* x) { PUSH(x, &x->m->d[x->m->h]); }

V reset_context(X* x) {
	x->dp = 0;
	x->rp = 0;
	x->ip = MEM_SIZE;
	x->err = 0;
}


V variable(X* x) {
  DO(x, create);
  x->m->l->f = VARIABLE;
}

V constant(X* x) {
  L1(x, C, v);
  DO(x, create);
  x->m->l->f = CONSTANT;
  x->m->l->c = v;
}
  
V colon(X* x) {
  DO(x, create);
  x->m->l->f = HIDDEN;
  x->m->c = 1;
}

V semicolon(X* x) {
  x->m->d[x->m->h++] = ']';
  x->m->d[x->m->h++] = 0;
  x->m->c = 0;	
  x->m->l->f &= ~HIDDEN;
  printf("Created %s\n", x->m->l->n);
  printf("Code %s\n", x->m->d + x->m->l->c);
  printf("Flags %ld\n", x->m->l->f);
}
  
V immediate(X* x) {	
  x->m->l->f |= IMMEDIATE;
}

V inner(X* x);

#define TAIL(x) (x->ip >= MEM_SIZE || x->m->d[x->ip] == ']' || x->m->d[x->ip] == '}')
V call(X* x) { L1(x, C, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = MEM_SIZE; }
V jump(X* x) { L1(x, C, d); x->ip += d - 1; }
V zjump(X* x) { L2(x, C, d, C, b); if (!b) x->ip += d - 1; }
V eval(X* x, C q) { PUSH(x, q); call(x); inner(x); }

V bfetch(X* x) { L1(x, B*, a); PUSH(x, *a); }
V bstore(X* x) { L2(x, B*, a, B, v); *a = v; }
V cfetch(X* x) { L1(x, C*, a); PUSH(x, *a); }
V cstore(X* x) { L2(x, C*, a, C, v); *a = v; }

V bcompile(X* x) { L1(x, B, v); x->m->d[x->m->h] = v; x->m->h += 1; }
V ccompile(X* x) { L1(x, C, v); *((C*)&x->m->d[x->m->h]) = v, x->m->h += sizeof(C); }

V dup(X* x) { PUSH(x, T(x)); }
V over(X* x) { PUSH(x, N(x)); }
V swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { C t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }
V nip(X* x) { N(x) = T(x); DROP(x); }

V to_r(X* x) { x->r[x->rp++] = x->d[--x->dp]; }
V from_r(X* x) { x->d[x->dp++] = x->r[--x->rp]; }

#define OP2(x, op) N(x) = N(x) op T(x); DROP(x)
V add(X* x) { OP2(x, +); }
V sub(X* x) { OP2(x, -); }
V mul(X* x) { OP2(x, *); }
V division(X* x) { OP2(x, /); }
V mod(X* x) { OP2(x, %); }

V and(X* x) { OP2(x, &); }
V or(X* x) { OP2(x, |);}
V xor(X* x) { OP2(x, ^); }
V not(X* x) { T(x) = !T(x); }
V inverse(X* x) { T(x) = ~T(x); }

V lt(X* x) { N(x) = (N(x) < T(x)) ? -1 : 0; x->dp--; }
V eq(X* x) { N(x) = (N(x) == T(x)) ? -1 : 0; x->dp--; }
V gt(X* x) { N(x) = (N(x) > T(x)) ? -1 : 0; x->dp--; }

#define PEEK(x) (x->m->d[x->ip])
#define TOKEN(x) (x->m->d[x->ip++])

V step(X* x) {
	switch (PEEK(x)) {
  case 'A': case 'B': case 'C':	case 'D': 
  case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N': 
	case 'O': case 'P': case 'Q': case 'R': 
  case 'S': case 'T': case 'U': case 'V': 
  case 'W': case 'X': case 'Y': case 'Z':
    EXT(x, TOKEN(x))(x);
    break;
	default:
		switch (TOKEN(x)) {
		case 'k': EXT(x, 'K')(x); break;
		case 'e': EXT(x, 'E')(x); break;

		case '0': PUSH(x, 0); break;
		case '1': PUSH(x, 1); break;
    case '#': PUSH(x, *((C*)(&x->m->d[x->ip]))); x->ip += 8; break;
    case '$': x->m->d[x->m->h++] = TOKEN(x); break;

    case '\\': parse(x); break;

    case '{': colon(x); break;
    case '}': semicolon(x); break;
    case 'v': variable(x); break;
    case 'c': constant(x); break;
      
		case 'x': call(x); break;
		case ']': case '@': ret(x); break;
		case 'j': jump(x); break;
		case 'z': zjump(x); break;

    case 'i': immediate(x); break;

		case ':': bfetch(x); break;
		case ';': bstore(x); break;
		case '.': cfetch(x); break;
		case ',': cstore(x); break;

		case '\'': bcompile(x); break;
		case '"': ccompile(x); break;

	  case '_': DROP(x); break;
	  case 's': swap(x); break;
	  case 'o': over(x); break;
	  case 'd': dup(x); break;
	  case 'r': rot(x); break;
		case 'n': nip(x); break;

		case '(': to_r(x); break;
		case ')': from_r(x); break;

	  case '+': add(x); break;
	  case '-': sub(x); break;
	  case '*': mul(x); break;
	  case '/': division(x); break;
	  case '%': mod(x); break;

	  case '&': and(x); break;
	  case '|': or(x); break;
	  case '!': not(x); break;
	  case '^': xor(x); break;
	  case '~': inverse(x); break;

	  case '<': lt(x); break;
	  case '=': eq(x); break;
	  case '>': gt(x); break;

		case 'c': PUSH(x, sizeof(C)); break;
		case 'l': PUSH(x, &x->m->l); break;

		case '{': compilation(x); break;
		case '}': interpretation(x); break;
		case 'w': create(x); break;
		case 'v': hide(x); break;
		case 'u': reveal(x); break;
		case 'i': immediate(x); break;

		case 'a': allot(x); break;
		case 'h': here(x); break;

		case 'q': x->err = -1; break;
		case 'l': PUSH(x, &x->m->l); break;
		case '{': create(x); hide(x); compilation(x); break;
		case '}': end(x); interpretation(x); unhide(x); break;
		case 'c': constant(x); break;
		case 'v': variable(x); break;
		case 'w':
			switch (TOKEN(x)) {
			case 'c': create(x); break;
			case 'd': does(x); break;
			case 'h': x->m->l->f |= HIDDEN; break;
			case 'i': x->m->l->f |= IMMEDIATE; break;
			case 'l': PUSH(x, &x->m->l); break;
			case 'r': x->m->c = 0; break;
			case 's': x->m->c = -1; break;
			case 'u': x->m->l->f &= ~HIDDEN; break;
			}
			break;
		case 'h': PUSH(x, &x->m->d[x->m->h]); break;

		case 'c': PUSH(x, sizeof(C)); break;
		}
	}
}

V inner(X* x) { 
	C rp = x->rp; 
	while(x->rp >= rp && x->ip < MEM_SIZE && !x->err) { 
		step(x); 
	} 
}

V evaluate(X* x, B* s) {
	x->m->ibuf = s;	
	x->m->ilen = strlen((const char *)s);
	x->m->ipos = 0;
	while (x->m->ipos < x->m->ilen && !x->err) {
		DO(x, parse_name);
    if (T(x) == 0) { DROP(x); DROP(x); return; }
		else {
			DO(x, find_name);
			if (T(x)) {
				L3(x, S*, s, C, _, B*, __);
				C i;
				for (i = 0; i < x->dp; i++) printf("%ld ", x->d[i]);
				printf("\n");
				PUSH(x, s->c);
				if (x->m->c && (s->f & IMMEDIATE) != IMMEDIATE) { compile(x); }
				else if ((s->f & VARIABLE) == VARIABLE) { T(x) += (C)x->m->d; }
				else { call(x); inner(x); } 
			} else {
			  L3(x, S*, _, C, l, B*, t);
				if (t[0] == '\\') {
					int i;
					for (i = 1; i < l; i++) { x->m->d[MEM_SIZE - l + i] = t[i]; }
					eval(x, MEM_SIZE - l + 1);
				} else {
					char * end;
					int n = strtol(t, &end, 10);
					if ((n == 0 && end == t) || end < (t + l)) {
						PUSH(x, t);
						PUSH(x, l);
						x->err = ERR_UNDEFINED_WORD;
						return;
					}
					PUSH(x, n);
					if (x->m->c) literal(x);
				}
			}
		}
	}
}

X* init() {
	X* x = malloc(sizeof(X));
	if (!x) return 0;
	x->m = malloc(sizeof(M) + MEM_SIZE);
	if (!x->m) { free(x); return 0; }
	x->m->s = MEM_SIZE;
	x->m->l = 0;
	reset_context(x);

	evaluate(x, "\\wv{ : \\$w$v${ \\$]}u");
	evaluate(x, ": ; \\$}$u$$$] \\$]}ui");
	evaluate(x, ": dup \\$d ;");

	return x;
}

V parse_spaces(X* x) {
	while (x->m->ipos < x->m->ilen && isspace(x->m->ibuf[x->m->ipos])) x->m->ipos++;
}
V parse(X* x) {
	L1(x, B, c);
	PUSH(x, &x->m->ibuf[x->m->ipos]);
	while (x->m->ipos < x->m->ilen && x->m->ibuf[x->m->ipos] != c) x->m->ipos++;
	x->m->ipos++;
	PUSH(x, ((C)&x->m->ibuf[x->m->ipos]) - T(x));
}
V word(X* x) {
	parse_spaces(x);
	parse(x);
	{
		L2(x, C, l, B*, t);
		x->m->cstr[0] = (uint8_t)l;
		for(;l > 0; l--) x->m->cstr[l - 1] = t[l - 1];
	}
	PUSH(x, x->m->cstr);
}

V pick(X* x) {
	L1(x, C, n);
	PUSH(x, x->d[x->dp - 1 - n]);
}

V d_minus(X* x) {
	L4(x, C, bh, C, bl, C, ah, C, al);
	C sh;
	C sl;
	sh = 0;
	sl = al - bl;
	if (al < bl) sh = 1;
	sh = ah - bh - sh;
	PUSH(x, sl);
	PUSH(x, sh);
}

V type(X* x) {
	L2(x, C, l, B*, s);
	printf("%.*s", (int)l, s);
}

V cmove(X* x) {
	L3(x, C, u, B*, dst, B*, src);
	C i;
	for (i = 0; i < u; i++) {
		dst[i] = src[i];
	}
}

V add_store(X* x) {
	L2(x, C*, a, C, v);
	*a += v;
}

V wstore(X* x) {
	L2(x, W*, a, W, v);
	*a = v;
}

V pForth_ext(X* x) {
	switch (TOKEN(x)) {
	case ':': create(x); hide(x); compilation(x); break;
	case ';': end(x); interpretation(x); reveal(x); break;
	case '#': literal(x); break;
	case '+': add_store(x); break;
	case 'b': PUSH(x, &x->m->base); break;
	case 'c': T(x) *= sizeof(C); break;
	case 'd':
		switch (TOKEN(x)) {
		case '-': d_minus(x); break;
		}
		break;
	case 'e': PUSH(x, '\n'); break;
	case 'h': PUSH(x, &x->m->h); break;
	case 'i': immediate(x); break;
	case 'l': PUSH(x, &x->m->l); break;
	case 'm': cmove(x); break;
	case 'p': pick(x); break;
	case 's': PUSH(x, &x->m->c); break;
	case 't': type(x); break;
	case 'w': word(x); break;
	case 'x':
		switch (TOKEN(x)) {
		case '!': wstore(x); break;
		}
		break;
	}
}

X* init_pForth() {
	X* x = init();
	EXT(x, 'P') = &pForth_ext;

	evaluate(x, "\\P: : \\$P$: \\P;");
	evaluate(x, ": ; \\$P$; \\P;Pi");

	evaluate(x, ": immediate \\$P$i ;");
	evaluate(x, ": context \\$P$l ;");
	evaluate(x, ": cell \\$c ;");
	evaluate(x, ": cells \\$P$c ;");

	evaluate(x, ": @ \\$. ;");
	evaluate(x, ": ! \\$, ;");
	evaluate(x, ": c@ \\$: ;");
	evaluate(x, ": c! \\$; ;");
	evaluate(x, ": +! \\$P$+ ;");
	evaluate(x, ": w! \\$P$x$! ;");

	evaluate(x, ": dup \\$d ;");
	evaluate(x, ": swap \\$s ;");
	evaluate(x, ": 2swap \\$r$($r$) ;");
	evaluate(x, ": drop \\$_ ;");
	evaluate(x, ": rot \\$r ;");
	evaluate(x, ": pick \\$P$p ;");
	evaluate(x, ": over \\$o ;");

	evaluate(x, ": >r \\$( ;");
	evaluate(x, ": r> \\$) ;");

	evaluate(x, ": and \\$& ;");
	evaluate(x, ": or \\$| ;");
	evaluate(x, ": xor \\$^ ;");

	evaluate(x, ": < \\$< ;");
	evaluate(x, ": > \\$> ;");
	evaluate(x, ": 0= \\$0$= ; ");

	evaluate(x, ": + \\$+ ;");
	evaluate(x, ": - \\$- ;");
	evaluate(x, ": 1+ \\$1$+ ;");
	evaluate(x, ": 1- \\$1$- ;");

	evaluate(x, ": d- \\$P$d$- ;");

	evaluate(x, ": word \\$P$w ;");
	evaluate(x, ": eol \\$P$e ;");
	evaluate(x, ": type \\$P$t ;");
	evaluate(x, ": cmove \\$P$m ;");

	evaluate(x, ": base \\$P$b ;");
	evaluate(x, ": state \\$P$s ;");

	evaluate(x, ": literal \\$P$# ; immediate");
	evaluate(x, ": here \\$h ;");
	evaluate(x, ": dp \\$P$h ;");

	return x;
}
*/

#endif
