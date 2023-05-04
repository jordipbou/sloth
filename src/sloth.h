#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

typedef int8_t BYTE;
typedef intptr_t CELL;

typedef struct { CELL type, size, length; CELL data[1]; } ARRAY;

typedef struct CONTEXT_S {
	ARRAY* data_stack;
	ARRAY* call_stack;
	/* TODO: Make extensions optional, will save pointer size bytes */
	ARRAY* extensions;
	/* DOUBT: Can code be optional? */
	ARRAY* code;
	/* Make contiguous data region optional, will save pointer size bytes */
	ARRAY* data;
	BYTE err;
	BYTE* ip;
} CONTEXT;

typedef void (*FUNC)(CONTEXT*);

#define S(x)				(x->data_stack)

#define TOS(x)			(S(x)->data[S(x)->length - 1])
#define NOS(x)			(S(x)->data[S(x)->length - 2])
#define NNOS(x)			(S(x)->data[S(x)->length - 3])

#define PUSH(x, v)	(S(x)->data[S(x)->length++] = (CELL)v)
#define POP(x)			(S(x)->data[--S(x)->length])
#define DROP(x)			(S(x)->length--)

#define C(x)				(x->call_stack)

#define TORS(x)			(C(x)->data[C(x)->length - 1])
#define PUSHR(x, v)	(C(x)->data[C(x)->length++] = (CELL)v)
#define POPR(x)			(C(x)->data[--R(x)->length])

#define E(x)				(x->extensions)

void dump(CONTEXT* x) {
	CELL i;
	BYTE* j;
	char buf[50];

	buf[0] = 0;
	for (i = 0; i < S(x)->length; i++) {
		sprintf(buf, "%.47s%ld ", buf, S(x)->data[i]);
	}
	printf("%40s||| ", buf);
	for (j = x->ip; *(j-1) != ';' && *j != 0; j++) {
		printf("%c", *(j));
	}
	printf("\n");
}

#define ERR_OK									0
#define ERR_STACK_OVERFLOW			-1
#define ERR_STACK_UNDERFLOW			-2
#define ERR_DIVISION_BY_ZERO		-3
#define ERR_EXIT								-4

#define ERR(x, c, e)						if (c) { t = error(x, e); if (t) { return t; } }

#define OF(x, n)								ERR(x, S(x)->length + n > S(x)->size, ERR_STACK_OVERFLOW)
#define UF(x, n)								ERR(x, S(x)->length < n, ERR_STACK_UNDERFLOW)
#define ZD(x)										ERR(x, TOS(x) == 0, ERR_DIVISION_BY_ZERO)

/* Errors can be used to end calculations on loops affecting the stack!! */
CELL error(CONTEXT* x, CELL error) {
	switch (error) {
		case ERR_STACK_OVERFLOW: printf("ERROR: Stack overflow\n"); dump(x); break;
		case ERR_STACK_UNDERFLOW: printf("ERROR: Stack underflow\n"); dump(x); break;
		case ERR_DIVISION_BY_ZERO: printf("ERROR: Division by zero\n"); dump(x); break;
	}
	return error;
}

#define STEP(x) \
	switch (*x->ip) { \
		case '0': OF(x, 1); PUSH(x, 0); break; \
		case '1': OF(x, 1); PUSH(x, 1); break; \
		/* Arithmetics */ \
		case '+': UF(x, 2); NOS(x) += TOS(x); DROP(x); break; \
		case '-': UF(x, 2); NOS(x) -= TOS(x); DROP(x); break; \
		case '*': UF(x, 2); NOS(x) *= TOS(x); DROP(x); break; \
		case '/': UF(x, 2); ZD(x); NOS(x) /= TOS(x); DROP(x); break; \
		case '%': UF(x, 2); NOS(x) %= TOS(x); DROP(x); break; \
		/* Comparisons */ \
		case '>': UF(x, 2); NOS(x) = NOS(x) > TOS(x); DROP(x); break; \
		case '<': UF(x, 2); NOS(x) = NOS(x) < TOS(x); DROP(x); break; \
		case '=': UF(x, 2); NOS(x) = NOS(x) == TOS(x); DROP(x); break; \
		/* Bits */ \
		case '&': UF(x, 2); NOS(x) &= TOS(x); DROP(x); break; \
		case '|': UF(x, 2); NOS(x) |= TOS(x); DROP(x); break; \
		case '~': UF(x, 1); TOS(x) = !TOS(x); break; \
		/* Stack manipulators */ \
		case 'd': UF(x, 1); PUSH(x, TOS(x)); break; \
		case 's': UF(x, 2); t = TOS(x); TOS(x) = NOS(x); NOS(x) = t; break; \
		case 'o': UF(x, 2); OF(x, 1); PUSH(x, NOS(x)); break; \
		case 'r': UF(x, 3); t = NNOS(x); NNOS(x) = NOS(x); NOS(x) = TOS(x); TOS(x) = t; break; \
		case '\\': UF(x, 1); DROP(x); break; \
		/* Calls & jumps */ \
		case 'c': break; \
		case 'n': break; \
		case 'j': break; \
		/* Control flow helpers */ \
		case '?': \
			t = 1; \
			if (!POP(x)) { \
				while (t) { \
					x->ip++; \
					/* TODO: Add error if x->ip > code+length */ \
					if (*x->ip == '(') { t--; } \
					else if (*x->ip == '?') { t++; } \
				} \
			} \
			break; \
		case '(': \
			t = 1; \
			while (t) { \
				x->ip++; \
				/* TODO: Add error if x->ip > code+length */ \
				if (*x->ip == '(') { t++; } \
				else if (*x->ip == ')') { t--; } \
			} \
			break; \
		case ')': /* NOOP */ break; \
		case '[': /* NOOP */ break; \
		case ']': \
			t = 1; \
			while (t) { \
				x->ip--; \
				/* TODO: Add error if x->ip <= 0 */ \
				if (*x->ip == ']') { t++; } \
				else if (*x->ip == '[') { t--; } \
			} \
			break; \
		/* TODO: Add as optional: key/emit, fetch/store */ \
		/* TODO: Add as optional: extensions */ \
		/* TODO: Add as optional: stack based local variables (uvwxyz)*/ \
		/* Something like: x> x< x@ */ \
		/* Maybe use extensions to make it easier to get rid of it */ \
	}

CELL inner(CONTEXT* x) {
	CELL t;

	while (x->ip != 0 && *(x->ip) != 0) {
		/* TODO: Add error if x->ip > code+length */ \
		STEP(x);
		x->ip++;
	}

	return ERR_OK;
}

CELL trace(CONTEXT* x) {
	CELL t;

	dump(x);
	while (x->ip != 0 && *(x->ip) != 0) {
		/* TODO: Add error if x->ip > code+length */ \
		STEP(x);
		x->ip++;
		dump(x);
	}

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
