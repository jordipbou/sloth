#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

#if INTPTR_MAX == INT32_MAX
    #define BITS_32
#elif INTPTR_MAX == INT64_MAX
    #define BITS_64
#else
    #error "Environment not 32 or 64-bit."
#endif

typedef void V;
typedef int8_t B;
typedef int16_t S;
typedef int32_t I;
typedef int64_t L;
typedef intptr_t C;

#if defined (BITS_64)
#define ALIGN(a) (a + 7) & ~(7)
#else
#define ALIGN(a) (a + 3) & ~(3)
#endif

typedef struct _Machine M;

typedef struct _Dictionary {
	B* b;
	C s;
	V (*x[26])(M*);
} D;

#define EXT(m, l) (m->d->x[l - 'A'])

#define DSTACK_SIZE 256
#define RSTACK_SIZE 256

struct _Machine {
	C sp;
	C rp;
	C ip;
	C err;
	C s[DSTACK_SIZE];
	C r[RSTACK_SIZE];
	D* d;
};

V inner(M*);

#define PUSH(m, v) (m->s[m->sp++] = (C)(v))
#define POP(m) (m->s[--m->sp])
#define DROP(m) (--m->sp)

#define T(m) (m->s[m->sp - 1])
#define N(m) (m->s[m->sp - 2])
#define NN(m) (m->s[m->sp - 3])

#define L1(m, t, v) t v = (t)POP(m)
#define L2(m, t1, v1, t2, v2) L1(m, t1, v1); L1(m, t2, v2)
#define L3(m, t1, v1, t2, v2, t3, v3) L2(m, t1, v1, t2, v2); L1(m, t3, v3)
#define L4(m, t1, v1, t2, v2, t3, v3, t4, v4) L3(m, t1, v1, t2, v2, t3, v3); L1(m, t4, v4)

#define DO(m, f) { f(m); if (m->err) return; }
#define ERR(m, c, e) if (c) { m->err = e; return; }

V duplicate(M* m) { PUSH(m, T(m)); }
V over(M* m) { PUSH(m, N(m)); }
V swap(M* m) { C t = T(m); T(m) = N(m); N(m) = t; }
V rot(M* m) { C t = NN(m); NN(m) = N(m); N(m) = T(m); T(m) = t; }
V nip(M* m) { N(m) = T(m); DROP(m); }

V to_r(M* m) { m->r[m->rp++] = m->s[--m->sp]; }
V from_r(M* m) { m->s[m->sp++] = m->r[--m->rp]; }

V add(M* m) { N(m) = N(m) + T(m); DROP(m); }
V sub(M* m) { N(m) = N(m) - T(m); DROP(m); }
V mul(M* m) { N(m) = N(m) * T(m); DROP(m); }
V division(M* m) { N(m) = N(m) / T(m); DROP(m); }
V mod(M* m) { N(m) = N(m) % T(m); DROP(m); }

V and(M* m) { N(m) = N(m) & T(m); DROP(m); }
V or(M* m) { N(m) = N(m) | T(m); DROP(m); }
V xor(M* m) { N(m) = N(m) ^ T(m); DROP(m); }
V not(M* m) { T(m) = !T(m); }
V invert(M* m) { T(m) = ~T(m); }

V lt(M* m) { N(m) = (N(m) < T(m)) ? -1 : 0; DROP(m); }
V eq(M* m) { N(m) = (N(m) == T(m)) ? -1 : 0; DROP(m); }
V gt(M* m) { N(m) = (N(m) > T(m)) ? -1 : 0; DROP(m); }

V pstore(M* m) { L2(m, C*, a, C, b); *a = b; }
V pfetch(M* m) { L1(m, C*, a); PUSH(m, *a); }
V bstore(M* m) { L2(m, B*, a, B, b); *a = b; }
V bfetch(M* m) { L1(m, B*, a); PUSH(m, *a); }

V reset(M* m) { m->ip = 0; m->rp = 0; m->sp = 0; }

#define PEEK(m) (m->d->b[m->ip])
#define TOKEN(m) (m->d->b[m->ip++])
#define IN(m, p) (p >= 0 && p < m->d->s)
#define TAIL(m) (!(IN(m, m->ip)) || PEEK(m) == ']' || PEEK(m) =='}')

