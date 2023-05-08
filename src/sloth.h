/******************************************************************************

	SLOTH								Dual stack virtual machine with human readable bytecode
																																 by jordipbou
	Inspired by:
		STABLE Forth (https://w3group.de/stable.html)
		RetroForth/ILO	(http://ilo.retroforth.org/)
		XY (https://nsl.com/k/xy/xy.htm)
	
******************************************************************************/

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

typedef int8_t B;
typedef intptr_t C;

typedef struct { C s, l; C d[1]; } *A;
typedef struct { C s, l; B d[1]; } *BA;

#define SZA		((sizeof(A) / sizeof(C)) + (sizeof(A) % sizeof(C) == 0 ? 0 : 1))

A a_alloc(C n) { A a = (A)calloc(SZA + n - 1, sizeof(C)); a->s = n; a->l = 0; return a; }
A a_from(B* b, C s) { A a = (A)b; a->s = s - SZA + 1; a->l = 0; return a; }
BA ba_alloc(C n) { BA a = (BA)calloc(sizeof(BA) + n - 1, sizeof(B)); a->s = n; a->l = 0; return a; }
BA ba_from(B* b, C s) { BA a = (BA)b; a->s = s - sizeof(BA) + 1; a->l = 0; return a; }

typedef struct { A s, r, e; BA c, d; C i; } X;

typedef void (*F)(X*);

#define DEPTH(x)			(x->s->l)
#define MAX(x)				(x->s->s)

#define TOS(x)				(x->s->d[x->s->l - 1])
#define NOS(x)				(x->s->d[x->s->l - 2])
#define NNOS(x)				(x->s->d[x->s->l - 3])

#define PEEK(x, i)		(x->s->d[i])
#define DROP(x)				(--x->s->l)
#define PUSH(x, v)		(x->s->d[x->s->l++] = (C)v)
#define POP(x)				(x->s->d[--x->s->l])

#define DEPTHR(x)			(x->r->l)

#define PUSHR(x, v)		(x->r->d[x->r->l++] = (C)v)
#define POPR(x)				(x->r->d[--x->r->l])

#define AS_C(a)				(*((C*)(&a)))

#define HERE(x)				(x->d->l)
#define AT(x, i)			(x->d->d[i])
#define DATA_SIZE(x)	(x->d->s)

#define IP(x)					(x->i)
#define OP(x, i)			(x->c->d[i])
#define CODE_SIZE(x)	(x->c->s)


#define ERR_OK									0
#define ERR_STACK_OVERFLOW			-1
#define ERR_STACK_UNDERFLOW			-2
#define ERR_DIVISION_BY_ZERO		-3
#define ERR_IP_OUT_OF_BOUNDS		-4
#define ERR_MEM_OUT_OF_BOUNDS		-5
#define ERR_EXIT								-6

#define O1(x)				if (DEPTH(x) == MAX(x)) { return ERR_STACK_OVERFLOW; }
#define O2(x)				if (DEPTH(x) + 1 == MAX(x)) { return ERR_STACK_OVERFLOW; }
#define U1(x)				if (DEPTH(x) == 0) { return ERR_STACK_UNDERFLOW; }
#define U2(x)				if (DEPTH(x) == 1) { return ERR_STACK_UNDERFLOW; }
#define U3(x)				if (DEPTH(x) == 2) { return ERR_STACK_UNDERFLOW; }
#define ZD(x)				if (TOS(x) == 0) { return ERR_DIVISION_BY_ZERO; }
#define IOB(x)			if (IP(x) < 0 || IP(x) >= CODE_SIZE(x)) { return ERR_IP_OUT_OF_BOUNDS; }
#define BDOB(x, a)	if (a < 0 || a >= DATA_SIZE(x)) { return ERR_MEM_OUT_OF_BOUNDS; }
#define CDOB(x, a)	if (a < 0 || (a + sizeof(C)) >= DATA_SIZE(x)) { return ERR_MEM_OUT_OF_BOUNDS; }
#define BCOB(x, a)	if (a < 0 || a >= CODE_SIZE(x)) { return ERR_MEM_OUT_OF_BOUNDS; }
#define CCOB(x, a)	if (a < 0 || (a + sizeof(C)) >= CODE_SIZE(x)) { return ERR_MEM_OUT_OF_BOUNDS; }

