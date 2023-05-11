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
#include<string.h>
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

typedef struct { A s, r, e; BA d; C i, t; } X;

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
#define OP(x, a)			(*((B*)a))
/* #define OP(x, i)			(x->c->d[i]) */
/* #define CODE_SIZE(x)	(x->c->s) */

#define TRACE(x)			(x->t)

#define EX(x, l)			(x->e->d[l - 'A'])

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
#define IOB(x)			if (IP(x) == 0) { return ERR_IP_OUT_OF_BOUNDS; }
#define BDOB(x, a)	if (a < 0 || a >= DATA_SIZE(x)) { return ERR_MEM_OUT_OF_BOUNDS; }
#define CDOB(x, a)	if (a < 0 || (a + sizeof(C)) >= DATA_SIZE(x)) { return ERR_MEM_OUT_OF_BOUNDS; }

char* dump_stack(char* s, X* x, C b) {
	C i;
	for (i = 0; i < DEPTH(x); i++) {
		if (b) {
			#if INTPTR_MAX == INT64_MAX
			sprintf(s, "%s%ld\n", s, PEEK(x, i));
			#else
			sprintf(s, "%s%d\n", s, PEEK(x, i));
			#endif
		} else {
			#if INTPTR_MAX == INT64_MAX
			sprintf(s, "%s%ld ", s, PEEK(x, i));
			#else
			sprintf(s, "%s%d ", s, PEEK(x, i));
			#endif
		}
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

char* dump(char* s, X* x, C f) {
	char buf[255];
	char i;
	if (f) {
		buf[0] = 0;
		sprintf(buf, "%s", dump_stack(buf, x, 0));
		for (i = 0; i < f - strlen(buf); i++) { s[i] = ' '; }
		s[f - strlen(buf)] = 0;
	}
	sprintf(s, "%s: ", dump_stack(s, x, 0));
	s = dump_rstack(s, x);
	if (f) sprintf(s, "%s\n", s);
	return s;
}

#define STEP \
	switch (OP(x, IP(x))) {	\
		case '0': /* Literal two */ O1(x); PUSH(x, 0); break;	\
		case '1': /* Literal one */ O1(x); PUSH(x, 1); break;	\
		case 'r': /* Relative literal -to next IP- */ PUSH(x, OP(x, ++IP(x)) + (IP(x) + 1)); break; \
		case 'l': /* Literal */ PUSH(x, AS_C(OP(x, IP(x) + 1))); IP(x) += sizeof(C); break; \
		/* Arithmetics */	\
		case '+': /* add */ U2(x); NOS(x) += TOS(x); DROP(x); break;	\
		case '-': /* subtract */ U2(x); NOS(x) -= TOS(x); DROP(x); break;	\
		case '*': /* mul */ U2(x); NOS(x) *= TOS(x); DROP(x); break;	\
		case '/': /* div */ U2(x); ZD(x); NOS(x) /= TOS(x); DROP(x); break; \
		case '%': /* mod */ U2(x); NOS(x) %= TOS(x); DROP(x); break;	\
		/* Comparisons */	\
		case '>': /* greater than */ U2(x); NOS(x) = NOS(x) > TOS(x); DROP(x); break;	\
		case '<': /* less than */ U2(x); NOS(x) = NOS(x) < TOS(x); DROP(x); break;	\
		case '=': /* equal */ U2(x); NOS(x) = NOS(x) == TOS(x); DROP(x); break; \
		/* Bits */ \
		case '&': /* and */ U2(x); NOS(x) &= TOS(x); DROP(x); break; \
		case '|': /* or */ U2(x); NOS(x) |= TOS(x); DROP(x); break;	\
		case '!': /* not */ U1(x); TOS(x) = !TOS(x); break;	\
		case '~': /* invert */ U1(x); TOS(x) = ~TOS(x); break;	\
		/* Stack manipulators */ \
		case 'd': /* dup */ U1(x); PUSH(x, TOS(x)); break; \
		case 's': /* swap */ U2(x); t = TOS(x); TOS(x) = NOS(x); NOS(x) = t; break; \
		case 'o': /* over */ U2(x); O1(x); PUSH(x, NOS(x)); break; \
		case '@': /* rot */ U3(x); t = NNOS(x); NNOS(x) = NOS(x); NOS(x) = TOS(x); TOS(x) = t; break; \
		case '\\': U1(x); DROP(x); break; \
		/* Calls & jumps */ \
		case 'j': /* Jump (absolute) */ U1(x); IP(x) = POP(x) - 1; break; \
		case 'c': /* Call */ U1(x); PUSHR(x, IP(x)); IP(x) = POP(x) - 1; break;	\
		case 'n': /* Native (C) call */ U1(x); ((F)POP(x))(x); break; \
		case 'z': /* 0 jump */ U2(x); IP(x) = NOS(x) ? (DROP(x), IP(x)) : POP(x) - 1; DROP(x); break; \
		/* Safe memory access (data region) */ \
		case 'h': /* HERE */ O1(x); PUSH(x, HERE(x)); break;	\
		case 'a': /* Allocate */ O1(x); t = HERE(x) + TOS(x); BDOB(x, t); HERE(x) += POP(x); break;	\
		case 'g': /* Align */ /* TODO: Align here */ break;	\
		case ':': /* Byte read */ U1(x); BDOB(x, TOS(x)); PUSH(x, AT(x, POP(x))); break; \
		case ';': /* Byte write */ U2(x); BDOB(x, TOS(x)); t = POP(x); AT(x, t) = (B)POP(x); break; \
		case '.': /* Cell read */ U1(x); CDOB(x, TOS(x)); PUSH(x, AS_C(AT(x, POP(x)))); break; \
		case ',': /* Cell write */ U2(x); CDOB(x, TOS(x)); t = POP(x); AS_C(AT(x, t)) = POP(x); break; \
		/* REPL */ \
		case '$': /* Trace on/off */ U1(x); TRACE(x) = POP(x); break; \
		case 'q': /* Quit repl */ return ERR_EXIT; break; \
		/* Helpers */	\
		case '^': /* Recurse */ PUSHR(x, IP(x)); while (OP(x, IP(x)) != '[') IP(x)--; break; \
		case '#':	/* Parsed number literal */ \
			O1(x); \
			t = 0; \
			while ((o = OP(x, IP(x) + 1) - '0', 0 <= o && o <= 9)) { \
				t = 10*t + o; \
				IP(x)++; \
			}	\
			PUSH(x, t); \
			break; \
		case '?': /* If */ \
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
		case '(':	/* Else */ \
			t = 1; \
			while (t) { \
				IP(x)++; \
				IOB(x); \
				if (OP(x, IP(x)) == '(') t++; \
				if (OP(x, IP(x)) == ')') t--; \
			} \
			break; \
		case ')': /* Then */ /* NOOP */ break; \
		case '{': /* Start of loop */ /* NOOP */ break;	\
		case '}': /* End of loop */ \
			t = 1; \
			while (t) { \
				IP(x)--; \
				IOB(x); \
				if (OP(x, IP(x)) == '}') t++; \
				if (OP(x, IP(x)) == '{') t--; \
			} \
			break; \
		case '[': /* Block */ PUSH(x, IP(x) + 1); while (OP(x, IP(x)) != ']') IP(x)++; break; \
		case ']': /* End of block */ if (DEPTHR(x) > 0) IP(x) = POPR(x); else return ERR_OK; break; \
		/* Extensions */ \
		default: o = OP(x, IP(x)); if ('A' <= o && o <= 'Z') { ((F)EX(x, o))(x); } break; \
	}

/* Not used right now, but useful for debugging? */
C step(X* x) { C t, o; IOB(x); STEP(x); IP(x)++; return ERR_OK; }

void print(X* x, char* s) { while (*s != 0) { PUSH(x, *s); ((F)(EX(x, 'E')))(x); s++; } }

C inner(X* x) {
	char buf[255];
	C t, o, e;

	IOB(x); 
	if (TRACE(x)) {
		buf[0] = 0; 
		print(x, dump(buf, x, 40));
		while (OP(x, IP(x)) != 0) { 
			STEP(x); 
			IP(x)++; 
			IOB(x); 
			buf[0] = 0;
			print(x, dump(buf, x, 40));
		}
	} else {
		while (OP(x, IP(x)) != 0) { 
			STEP(x); 
			IP(x)++; 
			IOB(x); 
		}
	}

	return ERR_OK;
}

/* API */

X* init() {
	X* x = (X*)malloc(sizeof(X));

	x->s = a_alloc(256);
	x->r = a_alloc(256);

	x->e = a_alloc(26);

	x->d = ba_alloc(2048);

	IP(x) = 0;
	TRACE(x) = 0;

	return x;
}

C repl(X* x) {
	char buf[255];
	C i;

	do {
		print(x, "IN: ");
		/* TODO: This depends on fget and not on key !!! */
		IP(x) = (C)fgets(buf, 255, stdin);
		i = inner(x);
		if (!TRACE(x) && DEPTH(x) != 0) { 
			buf[0] = 0; 
			sprintf(buf, "%s", dump_stack(buf, x, 1));
			print(x, buf);
		}
		if (i == ERR_EXIT) { return ERR_EXIT; }
		if (i != ERR_OK) { 
			buf[0] = 0; 
			sprintf(buf, "ERROR: %ld\n", i);
			print(x, buf);
		}
	} while(1);
}

#endif