V execute(M* m) { L1(m, C, q); if (!TAIL(m)) { m->r[m->rp++] = m->ip; } m->ip = q; }
V ret(M* m) { m->ip = (m->rp > 0) ? m->r[--m->rp] : m->d->s; }

#define BLOCK(m, ip) { \
	C t = 1; \
	while (t && IN(m, ip)) { \
		switch (m->d->b[ip++]) { \
		case '{': case '[': t++; break; \
		case '}': case ']': t--; break; \
		} \
	} \
}

V block(M* m) { C t = 1; PUSH(m, m->ip); BLOCK(m, m->ip); }

V dc(M* m, C ip) { 
	C t = ip;
	B c;
	BLOCK(m, ip);
	printf(" : ");
	for(;t < ip; c = m->d->b[t], t++) { 
		if (c != 10) printf("%c", c); 
	}
}
V ds(M* m) { C i; for(i = 0; i < m->sp; i++) { printf("%ld ", m->s[i]); } }
V dr(M* m) { C i; dc(m, m->ip); for(i = 0; i < m->rp; i++) { dc(m, m->r[i]); } }
V trace(M* m) { ds(m); dr(m); }

V times(M* m) { L2(m, C, q, C, n); while (n-- > 0) { PUSH(m, q); execute(m); DO(m, inner); } }

V step(M* m) {
	trace(m); printf("\n");
	switch (PEEK(m)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
  case 'E': case 'K':
		EXT(m, TOKEN(m))(m);
		break;
	default:
		switch (TOKEN(m)) {
		case '0': PUSH(m, 0); break;
		case '1': PUSH(m, 1); break;

		case '_': DROP(m); break;
		case 'd': duplicate(m); break;
		case 'o': over(m); break;
		case 's': swap(m); break;
		case 'r': rot(m); break;
		case 'n': nip(m); break;

		case '(': to_r(m); break;
		case ')': from_r(m); break;

		case '+': add(m); break;
		case '-': sub(m); break;
		case '*': mul(m); break;
		case '/': division(m); break;
		case '%': mod(m); break;

		case '&': and(m); break;
		case '|': or(m); break;
		case '^': xor(m); break;
		case '!': not(m); break;
		case '~': invert(m); break;

		case '<': lt(m); break;
		case '=': eq(m); break;
		case '>': gt(m); break;

		case ',': pstore(m); break;
		case '.': pfetch(m); break;
		case ';': bstore(m); break;
		case ':': bfetch(m); break;

		case '{': block(m); break;
		case '}': ret(m); break;
		case 'x': execute(m); break;

		case 't': times(m); break;

		case '$': reset(m); break;
/*
		case 'x': call(m); break;
		case 'j': jump(m); break;
		case 'z': zjump(m); break;
		case '[': quot(m); break;
		case ']': ret(m); break;

		case 'c': PUSH(m, sizeof(C)); break;
		case '@': PUSH(m, m); break;
*/
		}
	}
}

V inner(M* m) {
	C t = m->rp;
	while (m->rp >= t && IN(m, m->ip)) {
		step(m);
		/* Manage errors */
	}
}

V isolated(M* m, char* s) {
	m->d->b = (B*)s;
	m->d->s = strlen(s);
	m->ip = 0;
	inner(m);
}

D* init_DICT(int block_size) {
	D* d = malloc(sizeof(D));
	if (!d) return 0;
	if (block_size) {
		d->b = malloc(block_size);
		d->s = block_size;
	} else {
		d->b = 0;
		d->s = 0;
	}

	return d;
	
}

M* init_VM(D* d) {
	M* m;

	if (!d) return 0;
	m = malloc(sizeof(M));
	if (!m) return 0;
	m->d = d;

	return m;
}

