/******************************************************************************

	SLOTH								Dual stack virtual machine with human readable bytecode
																																(c) jordipbou

	Inspired on:
		STABLE Forth (https://w3group.de/stable.html)
		RetroForth/ILO	(http://ilo.retroforth.org/)
	
******************************************************************************/

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

typedef int8_t BYTE;
typedef intptr_t CELL;

typedef struct { CELL tp, sz, len; CELL dt[1]; } *ARRAY;
typedef struct { CELL tp, sz, len; BYTE dt[sizeof(CELL)]; } *BYTE_ARRAY;

#define bSZ(a)	(sz*sizeof(CELL))

typedef struct CONTEXT_S {
	ARRAY s;					/* Data stack */
	ARRAY r;					/* Return stack */
	ARRAY x;					/* Extensions */
	BYTE_ARRAY c;			/* Code */
	BYTE_ARRAY d;			/* Data */
	CELL e;						/* Error code */
	CELL i;						/* Instruction pointer */
} CONTEXT;

typedef void (*FUNC)(CONTEXT*);

#define TOS				(ctx->s->dt[ctx->s->len - 1])
#define NOS				(ctx->s->dt[ctx->s->len - 2])
#define NNOS			(ctx->s->dt[ctx->s->len - 3])

#define PUSH(v)		(ctx->s->dt[ctx->s->len++] = (CELL)v)
#define POP				(ctx->s->dt[--ctx->s->len])
#define DROP			(ctx->s->len--)

#define PUSHR(v)	(ctx->r->dt[ctx->r->len++] = (CELL)v)
#define POPR			(ctx->r->dt[--ctx->r->len])

#define C					(((BYTE*)(ctx->c->dt)))
#define D					(ctx->d)
#define BD(i)			(D->dt[i])
#define CD(i)			(*((CELL*)&(D->dt[i])))

#define IP				(ctx->i)
#define OP				(C[IP])

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
#define BOB(a)		if (a < 0 || a >= ctx->d->sz) { return ERR_MEM_OUT_OF_BOUNDS; }
#define COB(a)		if (a < 0 || (a+sizeof(CELL)) >= ctx->d->sz) { return ERR_MEM_OUT_OF_BOUNDS; }

void dump(CONTEXT* ctx) {
	CELL i;
	char buf[50];

	buf[0] = 0;
	for (i = 0; i < ctx->s->len; i++) {
		sprintf(buf, "%.47s%ld ", buf, ctx->s->dt[i]);
	}
	printf("%40s||| ", buf);
	for (i = IP; C[i - 1] != ';' && C[i] != 0 && C[i] != 10; i++) {
		printf("%c", C[i]);
	}
	printf("\n");
}

