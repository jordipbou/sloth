/*
TODO: Add error management and "condition" system
TODO: Add register navigation (GPn load registers in G, execute native function in P)
TODO: Check how to use multiple stacks
TODO: Check how to use registers as stacks/locals
TODO: Check how to incorporate types for typed stacks/multiple interpreters?
*/
#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

#ifdef _WIN32
  #include <conio.h>
#else
	#include <unistd.h>
	#include <termios.h>
#endif

typedef int8_t BYTE;
typedef intptr_t CELL;

typedef struct { CELL type, size, length; CELL data[1]; } VECTOR;

struct CONTEXT_S;

typedef void (*FUNC)(struct CONTEXT_S*);

typedef struct CONTEXT_S {
	VECTOR* mstack;
	VECTOR* rstack;
	VECTOR** dstack;
	VECTOR* mregisters;
	VECTOR** registers;
	FUNC* inner;
	BYTE* ip;
} CONTEXT;

#define S(x)				(*x->dstack)

#define TOS(x)			(S(x)->data[S(x)->length - 1])
#define NOS(x)			(S(x)->data[S(x)->length - 2])
#define NNOS(x)			(S(x)->data[S(x)->length - 3])
#define PUSH(x, v)	(S(x)->data[S(x)->length++] = (CELL)v)
#define POP(x)			(S(x)->data[--S(x)->length])

#define DROP(x)			(S(x)->length--)

#define R(x)				(x->rstack)

#define TORS(x)			(R(x)->data[R(x)->length - 1])
#define PUSHR(x, v)	(R(x)->data[R(x)->length++] = (CELL)v)
#define POPR(x)			(R(x)->data[--R(x)->length])

/* TODO: This should use an external allocator. This would allow not accessing out of bounds
memory if using a memory block, for example */
VECTOR* newV(CELL size) {
	VECTOR* vector;

	vector = calloc(size + 3, sizeof(CELL));

	vector->type = 0;
	vector->size = size;
	vector->length = 0;

	return vector;
}

CONTEXT* init() {
	CONTEXT* x = malloc(sizeof(CONTEXT));

	x->mstack = newV(256);
	x->rstack = newV(256);
	x->dstack = &x->mstack;

	x->mregisters = newV(26);
	x->registers = &x->mregisters;

	x->ip = 0;

	return x;
}

void dump(CONTEXT*);

/*
 Source code for getch is taken from:
 Crossline readline (https://github.com/jcwangxp/Crossline).
 It's a fantastic readline cross-platform replacement, but only getch was
 needed and there's no need to include everything else.
*/
#ifdef _WIN32
int _getch (void) {	fflush (stdout); return _getch(); }
#else
int _getch ()
{
	char ch = 0;
	struct termios old_term, cur_term;
	fflush (stdout);
	if (tcgetattr(STDIN_FILENO, &old_term) < 0)	{ perror("tcsetattr"); }
	cur_term = old_term;
	cur_term.c_lflag &= ~(ICANON | ECHO | ISIG); /* echoing off, canonical off, no signal chars */
	cur_term.c_cc[VMIN] = 1;
	cur_term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &cur_term) < 0)	{ perror("tcsetattr"); }
	if (read(STDIN_FILENO, &ch, 1) < 0)	{ /* perror("read()"); */ } /* signal will interrupt */
	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old_term) < 0)	{ perror("tcsetattr"); }
	return ch;
}
#endif

void inner(CONTEXT* x) {
	CELL t;
	CELL* a;
	BYTE opcode, opcode2;

	dump(x);
	while (x->ip != 0) {
		opcode = *x->ip;
		switch (opcode) {
			case '0': PUSH(x, 0); break;
			case '1': PUSH(x, 1); break;
			case '2': PUSH(x, 2); break;
			case '3': PUSH(x, 3); break;

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
			case '~': TOS(x) = ~TOS(x); break;

			case 'd': PUSH(x, TOS(x)); break;
			case 's': t = TOS(x); TOS(x) = NOS(x); NOS(x) = t; break;
			case 'o': PUSH(x, NOS(x)); break;
			case 'r': t = NNOS(x); NNOS(x) = NOS(x); NOS(x) = TOS(x); TOS(x) = t; break;
			case '\\': DROP(x); break;

			case 'k': PUSH(x, _getch()); break;
			case 'e': printf("%c", (char)POP(x)); break;

			case '?': t = 1; if (!POP(x)) { while (t) { x->ip++; if (*x->ip == '(') { t--; } else if (*x->ip == '?') { t++; } } } break;
			case '(': t = 1; while (t) { x->ip++; if (*x->ip == '(') t++; else if (*x->ip == ')') t--; }; break;
			case ')': /* NOOP */ break;

			case '[': /* NOOP */ break;
			case ']': t = 1; while (t) { x->ip--; if (*x->ip == ']') t++; else if (*x->ip == '[') t--; }; break;

			case '{': PUSH(x, x->ip + 1); t = 1; while (t) { x->ip++; if (*x->ip == '{') t++; else if (*x->ip == '}') t--; }; break;
			case '}':
			case ';': if (x->rstack->length > 0) x->ip = (BYTE*)POPR(x); else return; break;

			/* Should nested definitions be allowed here? */
			case ':': /* NOOP */ break;
			case '`': PUSHR(x, x->ip); while (*x->ip != ':') { x->ip--; }; break;

			case '!': a = (CELL*)POP(x); t = POP(x); *a = t; break;
			case '@': TOS(x) = *((CELL*)TOS(x)); break;

			default:
				if (opcode >= 'A' && opcode <= 'Z') {
					/*PUSH(x, &((*(x->registers))->data[opcode - 'A']));*/
					printf("Here!\n");
					opcode2 = *(++x->ip);
					printf("opcode: '%c' opcode2: '%c' \n", opcode, opcode2);
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
						/* TODO */
					}
				}
				break;
		}
		x->ip++;
		dump(x);
	}
}

void dump(CONTEXT* x) {
	CELL i;
	BYTE* j;
	char buf[50];

	buf[0] = 0;
	for (i = 0; i < S(x)->length; i++) {
		sprintf(buf, "%.47s%ld ", buf, S(x)->data[i]);
	}
	printf("%40s||| ", buf);
	for (j = x->ip; *(j-1) != ';'; j++) {
		printf("%c", *(j));
	}
	printf("\n");
}

#endif