/*
#define VARIABLE 32
#define HIDDEN 64 
#define IMMEDIATE 128 

#define FLAGS(w) (*((B*)(w + 2)))
#define SET_VARIABLE(w)	(FLAGS(w) = FLAGS(w) | VARIABLE)
#define SET_HIDDEN(w) (FLAGS(w) = FLAGS(w) | HIDDEN)
#define SET_IMMEDIATE(w) (FLAGS(w) = FLAGS(w) | IMMEDIATE)
#define UNSET_VARIABLE(w) (FLAGS(w) = FLAGS(w) & ~VARIABLE)
#define UNSET_HIDDEN(w) (FLAGS(w) = FLAGS(w) & ~HIDDEN)
#define UNSET_IMMEDIATE(w) (FLAGS(w) = FLAGS(w) & ~IMMEDIATE)
#define IS_IMMEDIATE(w) ((FLAGS(w) & IMMEDIATE) == IMMEDIATE)
#define IS_HIDDEN(w) ((FLAGS(w) & HIDDEN) == HIDDEN)
#define IS_VARIABLE(w) ((FLAGS(w) & VARIABLE) == VARIABLE)
#define NAME_LENGTH(w) (FLAGS(w) & 0x1F)
#define NAME(w) (w + 3)

#define CODE(w) (ALIGN(w + NAME_LENGTH(w) + 1))

#define GET_AT(m, t, p) (*((t*)(m->s->b + p)))
#define PUT_AT(m, t, p, v) (*((t*)(m->s->b + p)) = (t)(v))

#define HERE(m) (*((C*)(m->s->b)))
#define SIZE(m) (*((C*)(m->s->b + sizeof(C))))
#define LATEST(m) (*((C*)(m->s->b + 2*sizeof(C))))

#define COMPILE(m, t, v) { *((t*)(m->s->b + HERE(m))) = (t)(v); HERE(m) = HERE(m) + sizeof(t); }

#define ABUF(m) (m->s->b + SIZE(m) - 64)
#define IBUF(m) (ABUF(m) - 256)

typedef struct _Machine M; 

typedef struct _Dictionary {
	V (*x[26])(M*);
	B* b;
	C s;
} SYS;

#define EXT(m, l) (m->s->x[l - 'A'])

#define DSTACK_SIZE 256
#define RSTACK_SIZE 256

struct _Machine {
  C d[DSTACK_SIZE];
	C dp;
	C r[RSTACK_SIZE];
	C rp;
	C ip;
	C err;
	struct _System* s;
};

V inner(M*);

#define PUSH(m, v) (m->d[m->dp++] = (C)(v))
#define POP(m) (m->d[--m->dp])
#define DROP(m) (--m->dp)

#define T(m) (m->d[m->dp - 1])
#define N(m) (m->d[m->dp - 2])
#define NN(m) (m->d[m->dp - 3])

#define L1(m, t, v) t v = (t)POP(m)
#define L2(m, t1, v1, t2, v2) L1(m, t1, v1); L1(m, t2, v2)
#define L3(m, t1, v1, t2, v2, t3, v3) L2(m, t1, v1, t2, v2); L1(m, t3, v3)
#define L4(m, t1, v1, t2, v2, t3, v3, t4, v4) L3(m, t1, v1, t2, v2, t3, v3); L1(m, t4, v4)

#define DO(m, f) { f(m); if (m->err) return; }
#define ERR(m, c, e) if (c) { m->err = e; return; }

V duplicate(M* m) { PUSH(m, T(m)); }
V over(M* m) { PUSH(m, N(m)); }
V swap(M* m) { C t = T(m); T(m) = N(m); N(m) = t; }
V rot(M* m) { C t = NN(m); NN(m) = N(m); N(m) = T(m); T(m) = t; }
V nip(M* m) { N(m) = T(m); DROP(m); }

V to_r(M* m) { m->r[m->rp++] = m->d[--m->dp]; }
V from_r(M* m) { m->d[m->dp++] = m->r[--m->rp]; }

V add(M* m) { N(m) = N(m) + T(m); DROP(m); }
V sub(M* m) { N(m) = N(m) - T(m); DROP(m); }
V mul(M* m) { N(m) = N(m) * T(m); DROP(m); }
V division(M* m) { N(m) = N(m) / T(m); DROP(m); }
V mod(M* m) { N(m) = N(m) % T(m); DROP(m); }

V and(M* m) { N(m) = N(m) & T(m); DROP(m); }
V or(M* m) { N(m) = N(m) | T(m); DROP(m); }
V xor(M* m) { N(m) = N(m) ^ T(m); DROP(m); }
V not(M* m) { T(m) = !T(m); }
V invert(M* m) { T(m) = ~T(m); }

V lt(M* m) { N(m) = (N(m) < T(m)) ? -1 : 0; DROP(m); }
V eq(M* m) { N(m) = (N(m) == T(m)) ? -1 : 0; DROP(m); }
V gt(M* m) { N(m) = (N(m) > T(m)) ? -1 : 0; DROP(m); }

V pstore(M* m) { L2(m, C*, a, C, b); *a = b; }
V pfetch(M* m) { L1(m, C*, a); PUSH(m, *a); }
V bstore(M* m) { L2(m, B*, a, B, b); *a = b; }
V bfetch(M* m) { L1(m, B*, a); PUSH(m, *a); }

#define TAIL(m) (m->ip >= (SIZE(m)) || m->s->b[m->ip] == ']')
V call(M* m) { L1(m, C, q); if (!TAIL(m)) { m->r[m->rp++] = m->ip; } m->ip = q; }
V ret(M* m) { m->ip = (m->rp > 0) ? m->r[--m->rp] : 0; }
V jump(M* m) { L1(m, C, d); m->ip += d - 1; }
V zjump(M* m) { L2(m, C, d, C, b); if (!b) m->ip += d - 1; }
V quot(M* m) { L1(m, C, d); PUSH(m, m->ip); m->ip += d; }
V eval(M* m, B* q) { PUSH(m, q); call(m); inner(m); }

#define PEEK(x) (m->s->b[m->ip])
#define TOKEN(x) (m->s->b[m->ip++])

V step(M* m) {
	switch (PEEK(m)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
  case 'E': case 'K':
		EXT(m, TOKEN(m))(m);
		break;
	default:
		switch (TOKEN(m)) {
		case '0': PUSH(m, 0); break;
		case '1': PUSH(m, 1); break;

		case '_': DROP(m); break;
		case 'd': duplicate(m); break;
		case 'o': over(m); break;
		case 's': swap(m); break;
		case 'r': rot(m); break;
		case 'n': nip(m); break;

		case '(': to_r(m); break;
		case ')': from_r(m); break;

		case '+': add(m); break;
		case '-': sub(m); break;
		case '*': mul(m); break;
		case '/': division(m); break;
		case '%': mod(m); break;

		case '&': and(m); break;
		case '|': or(m); break;
		case '^': xor(m); break;
		case '!': not(m); break;
		case '~': invert(m); break;

		case '<': lt(m); break;
		case '=': eq(m); break;
		case '>': gt(m); break;

		case ',': pstore(m); break;
		case '.': pfetch(m); break;
		case ';': bstore(m); break;
		case ':': bfetch(m); break;

		case 'x': call(m); break;
		case 'j': jump(m); break;
		case 'z': zjump(m); break;
		case '[': quot(m); break;
		case ']': ret(m); break;

		case 'c': PUSH(m, sizeof(C)); break;
		case '@': PUSH(m, m); break;
		}
	}
}

V inner(M* m) { 
	C rp = m->rp; 
	while(m->rp >= rp && !m->err && m->ip < SIZE(m)) { 
		step(m); 
	} 
)

V evaluate(M* m, B* s) {
  
}

V reset(M* m) {
  m->ip = SIZE(m);
  m->dp = 0;
  m->rp = 0;
}
                
M* init() {
  M* m = malloc(sizeof(M));
  m->s = malloc(sizeof(SYS));
  m->s->b = malloc(65536);
  SIZE(m) = 65536;
  LATEST(m) = 0;
  HERE(m) = 3*sizeof(C);
  reset(m);
  return m;
}
*/
/*

typedef struct _Machine M;

#define DICT_SIZE 65536
#define ASM_BUFFER_SIZE 80

typedef struct _System {
	C hs;
	C hp;
	N* l;
	V (*x[26])(M*);
	B h[DICT_SIZE];
} S;

#define EXT(m, l) (m->s->x[l - 'A'])

#define cbyte(s, b) (s)->h[(s)->hp++] = (b)
#define cword(s, w) *((W*)((s)->h + (s)->hp)) = (w); (s)->hp += 2
#define clong(s, l) *((L*)((s)->h + (s)->hp)) = (l); (s)->hp += 4
#if defined (BITS_64)
#define cextra(s, x) { *((X*)((s)->h + (s)->hp)) = (x); (s)->hp += 8; }
#define ccell(s, n) cextra(s, n)
#else
#define ccell(s, n) clong(s, n)
#endif

#define DSTACK_SIZE 64 
#define RSTACK_SIZE 64

struct _Machine {
	C dp;
	C rp;
	B* ip;
	C err;
	C st;
	S* s;
	char* ibuf;
	C ilen;
	C ipos;
  C d[DSTACK_SIZE];
	C r[RSTACK_SIZE];
};

V inner(M*);

#define PUSH(m, v) (m->d[m->dp++] = (C)(v))
#define POP(m) (m->d[--m->dp])
#define DROP(m) (--m->dp)

#define T(m) (m->d[m->dp - 1])
#define N(m) (m->d[m->dp - 2])
#define NN(m) (m->d[m->dp - 3])

#define L1(m, t, v) t v = (t)POP(m)
#define L2(m, t1, v1, t2, v2) L1(m, t1, v1); L1(m, t2, v2)
#define L3(m, t1, v1, t2, v2, t3, v3) L2(m, t1, v1, t2, v2); L1(m, t3, v3)
#define L4(m, t1, v1, t2, v2, t3, v3, t4, v4) L3(m, t1, v1, t2, v2, t3, v3); L1(m, t4, v4)

#define DO(m, f) { f(m); if (m->err) return; }
#define ERR(m, c, e) if (c) { m->err = e; return; }

V duplicate(M* m) { PUSH(m, T(m)); }
V over(M* m) { PUSH(m, N(m)); }
V swap(M* m) { C t = T(m); T(m) = N(m); N(m) = t; }
V rot(M* m) { C t = NN(m); NN(m) = N(m); N(m) = T(m); T(m) = t; }
V nip(M* m) { N(m) = T(m); DROP(m); }

V to_r(M* m) { m->r[m->rp++] = m->d[--m->dp]; }
V from_r(M* m) { m->d[m->dp++] = m->r[--m->rp]; }

V add(M* m) { N(m) = N(m) + T(m); DROP(m); }
V sub(M* m) { N(m) = N(m) - T(m); DROP(m); }
V mul(M* m) { N(m) = N(m) * T(m); DROP(m); }
V division(M* m) { N(m) = N(m) / T(m); DROP(m); }
V mod(M* m) { N(m) = N(m) % T(m); DROP(m); }

V and(M* m) { N(m) = N(m) & T(m); DROP(m); }
V or(M* m) { N(m) = N(m) | T(m); DROP(m); }
V xor(M* m) { N(m) = N(m) ^ T(m); DROP(m); }
V not(M* m) { T(m) = !T(m); }
V invert(M* m) { T(m) = ~T(m); }

V lt(M* m) { N(m) = (N(m) < T(m)) ? -1 : 0; DROP(m); }
V eq(M* m) { N(m) = (N(m) == T(m)) ? -1 : 0; DROP(m); }
V gt(M* m) { N(m) = (N(m) > T(m)) ? -1 : 0; DROP(m); }

V pstore(M* m) { L2(m, C*, a, C, b); *a = b; }
V pfetch(M* m) { L1(m, C*, a); PUSH(m, *a); }
V bstore(M* m) { L2(m, B*, a, B, b); *a = b; }
V bfetch(M* m) { L1(m, B*, a); PUSH(m, *a); }

V literal(M* m) {
	L1(m, C, n);
	if (n >= INT8_MIN && n <= INT8_MAX) { cbyte(m->s, 'b'); cbyte(m->s, (B)n); }
	else if (n >= INT16_MIN && n <= INT16_MAX) { cbyte(m->s, 'w'); cword(m->s, (W)n); }
	else if (n >= INT32_MIN && n <= INT32_MAX) { cbyte(m->s, 'l'); clong(m->s, (L)n); }
#if defined (BITS_64)
	else { cbyte(m->s, 'x'); cextra(m->s, (X)n); }
#endif
}

V compile(M* m) {
}

V interpret(M* m) {
}

V call(M* m) { L1(m, B*, q); if (*q != ']') m->r[m->rp++] = (C)m->ip; m->ip = q; }
V ret(M* m) { m->ip = (m->rp > 0) ? (B*)m->r[--m->rp] : 0; }
V jump(M* m) { L1(m, C, d); m->ip += d - 1; }
V zjump(M* m) { L2(m, C, d, C, b); if (!b) m->ip += d - 1; }
V quot(M* m) { L1(m, C, d); PUSH(m, m->ip); m->ip += d; }
V eval(M* m, B* q) { PUSH(m, q); call(m); inner(m); }

V align(M* m) {
#if defined (BITS_64)
	m->s->hp = (m->s->hp + 7) & 7;
#else
	m->s->hp = (m->s->hp + 3) & 3;
#endif
}

V parse_spaces(M* m) { while (m->ipos < m->ilen && isspace(m->ibuf[m->ipos]))	m->ipos++; }
V parse(M* m) { L1(m, C, c); while (m->ipos < m->ilen && m->ibuf[m->ipos] != c) m->ipos++; m->ipos++; }
V parse_non_spaces(M* m) { while (m->ipos < m->ilen && !isspace(m->ibuf[m->ipos])) m->ipos++; }
V parse_name(M* m) {
	parse_spaces(m);
	PUSH(m, m->ibuf + m->ipos);
	parse_non_spaces(m);
	PUSH(m, (m->ibuf + m->ipos) - T(m));
}

V colon(M* m) {
	parse_name(m);
	{
		L2(m, C, l, B*, t);
		N* n = (N*)(m->s->h + m->s->hp);
		printf("Creating new word at %ld\n", n);
		printf("m->s->l %ld\n", m->s->l);
		n->p = m->s->l;
		m->s->l = n;
		n->f = HIDDEN;
		n->l = l;
		strncpy((char*)n->n, (char*)t, l);
		n->n[l] = 0;
		align(m);
		n->c = m->s->h + m->s->hp;
		m->st = 1;
	}
}

V semicolon(M* m) {
	m->s->l->f &= ~HIDDEN;
	m->st = 0;
}

#define PEEK(x) (*m->ip)
#define TOKEN(x) (*m->ip++)

V step(M* m) {
	switch (PEEK(m)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
  case 'E': case 'K':
		EXT(m, TOKEN(m))(m);
		break;
	default:
		switch (TOKEN(m)) {
		case '0': PUSH(m, 0); break;
		case '1': PUSH(m, 1); break;
		case 'b': PUSH(m, TOKEN(x)); break;
		case 'w': PUSH(m, *((W*)m->ip)); m->ip += 2; break;
		case 'l': PUSH(m, *((L*)m->ip)); m->ip += 4; break;
#if defined (BITS_64)
		case 'x': PUSH(m, *((X*)m->ip)); m->ip += 8; break;
#endif

		case 'h': colon(m); break;
		case 'v': semicolon(m); break;

		case '_': DROP(m); break;
		case 'd': duplicate(m); break;
		case 'o': over(m); break;
		case 's': swap(m); break;
		case 'r': rot(m); break;
		case 'n': nip(m); break;

		case '(': to_r(m); break;
		case ')': from_r(m); break;

		case '+': add(m); break;
		case '-': sub(m); break;
		case '*': mul(m); break;
		case '/': division(m); break;
		case '%': mod(m); break;

		case '&': and(m); break;
		case '|': or(m); break;
		case '^': xor(m); break;
		case '!': not(m); break;
		case '~': invert(m); break;

		case '<': lt(m); break;
		case '=': eq(m); break;
		case '>': gt(m); break;

		case ',': pstore(m); break;
		case '.': pfetch(m); break;
		case ';': bstore(m); break;
		case ':': bfetch(m); break;

		case 'e': call(m); break;
		case 'j': jump(m); break;
		case 'z': zjump(m); break;
		case '[': quot(m); break;
		case ']': ret(m); break;

		case 'c': PUSH(m, sizeof(C)); break;
		case '@': PUSH(m, m); break;
		}
	}
}

V inner(M* m) { 
	C rp = m->rp; 
	while(m->rp >= rp && m->ip && !m->err) { 
		step(m); 
	} 
}

V reset_context(M* m) {
	m->dp = 0;
	m->rp = 0;
	m->ip = 0;
	m->err = 0;
}

M* init() {
	M* m = malloc(sizeof(M));
	m->s = malloc(sizeof(S));
	m->s->l = 0;
	reset_context(m);

	return m;
}

V find_name(M* m) { 
	L2(m, C, l, B*, t);
	N* n = m->s->l;
	while (n) {
		printf("Searching for name [%.*s] with name %ld [%.*s]\n", (int)l, t, n, (int)n->l, n->n);
		if (n->l == l && !strncmp((char*)n->n, (char*)t, l)) break;
		n = n->p;
		printf("n is now: %ld\n", n);
	}
	printf("After while...\n");
	PUSH(m, t);
	PUSH(m, l);
	PUSH(m, n);
}

V asm_comp(M* m, C l, char* t) { 
	C i;
	printf("Compiling assembler l: %ld\n", l);
	for (i = 0;i < l; i++) {
		cbyte(m->s, t[i]);
	}
}

V to_number(M* m, C l, char* t) {
	char * end;
	int n = strtol(t, &end, 10);
	if ((n == 0 && end == t) || end < (t + l)) {
		PUSH(m, t);
		PUSH(m, l);
		m->err = -13;
		return;
	}
	PUSH(m, n);
}

V evaluate(M* m, char* s) {
	m->ibuf = s;
	m->ilen = strlen(s);
	m->ipos = 0;
	while (!m->err && m->ipos < m->ilen) {
		printf("m->ipos %ld m->ilen %d\n", m->ipos, m->ilen);
		parse_name(m);
		printf("...m->ipos %ld m->ilen %d\n", m->ipos, m->ilen);
		printf("Stack depth: %ld\n", m->dp);
		printf("T(m) %ld N(m) %ld\n", T(m), N(m));
		if (!T(m)) { DROP(m); DROP(m); return; }
		printf("Pre find name\n");
		find_name(m);
		printf("....Stack depth: %ld\n", m->dp);
		printf("T(m) %ld N(m) %ld NN(m) %ld\n", T(m), N(m), NN(m));
		if (T(m)) {
			L3(m, N*, nt, C, _, char*, __);
			if (!m->st || (nt->f & IMMEDIATE) == IMMEDIATE) eval(m, nt->c);
			else { PUSH(m, nt->c); compile(m); }
		} else {
			L3(m, N*, _, C, l, char*, t);
			if (t[0] == '\\' && t[l - 1] == ']') eval(m, (B*)(t + 1)); 
			else if (t[0] == '$') asm_comp(m, l - 1, t + 1);
			else to_number(m, l, t);
		}
	}
}
*/
/*
V times(X*P* p) { L2(x, C, q, C, n); for (;n > 0; n--) eval(x, q); }
V choose(X*P* p) { L3(x, C, f, C, t, C, b); if (b) eval(x, t); else eval(x, f); }

V block(X*P* p) {
	C t = 1;
	PUSH(x,P* p->ip);
	while (x->ip < DICT_SIZE && t) {
		switch (TOKEN(x)) {
		case '{': t++; break;
		case '}': t--; break;
		}
	}
}

V string(X*P* p) {
	PUSH(x,P* p->m->h +P* p->ip);
	while (x->ip < DICT_SIZE && TOKEN(x) != '"') {}
	PUSH(x, (x->m->h +P* p->ip) - T(x) - 1);
}

V parse_spaces(X*P* p) { while (x->ipos <P* p->ilen && isspace(x->ibuf[x->ipos]))	x->ipos++; }
V parse(X*P* p) { L1(x, C, c); while (x->ipos <P* p->ilen &&P* p->ibuf[x->ipos] != c)P* p->ipos++;P* p->ipos++; }
V parse_non_spaces(X*P* p) { while (x->ipos <P* p->ilen && !isspace(x->ibuf[x->ipos]))P* p->ipos++; }
V parse_name(X*P* p) {
	parse_spaces(x);
	PUSH(x,P* p->ibuf +P* p->ipos);
	parse_non_spaces(x);
	PUSH(x, (x->ibuf +P* p->ipos) - T(x));
}

V find_name(X*P* p) { PUSH(x, 0); }

V asm_comp(X*P* p, C l, B* t) { }

V to_number(X*P* p, C l, B* t) {
	char * end;
	int n = strtol(t, &end, 10);
	if ((n == 0 && end == t) || end < (t + l)) {
		PUSH(x, t);
		PUSH(x, l);
		x->err = -13;
		return;
	}
	PUSH(x, n);
}

V evaluate(X*P* p, B* s) {
	x->ibuf = s;
	x->ilen = strlen(s);
	x->ipos = 0;
	while (!x->err &&P* p->ipos <P* p->ilen) {
		parse_name(x);
		if (!T(x)) { DROP(x); DROP(x); return; }
		find_name(x);
		if (T(x)) {
			L3(x, C, wp, C, _, B*, __);
		} else {
			L3(x, C, _, C, l, B*, t);			
			if (t[0] == '\\') asm_exec(x, l - 1, t + 1);
			else if (t[0] == '$') asm_comp(x, l - 1, t + 1);
			else to_number(x, l, t);
		}
	}
}
*/

#endif