#define STEP																																	\
	switch (OP) {																																\
		case '0': O1; PUSH(0); break;																							\
		case '1': O1; PUSH(1); break;																							\
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
		case 'c': U1; IP = POP - 1; break;																				\
		case 'n': break; 																													\
		case 'j': break; 																													\
		/* Safe memory access (data region) */																		\
		case 'h': O1; PUSH(ctx->d->len); break;																		\
		case 'a': O1; t = D->len + TOS; BOB(t); D->len += POP; break;							\
		case 'l': /* TODO: Align here */ break;																		\
		case 'r': U1; BOB(TOS); PUSH(BD(POP)); break;															\
		case 'w': U2; BOB(TOS); t = POP; BD(t) = (BYTE)POP; break;								\
		case 'R': U1; COB(TOS); PUSH(CD(POP)); break;															\
		case 'W': U2; COB(TOS); t = POP; CD(t) = POP;  break;											\
																																							\
		/* Helpers */																															\
		case '#': O1; t = 0;																											\
			while ((o = C[IP+1] - '0',0 <= o && o <= 9)) { t = 10*t + o; IP++; }		\
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
		case '`': PUSHR(IP); while (OP != ':') IP--; break;												\
		case ':': PUSH(IP + 1); while (OP != ';') IP++; break;										\
		case ';': if (ctx->r->len > 0) IP = POPR; else return; break;							\
																																							\
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

/*

void inner(CONTEXT* x) {
	CELL t;
	CELL* a;
	BYTE opcode, opcode2;
	char* endptr;

	dump(x);
	while (x->ip != 0 && *(x->ip) != 0) {
		opcode = *x->ip;
		switch (opcode) {
			case '0': PUSH(x, 0); break;
			case '1': PUSH(x, 1); break;

			case '#':	t = strtoimax(++x->ip, &endptr, 0); x->ip = (BYTE*)endptr - 1; PUSH(x, t); break;

			case '+': NOS(x) += TOS(x); DROP(x); break;
			case '-': NOS(x) -= TOS(x); DROP(x); break;
			case '*': NOS(x) *= TOS(x); DROP(x); break;
			case '/': NOS(x) /= TOS(x); DROP(x); break;
			case '%': NOS(x) %= TOS(x); DROP(x); break;

			case '>': NOS(x) = NOS(x) > TOS(x); DROP(x); break;
			case '<': NOS(x) = NOS(x) < TOS(x); DROP(x); break;
			case '=': NOS(x) = NOS(x) == TOS(x); DROP(x); break;

			case '&': NOS(x) = NOS(x) & TOS(x); DROP(x); break;
			case '|': NOS(x) = NOS(x) | TOS(x); DROP(x); break;
			case '~': TOS(x) = !TOS(x); break;

			case 'd': PUSH(x, TOS(x)); break;
			case 's': t = TOS(x); TOS(x) = NOS(x); NOS(x) = t; break;
			case 'o': PUSH(x, NOS(x)); break;
			case 'r': t = NNOS(x); NNOS(x) = NOS(x); NOS(x) = TOS(x); TOS(x) = t; break;
			case '\\': DROP(x); break;

			case 'k': PUSH(x, _getch()); break;
			case 'e': printf("%c", (char)POP(x)); break;

			case '?': t = 1; if (!POP(x)) { while (t) { x->ip++; if (*x->ip == '(') { t--; } else if (*x->ip == '?') { t++; } } } break;
			case '(': t = 1; while (t) { x->ip++; if (*x->ip == '(') t++; else if (*x->ip == ')') t--; }; break;
			case ')':  NOOP  break;

			case '[':  NOOP  break;
			case ']': t = 1; while (t) { x->ip--; if (*x->ip == ']') t++; else if (*x->ip == '[') t--; }; break;

			case '{': PUSH(x, x->ip + 1); t = 1; while (t) { x->ip++; if (*x->ip == '{') t++; else if (*x->ip == '}') t--; }; break;
			case '}':
			case ';': if (x->rstack->length > 0) x->ip = (BYTE*)POPR(x); else return; break;

			case ':':  NOOP  break;
			case '`': PUSHR(x, x->ip); while (*x->ip != ':') { x->ip--; }; break;

			case '!': a = (CELL*)POP(x); t = POP(x); *a = t; break;
			case '@': TOS(x) = *((CELL*)TOS(x)); break;

			default:
				if (opcode >= 'A' && opcode <= 'Z') {
					opcode2 = *(++x->ip);
					if (opcode2 == '!') {
						(*(x->registers))->data[opcode - 'A'] = POP(x);
					} else if (opcode2 == '@') {
						PUSH(x, (*(x->registers))->data[opcode - 'A']);
					} else if (opcode2 == 'c') {
						PUSHR(x, x->ip); 
						x->ip = ((BYTE*)((*(x->registers))->data[opcode - 'A'])) - 1;
					} else if (opcode2 == 'n') {
						((FUNC)((*(x->registers))->data[opcode - 'A']))(x);
					} else if (opcode2 >= 'A' && opcode2 <= 'Z') {
					}
				}
				break;
		}
		x->ip++;
		dump(x);
	}
}
*/

#endif
