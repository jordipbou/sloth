#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<string.h>

typedef char C;
typedef int16_t S;
typedef int32_t L;
typedef intptr_t I;

#define STACK_SIZE 64
#define RSTACK_SIZE 64
#define DICT_SIZE 4096

typedef struct _X { 
  I* s; I sp; I ss;
  C** r; I rp; I rs;
	C* ip;
  C* d; I dp; I ds;
  void (*key)(struct _X*);
  void (*emit)(struct _X*);
  void (*ext[26])(struct _X*);
  I err;
  I tr;
} X;

#define EXT(x, l) (x->ext[l - 'A'])

I dump_S(C* s, X* x) {
  I i = 0;
  I t, n = 0;
  while (i < x->sp) {
    s += t = sprintf(s, "%d ", x->s[i++]); 
    n += t;
  }
  return n;
}

I dump_R(C* s, X* x) {
  /* dump ip */
  /* dump r */
  return 0;
}

I dump_X(C* s, X* x) {
  I t, n = 0;
  s += t = dump_S(s, x); n += t;
  *s++ = ':'; *s++ = ' '; n += 2;
  s += t = dump_R(s, x); n += t;
  return n;
}

void S_inner(X* x);

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(I));
  x->r = malloc(RSTACK_SIZE*sizeof(I));
  x->d = malloc(DICT_SIZE);
	x->sp = x->rp = x->dp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ds = DICT_SIZE;
  x->err = 0;
  x->tr = 0;
	return x;
}

C S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
C S_token(X* x) { C tk = S_peek(x); x->ip++; return tk; }
I S_is_digit(C c) { return c >= '0' && c <= '9'; }

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])

void S_lit(X* x, I v) { x->s[x->sp++] = v; }
void S_dup(X* x) { S_lit(x, TS(x)); }
void S_over(X* x) { S_lit(x, NS(x)); }
void S_rot(X* x) { I t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
void S_swap(X* x) { I t = TS(x); TS(x) = NS(x); NS(x) = t; }
I S_drop(X* x) { return x->s[--x->sp]; }

void S_add(X* x) { NS(x) = NS(x) + TS(x); --x->sp; }
void S_sub(X* x) { NS(x) = NS(x) - TS(x); --x->sp; }
void S_mul(X* x) { NS(x) = NS(x) * TS(x); --x->sp; }
void S_div(X* x) { NS(x) = NS(x) / TS(x); --x->sp; }
void S_mod(X* x) { NS(x) = NS(x) % TS(x); --x->sp; }

void S_and(X* x) { NS(x) = NS(x) & TS(x); --x->sp; }
void S_or(X* x) { NS(x) = NS(x) | TS(x); --x->sp; }
void S_not(X* x) { TS(x) = !TS(x); }

void S_lt(X* x) { NS(x) = NS(x) < TS(x); --x->sp; }
void S_eq(X* x) { NS(x) = NS(x) == TS(x); --x->sp; }
void S_gt(X* x) { NS(x) = NS(x) > TS(x); --x->sp; }

void S_push(X* x) { x->r[x->rp++] = x->ip; }
void S_pop(X* x) { x->ip = x->r[--x->rp]; }
void S_exec(X* x) { if (S_peek(x) != ']') S_push(x); x->ip = (C*)S_drop(x); }
void S_to(X* x) { x->r[x->rp++] = (C*)x->s[--x->sp]; }
void S_from(X* x) { x->s[x->sp++] = (I)x->r[--x->rp]; }
void S_copy(X* x) { x->s[x->sp++] = (I)x->r[x->rp - 1]; }
void S_if(X* x) { S_rot(x); if (!S_drop(x)) { S_swap(x); } S_drop(x); S_exec(x); }
void S_times(X* x) { C* q = (C*)S_drop(x); I n = S_drop(x); for (;n > 0; n--) { S_lit(x, (I)q); S_exec(x); } }

void S_alloc(X* x) { x->dp += S_drop(x); }
void S_here(X* x) { S_lit(x, (I)&x->d[x->dp]); }

void S_store_i8(X* x) { C* a = (C*)S_drop(x); *a = (C)S_drop(x); }
void S_store_i16(X* x) { S* a = (S*)S_drop(x); *a = (S)S_drop(x); }
void S_store_i32(X* x) { L* a = (L*)S_drop(x); *a = (L)S_drop(x); }
void S_store_i64(X* x) { I* a = (I*)S_drop(x); *a = S_drop(x); }

void S_fetch_i8(X* x) { S_lit(x, *((C*)S_drop(x))); }
void S_fetch_i16(X* x) { S_lit(x, *((S*)S_drop(x))); }
void S_fetch_i32(X* x) { S_lit(x, *((L*)S_drop(x))); }
void S_fetch_i64(X* x) { S_lit(x, *((I*)S_drop(x))); }

void S_parse_literal(X* x) { 
	I n = 0; 
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); } 
	S_lit(x, n); 
}

void S_parse_quotation(X* x) { 
	I t = 1; 
	C c; 
	S_lit(x, (I)(++x->ip)); 
	while (t) { switch (S_token(x)) { case '[': t++; break; case ']': t--; break; } } 
}

void S_inner(X* x) {
	I frame = x->rp;
	do {
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
		case '[': 
      S_parse_quotation(x); break;
		case 0: case ']': 
      if (x->rp > frame && x->rp > 0) S_pop(x);
			else return;
      break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, S_token(x))(x);
      break;
		default:
			switch (S_token(x)) {
			case 'd': S_dup(x); break;
			case 'o': S_over(x); break;
			case 'r': S_rot(x); break;
			case '$': S_swap(x); break;
			case '\\': S_drop(x); break;
			case '+': S_add(x); break;
			case '-': S_sub(x); break;
			case '*': S_mul(x); break;
			case '/': S_div(x); break;
			case '%': S_mod(x); break;
			case '&': S_and(x); break;
			case '|': S_or(x); break;
			case '_': S_not(x); break;
			case '<': S_lt(x); break;
			case '=': S_eq(x); break;
			case '>': S_gt(x); break;
			case 'i': S_exec(x); break;
			case 'a': S_alloc(x); break;
			case 'h': S_here(x); break;
			case '@': S_fetch_i64(x); break;
			case '!': S_store_i64(x); break;
			case ',': S_here(x); S_store_i64(x); S_lit(x, sizeof(I)); S_alloc(x); break;
			case 't': S_to(x); break;
			case 'f': S_from(x); break;
			case 'p': S_copy(x); break;
			case '?': S_if(x); break;
			case 'n': S_times(x); break;
			case 'k': x->key(x); break;
			case 'e': x->emit(x); break;
			case 'q': exit(0); break;
			case 'c':
				switch(S_token(x)) {
				case '@': S_fetch_i8(x); break;
				case '!': S_store_i8(x); break;
				case ',': S_here(x); S_store_i8(x); S_lit(x, 1); S_alloc(x); break;
				}
				break;
			case 's':
				switch(S_token(x)) {
				case '@': S_fetch_i16(x); break;
				case '!': S_store_i16(x); break;
				case ',': S_here(x); S_store_i16(x); S_lit(x, sizeof(S)); S_alloc(x); break;
				}
				break;
			case 'l':
				switch(S_token(x)) {
				case '@': S_fetch_i32(x); break;
				case '!': S_store_i32(x); break;
				case ',': S_here(x); S_store_i32(x); S_lit(x, sizeof(L)); S_alloc(x); break;
				}
				break;
			}
		}
	} while(1);
}

#endif
