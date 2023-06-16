#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>

typedef char B;
typedef intptr_t I;

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct { I s[STACK_SIZE]; I sp; B* r[RSTACK_SIZE]; I rp; } X;

#define FRAME(x) (x->rp - 1)
#define IP(x) (x->r[x->rp - 1])
#define NEXT(x) (x->r[x->rp - 1]++)

B S_peek(X* x) { return IP(x) == 0 ? 0 : *(IP(x)); }
B S_token(X* x) { B ip = S_peek(x); NEXT(x); return ip; }
I S_is_digit(B c) { return c >= '0' && c <= '9'; }

void S_push_R(X* x, B* a) { x->r[x->rp++] = a; }
I S_return(X* x, I f) { x->rp--; return x->rp == f; }

void S_inner(X* x);

void S_eval(X* x, B* q) { 
	if (S_peek(x) && S_peek(x) != ']') { S_push_R(x, IP(x)); } 
	IP(x) = q;
	S_inner(x);
}

void S_push(X* x, I v) { x->s[x->sp++] = v; }
I S_pop(X* x) { return x->s[--x->sp]; }

X* S_init() {
	X* x = malloc(sizeof(X));
	x->sp = x->rp = 0;
	return x;
}

void S_add(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a + b); }
void S_sub(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a - b); }
void S_mul(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a * b); }
void S_div(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a / b); }
void S_mod(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a % b); }

void S_lt(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a < b); }
void S_eq(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a == b); }
void S_gt(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a > b); }

void S_and(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a & b); }
void S_or(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a | b); }
void S_not(X* x) { I a = S_pop(x); S_push(x, !a); }

void S_swap(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, b); S_push(x, a); }
void S_dup(X* x) { I a = S_pop(x); S_push(x, a); S_push(x, a); }
void S_rot(X* x) { I c = S_pop(x); I b = S_pop(x); I a = S_pop(x); S_push(x, b); S_push(x, a); S_push(x, c); }
void S_over(X* x) { I b = S_pop(x); I a = S_pop(x); S_push(x, a); S_push(x, b); S_push(x, a); }
void S_drop(X* x) { S_pop(x); }

void S_exec_i(X* x) { B* q = (B*)S_pop(x); S_eval(x, q); }
void S_exec_x(X* x) { B* q = (B*)S_pop(x); S_push(x, (I)q); S_eval(x, q); }

void S_ifthen(X* x) { B* f = (B*)S_pop(x); B* t = (B*)S_pop(x); if (S_pop(x)) S_eval(x, t); else S_eval(x, f); }
void S_times(X* x) { B* q = (B*)S_pop(x); I t = S_pop(x); for (;t > 0; t--) { S_eval(x, q); } }

void S_br(X* x, B* c, B* t, B* r1, B* r2) { S_eval(x, c); if (S_pop(x)) { S_eval(x, t); return; } S_eval(x, r1); S_br(x, c, t, r1, r2); S_swap(x); S_br(x, c, t, r1, r2); S_eval(x, r2); }
void S_bin_rec(X* x) { B* r2 = (B*)S_pop(x); B* r1 = (B*)S_pop(x); B* t = (B*)S_pop(x); B* c = (B*)S_pop(x); S_br(x, c, t, r1, r2); }

void S_parse_literal(X* x) {
	I n = 0;
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); }
	S_push(x, n);
}

void S_parse_quotation(X* x) { 
	I t = 1; B c; 
	S_push(x, (I)(++IP(x))); 
	while (t) { switch (S_token(x)) { case '[': t++; break; case ']': t--; break; } } 
}

void S_inner(X* x) {
	I frame = FRAME(x);
	I i;
	B* j;
	do {
		/* Tracing */
		printf("<%ld> ", x->sp);
		for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } 
		for (i = x->rp - 1; i >= 0; i--) { printf(" : "); for (j = x->r[i]; *j != 0 && *j != 10&& *j != ']'; j++) { printf("%c", *j); } }
		printf(" <%ld>\n", x->rp);
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': S_parse_literal(x); break;
		case '[': S_parse_quotation(x); break;
		case 0: case ']': if (S_return(x, frame)) return; break;
		default:
			switch (S_token(x)) {
			case '+': S_add(x); break;
			case '-': S_sub(x); break;
			case '*': S_mul(x); break;
			case '/': S_div(x); break;
			case '%': S_mod(x); break;
			case '<': S_lt(x); break;
			case '=': S_eq(x); break;
			case '>': S_gt(x); break;
			case '&': S_and(x); break;
			case '|': S_or(x); break;
			case '~': S_not(x); break;
			case 's': S_swap(x); break;
			case 'd': S_dup(x); break;
			case 'r': S_rot(x); break;
			case 'o': S_over(x); break;
			case '\\': S_drop(x); break;
			case 'i': S_exec_i(x); break;
			case 'x': S_exec_x(x); break;
			case '?': S_ifthen(x); break;
			case 't': S_times(x); break;
			case 'b': S_bin_rec(x); break;
			case 'q': exit(0); break;
			default: /* TODO: C extensions */ break;
			}
		}
	} while(1);
}

#endif
