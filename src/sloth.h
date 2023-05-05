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

typedef int8_t BYTE;
typedef intptr_t CELL;

typedef struct { CELL tp, sz, len; CELL dt[1]; } *ARRAY;
/* TODO: Remove byte array */
typedef struct { CELL tp, sz, len; BYTE dt[sizeof(CELL)]; } *BYTE_ARRAY;

#define bSZ(a)	(sz*sizeof(CELL))

typedef struct CONTEXT_S {
	ARRAY s;					/* Data stack */
	ARRAY r;					/* Return stack */
	ARRAY x;					/* Extensions */
	BYTE_ARRAY c;			/* Code */
	BYTE_ARRAY d;			/* Data */
	CELL i;						/* Instruction pointer */
} CONTEXT;

typedef void (*FUNC)(CONTEXT*);

#define S					(ctx->s)
#define R					(ctx->r)
#define X					(ctx->x)
#define C					(ctx->c)
#define D					(ctx->d)

#define TOS				(S->dt[S->len - 1])
#define NOS				(S->dt[S->len - 2])
#define NNOS			(S->dt[S->len - 3])

#define PUSH(v)		(S->dt[S->len++] = (CELL)v)
#define POP				(S->dt[--S->len])
#define DROP			(S->len--)

#define PUSHR(v)	(R->dt[R->len++] = (CELL)v)
#define POPR			(R->dt[--R->len])

#define BC(i)			(C->dt[i])
#define CC(i)			(*((CELL*)&(C->dt[i])))
#define BD(i)			(D->dt[i])
#define CD(i)			(*((CELL*)&(D->dt[i])))

#define IP				(ctx->i)
#define OP				(BC(IP))

#define ERR_OK									0
#define ERR_STACK_OVERFLOW			-1
#define ERR_STACK_UNDERFLOW			-2
#define ERR_DIVISION_BY_ZERO		-3
#define ERR_IP_OUT_OF_BOUNDS		-4
#define ERR_MEM_OUT_OF_BOUNDS		-5
#define ERR_EXIT								-6

#define O1		if (ctx->s->len == ctx->s->sz) { return ERR_STACK_OVERFLOW; }
#define O2		if (ctx->s->len + 1 == ctx->s->sz) { return ERR_STACK_OVERFLOW; }
#define U1		if (ctx->s->len == 0) { return ERR_STACK_UNDERFLOW; }
#define U2		if (ctx->s->len == 1) { return ERR_STACK_UNDERFLOW; }
#define U3		if (ctx->s->len == 2) { return ERR_STACK_UNDERFLOW; }
#define ZD		if (TOS == 0) { return ERR_DIVISION_BY_ZERO; }
#define IOB		if (IP < 0 || IP >= ctx->c->sz) { return ERR_IP_OUT_OF_BOUNDS; }
#define BDOB(a)		if (a < 0 || a >= ctx->d->sz) { return ERR_MEM_OUT_OF_BOUNDS; }
#define CDOB(a)		if (a < 0 || (a+sizeof(CELL)) >= ctx->d->sz) { return ERR_MEM_OUT_OF_BOUNDS; }
#define BCOB(a)		if (a < 0 || a >= ctx->c->sz) { return ERR_MEM_OUT_OF_BOUNDS; }
#define CCOB(a)		if (a < 0 || (a+sizeof(CELL)) >= ctx->c->sz) { return ERR_MEM_OUT_OF_BOUNDS; }

void dump(CONTEXT* ctx) {
	CELL i;
	char buf[50];

	buf[0] = 0;
	for (i = 0; i < ctx->s->len; i++) {
		sprintf(buf, "%.47s%ld ", buf, ctx->s->dt[i]);
	}
	printf("%40s||| ", buf);
	for (i = IP; BC(i - 1) != ';' && BC(i) != 0 && BC(i) != 10; i++) {
		printf("%c", BC(i));
	}
	printf("\n");
}

/* TODO: From this point, only defines should be used to allow modifications on architecture */

