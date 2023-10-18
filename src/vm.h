/* TOOD: Add literal to step */
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
typedef int8_t B;
typedef intptr_t C;

#define HIDDEN 1
#define IMMEDIATE 2
#define VARIABLE 4
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
	B d[1];
} M;

#define DSTACK_SIZE 256
#define RSTACK_SIZE 256

struct _Context {
  C d[DSTACK_SIZE];
	C dp;
	C r[RSTACK_SIZE];
	C rp;
	C ip;
	C err;
	M* m;
};

V reset_context(X* x) {
	x->dp = 0;
	x->rp = 0;
	x->ip = MEM_SIZE;
	x->err = 0;
}

X* init() {
	X* x = malloc(sizeof(X));
	if (!x) return 0;
	x->m = malloc(sizeof(M) + MEM_SIZE);
	if (!x->m) { free(x); return 0; }
	reset_context(x);

	return x;
}

/* Helpers */

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

#define DO(x, f) { f(x); if (x->err) return; }
#define ERR(x, c, e) if (c) { x->err = e; return; }

#define ERR_UNDEFINED_WORD -13

V parse_name(X* x) {
	while (x->m->ipos < x->m->ilen && isspace(x->m->ibuf[x->m->ipos])) x->m->ipos++;
	PUSH(x, &x->m->ibuf[x->m->ipos]);
	while (x->m->ipos < x->m->ilen && !isspace(x->m->ibuf[x->m->ipos])) x->m->ipos++;
	PUSH(x, &x->m->ibuf[x->m->ipos] - T(x));
}

V find_name(X* x) {
	L2(x, C, l, B*, t);
	S* s = x->m->l;
	while (s) {
		if (s->nl == l && !strncmp((const char *)s->n, (const char*)t, (long unsigned int)l)) break;
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

/* Inner interpreter */

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

		case 'x': call(x); break;
		case ']': case '}': ret(x); break;
		case 'j': jump(x); break;
		case 'z': zjump(x); break;

		case ':': bfetch(x); break;
		case '.': bstore(x); break;
		case ';': cfetch(x); break;
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

/*
		case 'h': PUSH(x, &x->m->d[x->m->h]); break;
		case 'l': PUSH(x, &x->m->l); break;

		case 'c': PUSH(x, sizeof(C)); break;
*/
		}
	}
}

V inner(X* x) { 
	C rp = x->rp; 
	while(x->rp >= rp && x->ip < MEM_SIZE && !x->err) { 
		step(x); 
		/* TODO: Manage errors with handlers */
	} 
}

/* Outer interpreter */

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
				if ((s->f & VARIABLE) == VARIABLE) {
					PUSH(x, &x->m->d[s->c]);
					if (x->m->c) literal(x);
				} else if ((s->f & CONSTANT) == CONSTANT) {
					PUSH(x, *((C*)(&x->m->d[s->c])));
					if (x->m->c) literal(x);
				} else if (!x->m->c || (s->f & IMMEDIATE) == IMMEDIATE) {
					eval(x, s->c);
				} else { 
					PUSH(x, s); 
					compile(x); 
				}
			} else {
			  L3(x, S*, _, C, l, B*, t);
				if (t[0] == '\\') {
					int i;
					for (i = 1; i < l; i++) { x->m->d[MEM_SIZE - l + i] = t[i]; }
					eval(x, MEM_SIZE - l + 1);
				} else {
					char * end;
					int n = strtol((char*)t, &end, 10);
					ERR(x, n == 0 && end == (char *)t, ERR_UNDEFINED_WORD);
					PUSH(x, n);
					if (x->m->c) literal(x);
				}
			}
		}
	}
}