char* dump_stack(char* s, X* x) {
	C i;
	for (i = 0; i < DEPTH(x); i++) {
		sprintf(s, "%s%ld ", s, PEEK(x, i));
	}
	return s;
}

char* dump_rstack(char* s, X* x) {
	C i, j;
	for (i = IP(x); OP(x, i - 1) != ';' && OP(x, i) != 0 && OP(x, i) != 10; i++) {
		if (OP(x, i) == 'l') {
			#if INTPTR_MAX == INT64_MAX
			sprintf(s, "%sl %08x ", s, (unsigned int)*((C*)&(OP(x, i + 1))));
			#elif INTPTR_MAX == INT32_MAX
			sprintf(s, "%sl %04x ", s, (unsigned int)*((C*)&(OP(x, i + 1))));
			#elif INTPTR_MAX == INT16_MAX
			sprintf(s, "%sl %02x ", s, (unsigned int)*((C*)&(OP(x, i + 1))));
			#endif
			i += sizeof(C);
		} else {
			sprintf(s, "%s%c", s, OP(x, i));
		}
	}
	return s;
}

char* dump(char* s, X* x) {
	s = dump_stack(s, x);
	sprintf(s, "%s: ", s);
	s = dump_rstack(s, x);
	return s;
}

#define STEP \
	switch (OP(x, IP(x))) {	\
		case '0': O1(x); PUSH(x, 0); break;	\
		case '1': O1(x); PUSH(x, 1); break;	\
		case 'l': PUSH(x, AS_C(OP(x, IP(x) + 1))); IP(x) += sizeof(C); break; \
		/* Arithmetics */	\
		case '+': U2(x); NOS(x) += TOS(x); DROP(x); break;	\
		case '-': U2(x); NOS(x) -= TOS(x); DROP(x); break;	\
		case '*': U2(x); NOS(x) *= TOS(x); DROP(x); break;	\
		case '/': U2(x); ZD(x); NOS(x) /= TOS(x); DROP(x); break; \
		case '%': U2(x); NOS(x) %= TOS(x); DROP(x); break;	\
		/* Comparisons */	\
		case '>': U2(x); NOS(x) = NOS(x) > TOS(x); DROP(x); break;	\
		case '<': U2(x); NOS(x) = NOS(x) < TOS(x); DROP(x); break;	\
		case '=': U2(x); NOS(x) = NOS(x) == TOS(x); DROP(x); break; \
		/* Bits */ \
		case '&': U2(x); NOS(x) &= TOS(x); DROP(x); break; \
		case '|': U2(x); NOS(x) |= TOS(x); DROP(x); break;	\
		case '!': U1(x); TOS(x) = !TOS(x); break;	\
		case '~': U1(x); TOS(x) = ~TOS(x); break;	\
		/* Stack manipulators */ \
		case 'd': U1(x); PUSH(x, TOS(x)); break; \
		case 's': U2(x); t = TOS(x); TOS(x) = NOS(x); NOS(x) = t; break; \
		case 'o': U2(x); O1(x); PUSH(x, NOS(x)); break; \
		case 't': U3(x); t = NNOS(x); NNOS(x) = NOS(x); NOS(x) = TOS(x); TOS(x) = t; break; \
		case '\\': U1(x); DROP(x); break; \
		/* Calls & jumps */ \
		case 'j': U1(x); IP(x) = POP(x) - 1; break; \
		case 'c': U1(x); PUSHR(x, IP(x)); IP(x) = POP(x) - 1; break;	\
		case 'n': U1(x); ((F)POP(x))(x); break; \
		case 'z': U2(x); if (NOS(x)) { DROP(x); } else { IP(x) = POP(x) - 1; } DROP(x); break; \
		/* Safe memory access (data region) */ \
		case 'h': O1(x); PUSH(x, HERE(x)); break;	\
		case 'a': O1(x); t = HERE(x) + TOS(x); BDOB(x, t); HERE(x) += POP(x); break;	\
		case 'g': /* TODO: Align here */ break;	\
		case 'r': U1(x); BDOB(x, TOS(x)); PUSH(x, AT(x, POP(x))); break;	\
		case 'w': U2(x); BDOB(x, TOS(x)); t = POP(x); AT(x, t) = (B)POP(x); break;	\
		case 'R': U1(x); CDOB(x, TOS(x)); PUSH(x, AS_C(AT(x, POP(x)))); break; \
		case 'W': U2(x); CDOB(x, TOS(x)); t = POP(x); AS_C(AT(x, t)) = POP(x); break; \
		/* Memory access (code region) */	\
		case '.': U1(x); BCOB(x, TOS(x)); PUSH(x, OP(x, POP(x))); break;	\
		case ',': U2(x); BCOB(x, TOS(x)); t = POP(x); OP(x, t) = (B)POP(x); break;	\
		case ':': U1(x); CCOB(x, TOS(x)); PUSH(x, AS_C(OP(x, POP(x)))); break;	\
		case ';': U2(x); CCOB(x, TOS(x)); t = POP(x); AS_C(OP(x, t)) = POP(x); break;	\
		/* Helpers */	\
		case '#':	\
			O1(x); \
			t = 0; \
			while ((o = OP(x, IP(x) + 1) - '0', 0 <= o && o <= 9)) { \
				t = 10*t + o; \
				IP(x)++; \
			}	\
			PUSH(x, t); \
			break; \
		case '?': \
			U1(x); \
			t = 1; \
			if (!POP(x)) { \
				while (t) { \
					IP(x)++; \
					IOB(x);	\
					if (OP(x, IP(x)) == '(') t--; \
					if (OP(x, IP(x)) == '?') t++; \
				} \
			} \
			break; \
		case '(':	\
			t = 1; \
			while (t) { \
				IP(x)++; \
				IOB(x); \
				if (OP(x, IP(x)) == '(') t++; \
				if (OP(x, IP(x)) == ')') t--; \
			} \
			break; \
		case ')': /* NOOP */ break; \
		case '{': /* NOOP */ break;	\
		case '}': \
			t = 1; \
			while (t) { \
				IP(x)--; \
				IOB(x); \
				if (OP(x, IP(x)) == '}') t++; \
				if (OP(x, IP(x)) == '{') t--; \
			} \
			break; \
		case '[': PUSH(x, IP(x) + 1); while (OP(x, IP(x)) != '[') IP(x)++; break; \
		case ']': if (DEPTHR(x) > 0) IP(x) = POPR(x); else return ERR_OK; break; \
		case '`': PUSHR(x, IP(x)); while (OP(x, IP(x)) != '[') IP(x)--; break; \
	}

C step(X* x) {
	C t, o;

	IOB(x); STEP(x); IP(x)++; 

	return ERR_OK;
}

C inner(X* x) {
	C t, o;

	IOB(x); while (OP(x, IP(x)) != 0) { STEP(x); IP(x)++; IOB(x); }

	return ERR_OK;
}

C trace(X* x) {
	char buf[255];
	C t, o;

	IOB(x); 
	buf[0] = 0; printf("%40s: ", dump_stack(buf, x)); buf[0] = 0; printf("%s\n", dump_rstack(buf, x));
	while (OP(x, IP(x)) != 0) { 
		STEP(x); 
		IP(x)++; 
		IOB(x); 
		buf[0] = 0; printf("%40s: ", dump_stack(buf, x)); buf[0] = 0; printf("%s\n", dump_rstack(buf, x));
	}

	return ERR_OK;
}

/* API */

X* init() {
	X* x = (X*)malloc(sizeof(X));

	x->s = a_alloc(256);
	x->r = a_alloc(256);

	x->e = a_alloc(26);

	x->c = ba_alloc(2048);
	x->d = ba_alloc(2048);

	IP(x) = 0;

	return x;
}

#endif