#define STEP																																	\
	switch (OP) {																																\
		case '0': O1; PUSH(0); break;																							\
		case '1': O1; PUSH(1); break;																							\
		case 'l': IP++; PUSH(CC(IP)); break;																			\
		/* Arithmetics */																													\
		case '+': U2; NOS += TOS; DROP; break;																		\
		case '-': U2; NOS -= TOS; DROP; break;																		\
		case '*': U2; NOS *= TOS; DROP; break;																		\
		case '/': U2; ZD; NOS /= TOS; DROP; break;																\
		case '%': U2; NOS %= TOS; DROP; break;																		\
		/* Comparisons */																													\
		case '>': U2; NOS = NOS > TOS; DROP; break;																\
		case '<': U2; NOS = NOS < TOS; DROP; break;																\
		case '=': U2; NOS = NOS == TOS; DROP; break;															\
		/* Bits */																																\
		case '&': U2; NOS &= TOS; DROP; break;																		\
		case '|': U2; NOS |= TOS; DROP; break;																		\
		case '!': U1; TOS = !TOS; break;																					\
		case '~': U1; TOS = ~TOS; break;																					\
		/* Stack manipulators */																									\
		case 'd': U1; PUSH(TOS); break;																						\
		case 's': U2; t = TOS; TOS = NOS; NOS = t; break;													\
		case 'o': U2; O1; PUSH(NOS); break;																				\
		case 't': U3; t = NNOS; NNOS = NOS; NOS = TOS; TOS = t; break;						\
		case '\\': U1; DROP; break;																								\
		/* Calls & jumps */																												\
		case 'c': U1; PUSHR(IP); IP = POP - 1; break;															\
		case 'n': U1; ((FUNC)POP)(ctx); break;																		\
		case 'j': U1; IP = POP - 1; break; 																				\
		/* Safe memory access (data region) */																		\
		case 'h': O1; PUSH(ctx->d->len); break;																		\
		case 'a': O1; t = D->len + TOS; BDOB(t); D->len += POP; break;						\
		case 'g': /* TODO: Align here */ break;																		\
		case 'r': U1; BDOB(TOS); PUSH(BD(POP)); break;														\
		case 'w': U2; BDOB(TOS); t = POP; BD(t) = (BYTE)POP; break;								\
		case 'R': U1; CDOB(TOS); PUSH(CD(POP)); break;														\
		case 'W': U2; CDOB(TOS); t = POP; CD(t) = POP; break;											\
		/* Memory access (code region) */																					\
		case '.': U1; BCOB(TOS); PUSH(BC(POP)); break;														\
		case ',': U2; BCOB(TOS); t = POP; BC(t) = (BYTE)POP; break;								\
		case ':': U1; CCOB(TOS); PUSH(CC(POP)); break;														\
		case ';': U2; CCOB(TOS); t = POP; CC(t) = POP; break;											\
																																							\
		/* Helpers */																															\
		case '#': O1; t = 0;																											\
			while ((o = BC(IP+1) - '0',0 <= o && o <= 9)) { t = 10*t + o; IP++; }		\
			PUSH(t);																																\
			break;																																	\
		case '?':	U1; t = 1; if (!POP) {																					\
			while (t) { IP++; IOB; if (OP == '(') t--; if (OP == '?') t++; }				\
		} break;																																	\
		case '(': t = 1;																													\
			while (t) { IP++; IOB; if (OP == '(') t++; if (OP == ')') t--; }				\
			break;																																	\
		case ')': /* NOOP */ break;																								\
		case '[': /* NOOP */ break;																								\
		case ']': t = 1;																													\
			while (t) { IP--; IOB; if (OP == ']') t++; if (OP == '[') t--; }				\
			break;																																	\
		case '{': PUSH(IP + 1); while (OP != '}') IP++; break;										\
		case '}': if (ctx->r->len > 0) IP = POPR; else return; break;							\
		case '`': PUSHR(IP); while (OP != '{') IP--; break;												\
																																							\
		/* Extensions */																													\
		/* TODO: Add as optional: key/emit, fetch/store */ \
		/* TODO: Add as optional: extensions */ \
		/* TODO: Add as optional: stack based local variables (uvwxyz)*/ \
		/* Something like: x> x< x@ */ \
		/* Maybe use extensions to make it easier to get rid of it */ \
	}

CELL inner(CONTEXT* ctx) {
	CELL t, o;

	IOB; while (OP != 0) { STEP; IP++; IOB;	}

	return ERR_OK;
}

CELL trace(CONTEXT* ctx) {
	CELL t, o;

	IOB; dump(ctx); while (OP != 0) { STEP; IP++; IOB; dump(ctx); }

	return ERR_OK;
}

#endif