/*
#define ERR_OK 0
#define ERR_UNDEFINED_WORD -13
#define ERR_ZERO_LEN_NAME -16
#define ERR_SYMBOL_ALLOCATION -256

#define DO(x, f) f(x); if (x->err) return
#define ERR(x, c, e) if (c) { x->err = e; return; }

typedef void V;
typedef char B;
typedef intptr_t C;

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Symbol {
  struct _Symbol* p;
  B f;
	C cl;
  C c;
  B nl;
  B n[1];
} S;

#define MEM_SIZE 65536

typedef struct _Memory {
	S* l;
	C hp;
	B h[MEM_SIZE];
} M;

#define DSTACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context { 
  C d[DSTACK_SIZE]; 
  C dp; 
  C r[RSTACK_SIZE]; 
  C rp; 
  C ip;
  C err;
  C s;
  C n;
  M* m;
  V (**x)(struct _Context*);
	B* ibuf;
	C ilen;
	C ipos;
} X;

V inner(X* x);

#define EXT(x, l) (x->x[l - 'A'])

#define DPUSH(x, u) (x->d[x->dp++] = (C)(u))
#define DPOP(x) (x->d[--x->dp])
#define DDROP(x) (x->dp--)

#define L1(x, t, v) t v = (t)DPOP(x)
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)

#define T(x) (x->d[x->dp - 1])
#define N(x) (x->d[x->dp - 2])
#define NN(x) (x->d[x->dp - 3])

#define GETB(x, a) (x->m->h[(a)])
#define GETS(x, a) (*((int16_t*)&x->m->h[(a)]))
#define GETI(x, a) (*((int32_t*)&x->m->h[(a)]))
#define GETL(x, a) (*((int64_t*)&x->m->h[(a)]))
#define GETC(x, a) (*((C*)&x->m->h[(a)]))

#define PUTB(x, a, b) (x->m->h[(a)] = (b))
#define PUTS(x, a, s) (*((int16_t*)&x->m->h[(a)]) = (int16_t)(s))
#define PUTI(x, a, w) (*((int32_t*)&x->m->h[(a)]) = (int32_t)(w))
#define PUTL(x, a, l) (*((int64_t*)&x->m->h[(a)]) = (int64_t)(l))
#define PUTC(x, a, i) (*((C*)&x->m->h[(a)]) = (i))

#define COMMAB(x, b) { PUTB(x, x->m->hp, b); x->m->hp += 1; }
#define COMMAS(x, w) { PUTS(x, x->m->hp, w); x->m->hp += 2; }
#define COMMAI(x, i) { PUTI(x, x->m->hp, i); x->m->hp += 4; }
#define COMMAL(x, l) { PUTL(x, x->m->hp, l); x->m->hp += 8; }

V literal(X* x) {
	L1(x, C, n);
	if (n == 0) { COMMAB(x, '0'); }
	else if (n == 1) { COMMAB(x, '1'); }
  else if (n > INT8_MIN && n < INT8_MAX) { COMMAB(x, '#'); COMMAB(x, n); }
  else if (n > INT16_MIN && n < INT16_MAX) { COMMAB(x, '2'); COMMAS(x, n); }
  else if (n > INT32_MIN && n < INT32_MAX) { COMMAB(x, '4'); COMMAI(x, n); }
  else { COMMAB(x, '8'); COMMAI(x, n); }
}

V compile(X* x) { 
	L1(x, S*, s); 
	int i; 
	while (i < s->cl) { 
		COMMAB(x, GETB(x, s->c + i++)); 
	} 
}

#define TAIL(x) (x->ip >= MEM_SIZE || GETB(x, x->ip) == ']' || GETB(x, x->ip) == '}')
V call(X* x) { L1(x, C, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = MEM_SIZE; }
V jump(X* x) { L1(x, C, d); x->ip += d - 1; }
V zjump(X* x) { L2(x, C, d, C, b); if (!b) x->ip += d - 1; }
V eval(X* x, C q) { DPUSH(x, q); call(x); inner(x); }
V quotation(X* x) { L1(x, C, d); DPUSH(x, x->ip); x->ip += d - 1; }
V recurse(X* x) { DPUSH(x, x->m->l->c); literal(x); PUTB(x, x->m->hp++, 'x'); }
V ahead(X* x) { DPUSH(x, x->m->hp + 1); DPUSH(x, 1024); literal(x); }
V fresolve(X* x) { L1(x, C, a); C d = x->m->hp - a - 2; PUTS(x, a, d); }
V bresolve(X* x) { L1(x, C, a); C d = a - x->m->hp + 3; DPUSH(x, a); literal(x); COMMAB(x, 'j'); }

V dup(X* x) { DPUSH(x, T(x)); }
V over(X* x) { DPUSH(x, N(x)); }
V swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { C t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }
V nip(X* x) { N(x) = T(x); DDROP(x); }

V to_r(X* x) { x->r[x->rp++] = x->d[--x->dp]; }
V from_r(X* x) { x->d[x->dp++] = x->r[--x->rp]; }

#define OP2(x, op) N(x) = N(x) op T(x); DDROP(x)
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

V lt(X* x) { OP2(x, <); }
V eq(X* x) { OP2(x, ==); }
V gt(X* x) { OP2(x, >); }

V cstore(X* x) { L2(x, C*, a, C, v); *a = v; }
V bstore(X* x) { L2(x, B*, a, B, v); *a = v; }
V cfetch(X* x) { L1(x, C*, a); DPUSH(x, *a); }
V bfetch(X* x) { L1(x, B*, a); DPUSH(x, *a); }

V times(X* x) { L2(x, C, q, C, n); for(;n > 0; n--) eval(x, q); }
V branch(X* x) { L3(x, C, f, C, t, C, b); b ? eval(x, t) : eval(x, f); }

#define S_pr(s, n, f, a) { C t; s += t = sprintf(s, f, a); n += t; }
              
V dump_code(X* x, C c) {
	C n;
	S* s;
  C t = 1;
	B token;
  while (t && c < MEM_SIZE) {
		token = GETB(x, c);
    if (token == '[') t++;
    if (token == ']') t--;
		if (token == '#') {
			if (GETB(x, c + 2) == 'e') {
				C n = GETB(x, ++c);
				s = x->m->l;
				while (s) {
					if (s->c == n) {
						printf("%.*s ", (int)s->nl, s->n);
					}
					s = s->p;
				}
				c += 2;
			} else {
				printf("#%d ", GETB(x, ++c));
				c += 1;
			}
		} else if (token == '2') {
			if (GETB(x, c + 3) == 'e') {
				C n = GETS(x, ++c);
				s = x->m->l;
				while (s) {
					if (s->c == n) {
						printf("%.*s ", (int)s->nl, s->n);
					}
					s = s->p;
				}
				c += 3;
			} else {
				printf("#%d ", GETS(x, ++c));
				c += 2;
			}
		} else if (token == '4') {
		  printf("#%d ", GETI(x, ++c));
			c += 4;
		} else if (token == '8') {
		  printf("#%ld ", GETL(x, ++c));
			c += 8;
		} else {
	    printf("%c", token);
	    c++;
		}
  }
}

V dump_context(X* x) {
  C i;
  B* t;
  for (i = 0; i < x->dp; i++) printf("%ld ", x->d[i]);
  printf("▢ ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" ▢ ");
    dump_code(x, x->r[i]); 
  }
  printf("\n");
}

V parse_name(X* x) {
	while (x->ipos < x->ilen && isspace(x->ibuf[x->ipos])) x->ipos++;
	DPUSH(x, &x->ibuf[x->ipos]);
	while (x->ipos < x->ilen && !isspace(x->ibuf[x->ipos])) x->ipos++;
	DPUSH(x, &x->ibuf[x->ipos] - T(x));
}

V find_name(X* x) {
	L2(x, C, l, B*, t);
	S* s = x->m->l;
	while (s) {
		if (s->nl == l && !strncmp(s->n, t, l)) break;
		s = s->p;
	}
	DPUSH(x, t);
	DPUSH(x, l);
	DPUSH(x, s);
}

V see(X* x) { 
	parse_name(x); 
	find_name(x); 
	{ 
		L3(x, S*, s, C, l, B*, t);
		printf("[%ld] : %.*s ", s->c, (int)l, t);
		dump_code(x, s->c); 
		if ((s->f & IMMEDIATE) == IMMEDIATE) printf(" IMMEDIATE");
		printf("\n");
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
    s->f = 0;
    s->cl = 0;
    s->c = x->m->hp;
    s->nl = l;
    strncpy(s->n, n, l);
    s->n[l] = 0;
  }
}
  
V colon(X* x) {
  DO(x, create);
  x->m->l->f = HIDDEN;
  x->s = 1;
}

V semicolon(X* x) { COMMAB(x, ']');	x->s = 0;	x->m->l->f &= ~HIDDEN; x->m->l->cl = x->m->hp - x->m->l->c - 1; }
V immediate(X* x) {	x->m->l->f |= IMMEDIATE; }

V postpone(X* x) { 
	parse_name(x); 
	find_name(x);
	{
		L3(x, S*, s, C, _, B*, __);
		int i;
		for (i = 0; i < s->cl; i++) {
			COMMAB(x, '$');
			COMMAB(x, GETB(x, s->c + i));
		}
	}
}

#define PEEK(x) (GETB(x, x->ip))
#define TOKEN(x) (GETB(x, x->ip++))
                
V step(X* x) {
	dump_context(x);
  if (!x->err) {
  	switch (PEEK(x)) {
  	  case 'A': case 'B': 
  	  case 'C':	case 'D': 
  	  case 'E': case 'F':
  	  case 'G': case 'H':
  	  case 'I': case 'J': 
  	  case 'K': case 'L':
  	  case 'M': case 'N': 
  	  case 'O': case 'P':
  	  case 'Q': case 'R': 
  	  case 'S': case 'T':
  	  case 'U': case 'V': 
  	  case 'W': case 'X':
  	  case 'Y': case 'Z':
  	    EXT(x, TOKEN(x))(x);
  	    break;
			default:
				switch (TOKEN(x)) {

  		  case '0': DPUSH(x, 0); break;
  		  case '1': DPUSH(x, 1); break;
				case '#': DPUSH(x, GETB(x, x->ip)); x->ip += 1; break;
 				case '2': DPUSH(x, GETS(x, x->ip)); x->ip += 2; break;
 				case '4': DPUSH(x, GETI(x, x->ip)); x->ip += 4; break;
 				case '8': DPUSH(x, GETL(x, x->ip)); x->ip += 8; break;

				case 'x': call(x); break;
				case '[': quotation(x); break;
  	  	case ']': case '\\': ret(x); break;
				case 'j': jump(x); break;
				case 'z': zjump(x); break;

				case '{': { L1(x, C, e); x->err = e; }; break;

				case ':': colon(x); break;
				case ';': semicolon(x); break;
				case 'i': immediate(x); break;
				case '$': COMMAB(x, GETB(x, x->ip++)); break;
				case '`': recurse(x); break;
				case 'a': ahead(x); break;
        case 'h': DPUSH(x, x->m->hp); break;
				case 'f': fresolve(x); break;
        case 'l': bresolve(x); break;

			  case '_': DDROP(x); break;
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
  		  case ',': istore(x); break;
  		  case ';': bstore(x); break;
  		  case '.': ifetch(x); break;
  		  case ':': bfetch(x); break;
  		  case 't': times(x); break;
  		  case '?': branch(x); break;

				case 'w':
					switch (TOKEN(x)) {
						case 's': see(x); break;
					}
					break;
				case 'p': 
					switch (TOKEN(x)) {
					case '@': cstore(x); break;
					case 'x': DPUSH(x, &x->m->l); break;
					}
					break;
			}
  	}
  }
}
              
V inner(X* x) { C rp = x->rp; while(x->rp >= rp && x->ip < MEM_SIZE && !x->err) { step(x); } }

V evaluate(X* x, B* s) {
	x->ibuf = s;	
	x->ilen = strlen(s);
	x->ipos = 0;
	while (x->ipos < x->ilen && !x->err) {
		DO(x, parse_name);
    if (T(x) == 0) { DDROP(x); DDROP(x); return; }
		DO(x, find_name);
		if (T(x)) {
			L3(x, S*, s, C, l, B*, t);
			if (!x->s || (s->f & IMMEDIATE) == IMMEDIATE) {
				eval(x, s->c);
			} else {
				DPUSH(x, s);
				compile(x);
			}
		} else {
			L3(x, S*, _, C, l, B*, t);
			if (t[0] == '\\') {
				int i;
				for (i = 1; i < l; i++) { PUTB(x, MEM_SIZE - l + i, t[i]); }
				eval(x, MEM_SIZE - l + 1);
			} else {
				char* end;
				int n = strtol(t, &end, 10);
        ERR(x, n == 0 && end == t, ERR_UNDEFINED_WORD);
				DPUSH(x, n);
				if (x->s) literal(x);
			}
		}
	}
}

V reset_context(X* x) {
	x->err = 0;
	x->dp = 0;
	x->rp = 0;
  x->ibuf = 0;
  x->ipos = 0;
  x->ilen = 0;
}

X* init_VM(M* m) { 
	X* x = malloc(sizeof(X)); 
	x->m = m; 
	x->ip = MEM_SIZE;

	return x;
}

X* init_EXT(X* x) { x->x = malloc(26*sizeof(C)); return x; }

M* init_MEM() { 
	M* m = malloc(sizeof(M));
	m->l = 0;
	m->hp = 0;
	return m;
}

X* init_SLOTH(X* x) {
	evaluate(x, "\\: : \\$: \\;");
	evaluate(x, ": ; \\$; \\;i");

	evaluate(x, ": immediate \\$i ;");
	evaluate(x, ": execute \\$x ;");

	evaluate(x, ": recurse \\$` ; immediate");

	evaluate(x, ": >mark \\$a ;");
	evaluate(x, ": >resolve \\$f ;");
  evaluate(x, ": mark> \\$h ;");
	evaluate(x, ": resolve> \\$l ;");
	evaluate(x, ": 0branch \\$z ;");
	evaluate(x, ": jump \\$j ;");

	evaluate(x, ": postpone \\$p ; immediate");

	evaluate(x, ": drop \\$_ ;");
	evaluate(x, ": dup \\$d ;");
	evaluate(x, ": over \\$o ;");
	evaluate(x, ": swap \\$s ;");
	evaluate(x, ": rot \\$@ ;");
	evaluate(x, ": nip \\$n ;");

	evaluate(x, ": + \\$+ ;");
	evaluate(x, ": - \\$- ;");
	evaluate(x, ": * \\$* ;");
	evaluate(x, ": / \\$/ ;");
	evaluate(x, ": mod \\$% ;");

	evaluate(x, ": < \\$< ;");
	evaluate(x, ": = \\$= ;");
	evaluate(x, ": > \\$> ;");

	evaluate(x, ": and \\$& ;");
	evaluate(x, ": or \\$| ;");
	evaluate(x, ": invert \\$~ ;");
	
	evaluate(x, ": see \\$w$s ;");

	evaluate(x, ": if >mark postpone 0branch ; immediate");
	evaluate(x, ": else >mark postpone jump swap >resolve ; immediate");
	evaluate(x, ": then >resolve ; immediate");

	evaluate(x, ": fib dup 1 > if 1 - dup 1 - recurse swap recurse + then ;");

  evaluate(x, ": >r \\$( ;");
  evaluate(x, ": r> \\$) ;");
  evaluate(x, ": 2dup over over ;");
  evaluate(x, ": 1+ 1 + ;");

	evaluate(x, ": do postpone swap postpone >r postpone >r mark> ; immediate");
	evaluate(x, ": i r> dup >r ;");
	evaluate(x, ": loop postpone r> postpone 1+ postpone r> postpone 2dup postpone >r postpone >r postpone = postpone if resolve> postpone jump postpone then postpone r> postpone r> ; immediate");

	return x;
}
*/
#endif
