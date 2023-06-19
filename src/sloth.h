/* TODO: Add i8, i16, i32 and i64 literals */
/* TODO: Add input/output */
/* TODO: Add jumps and calls */
/* TODO: Add extensions */
/* TODO: Add error primitives */
/* TODO: Add memory operations */
/* TODO: Add dictionary (memory) */

/* TODO: Create languages on top */

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<string.h>

typedef char B;
typedef intptr_t I;

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct { I s[STACK_SIZE]; I sp; B* r[RSTACK_SIZE]; I rp; } X;

#define FRAME(x) (x->rp - 1)
#define IP(x) (x->r[x->rp - 1])
#define NEXT(x) (x->r[x->rp - 1]++)

B S_peek(X* x) { return IP(x) == 0 ? 0 : *(IP(x)); }
B S_token(X* x) { B tk = S_peek(x); NEXT(x); return tk; }
I S_is_digit(B c) { return c >= '0' && c <= '9'; }

void S_push_R(X* x, B* a) { x->r[x->rp++] = a; }
I S_return(X* x, I f) { x->rp--; return x->rp == f; }

void S_inner(X* x);

void S_eval(X* x, B* q) {
  /* TODO: Condition for tail call elimination */
	S_push_R(x, IP(x));
	IP(x) = q;
  /* TODO: If tracing? trace else inner */
	S_inner(x);
}

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])
#define DROP(x) (x->sp--)

void S_push(X* x, I v) { x->s[x->sp++] = v; }
I S_pop(X* x) { return x->s[--x->sp]; }

X* S_init() {
	X* x = malloc(sizeof(X));
	x->sp = x->rp = 0;
	return x;
}

#define OP2(x, o) NS(x) = NS(x) o TS(x); DROP(x)

void S_add(X* x) { OP2(x, +); }
void S_sub(X* x) { OP2(x, -); }
void S_mul(X* x) { OP2(x, *); }
void S_div(X* x) { OP2(x, /); }
void S_mod(X* x) { OP2(x, %); }

void S_lt(X* x) { OP2(x, <); }
void S_eq(X* x) { OP2(x, ==); }
void S_gt(X* x) { OP2(x, >); }

void S_and(X* x) { OP2(x, &); }
void S_or(X* x) { OP2(x, |); }
void S_not(X* x) { TS(x) = !TS(x); }

void S_swap(X* x) { I t = TS(x); TS(x) = NS(x); NS(x) = t; }
void S_dup(X* x) { S_push(x, TS(x)); }
void S_rot(X* x) { I t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
void S_over(X* x) { S_push(x, NS(x)); }
void S_drop(X* x) { S_pop(x); }

void S_rjump(X* x) { I j = S_pop(x); IP(x) += (j - 1); }
void S_rcall(X* x) { S_push_R(x, IP(x)); S_rjump(x); }

void S_exec_i(X* x) { B* q = (B*)S_pop(x); S_eval(x, q); }
void S_exec_x(X* x) { B* q = (B*)S_pop(x); S_push(x, (I)q); S_eval(x, q); }

void S_ifthen(X* x) { B* f = (B*)S_pop(x); B* t = (B*)S_pop(x); if (S_pop(x)) S_eval(x, t); else S_eval(x, f); }
void S_times(X* x) { B* q = (B*)S_pop(x); I t = S_pop(x); for (;t > 0; t--) { S_eval(x, q); } }

void S_br(X* x, B* c, B* t, B* r1, B* r2) { S_eval(x, c); if (S_pop(x)) { S_eval(x, t); return; } S_eval(x, r1); S_br(x, c, t, r1, r2); S_swap(x); S_br(x, c, t, r1, r2); S_eval(x, r2); }
void S_bin_rec(X* x) { B* r2 = (B*)S_pop(x); B* r1 = (B*)S_pop(x); B* t = (B*)S_pop(x); B* c = (B*)S_pop(x); S_br(x, c, t, r1, r2); }

void S_i8_lit(X* x) { S_push(x, (I)*((B*)IP(x))); NEXT(x); }
void S_i64_lit(X* x) { /* TODO */ }

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

I S_dump_X(B*, X*);

/* TODO: Make STEP(x)? One variable for tracing its easier, isn't it? */
void S_inner(X* x) {
  B buf[255];
	I frame = FRAME(x);
	I i;
	B* j;
	do {
		/* Tracing */
    memset(buf, 0, sizeof(buf));
		S_dump_X(buf, x);
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': S_parse_literal(x); break;
		case '[': S_parse_quotation(x); break;
		case 0: case ']': if (S_return(x, frame)) return; break;
		default:
			switch (S_token(x)) {
      case '#': S_i8_lit(x); break;
      case '$': S_i64_lit(x); break;
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
			case '!': S_not(x); break;
			case 's': S_swap(x); break;
			case 'd': S_dup(x); break;
			case 'r': S_rot(x); break;
			case 'o': S_over(x); break;
			case '\\': S_drop(x); break;
      case '^': S_rjump(x); break;
      case 'c': S_rcall(x); break;
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

/*
void S_inner(X* x) {
	I i, frame = FRAME(x);
	do {
    STEP(x, frame);
  } while(1);
}

void S_trace(X* x) {
  I i, frame = FRAME(x);
  do {
    STEP(x, frame);
  } while(1);
}
*/

/* Dump */
I S_dump_S(B* s, X* x) {
  I i, t, n = 0;
  for (i = 0; i < x->sp; i++) {
    s += t = sprintf(s, "%ld ", x->s[i]); n+= t;
  }
  return n;
}

I S_dump_R(B* s, X* x) {
  I i, t, n = 0;
  B* j;
  for (i = x->rp - 1; i >= 0; i--) {
    s += sprintf(s, " : "); n += 3;
    t = 0;
    j = (i == (x->rp - 1) ? x->r[i] : (x->r[i] - 1));
    for (; *j != 0 && *j != 10 && t >= 0; j++) {
      *(s++) = *j; n++;
      if (*j == '[') t++;
      if (*j == ']') t--;
    }
  }
  return n;
}

I S_dump_X(B* s, X* x) {
  I t, n = 0;
  s += t = S_dump_S(s, x); n += t;
  s += t = S_dump_R(s, x); n += t;
  return n;
}

#endif