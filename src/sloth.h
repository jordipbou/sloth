#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy, memset, strncmp */

typedef char B;
typedef intptr_t C;

#define STACK_SIZE 64
#define RSTACK_SIZE 64
#define TSTACK_SIZE 64

typedef struct _W { struct _W* l; B* c; B f; B s; B n[1]; } W;

typedef struct _X { 
  C* s; C sp; C ss;
  B** r; C rp; C rs;
  C* t; C tp; C ts;
	B* ip;
  B* b;
	void (*key)(struct _X*);
	void (*emit)(struct _X*);
  void (**ext)(struct _X*);
  C err;
  C tr;
} X;

#include "trace.h"

#define EXT(x, l) (x->ext[l - 'A'])

#define S_DEFAULT_DICT_SIZE 8192

#define S_BLOCK_SIZE(x) (((C*)x->b)[0])
#define S_HERE(x) (((C*)x->b)[1])
#define S_LATEST(x) (((C*)x->b)[2])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(C));
  x->r = malloc(RSTACK_SIZE*sizeof(C));
  x->t = malloc(TSTACK_SIZE*sizeof(C));
	x->sp = x->rp = x->tp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
	x->ts = TSTACK_SIZE;
  x->ext = malloc(26*sizeof(C));
  x->ip = 0;
  x->b = 0;
  x->err = 0;
  x->tr = 0;
	return x;
}

void S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])

void S_lit(X* x, C v) { x->s[x->sp++] = v; }
void S_dup(X* x) { S_lit(x, TS(x)); }
void S_over(X* x) { S_lit(x, NS(x)); }
void S_rot(X* x) { C t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
void S_swap(X* x) { C t = TS(x); TS(x) = NS(x); NS(x) = t; }
C S_drop(X* x) { return x->s[--x->sp]; }

void S_add(X* x) { NS(x) = NS(x) + TS(x); --x->sp; }
void S_sub(X* x) { NS(x) = NS(x) - TS(x); --x->sp; }
void S_mul(X* x) { NS(x) = NS(x) * TS(x); --x->sp; }
void S_div(X* x) { NS(x) = NS(x) / TS(x); --x->sp; }
void S_mod(X* x) { NS(x) = NS(x) % TS(x); --x->sp; }

void S_and(X* x) { NS(x) = NS(x) & TS(x); --x->sp; }
void S_or(X* x) { NS(x) = NS(x) | TS(x); --x->sp; }
void S_xor(X* x) { NS(x) = NS(x) ^ TS(x); --x->sp; }
void S_not(X* x) { TS(x) = !TS(x); }
void S_invert(X* x) { TS(x) = ~TS(x); }

void S_lt(X* x) { NS(x) = NS(x) < TS(x); --x->sp; }
void S_eq(X* x) { NS(x) = NS(x) == TS(x); --x->sp; }
void S_gt(X* x) { NS(x) = NS(x) > TS(x); --x->sp; }

void S_to_T(X* x) { x->t[x->tp++] = x->s[--x->sp]; }
void S_from_T(X* x) { x->s[x->sp++] = x->t[--x->tp]; }
void S_peek_T(X* x, C n) { x->s[x->sp++] = x->t[x->tp - 1 - n]; }

void S_push(X* x) { x->r[x->rp++] = x->ip; }
void S_pop(X* x) { x->ip = x->r[--x->rp]; }
void S_call(X* x) { B t = S_peek(x); if (t && t != ']' && t != '}') S_push(x); x->ip = (B*)S_drop(x); }
void S_zcall(X* x) { S_swap(x); if (S_drop(x)) S_drop(x); else S_call(x); }
void S_eval(X* x, B* q) { S_lit(x, (C)q); S_call(x); S_inner(x); }

void S_bstore(X* x) { B* a = (B*)S_drop(x); *a = (B)S_drop(x); }
void S_cstore(X* x) { C* a = (C*)S_drop(x); *a = S_drop(x); }

void S_bfetch(X* x) { S_lit(x, *((B*)S_drop(x))); }
void S_cfetch(X* x) { S_lit(x, *((C*)S_drop(x))); }

void S_malloc(X* x) { S_lit(x, (C)malloc(S_drop(x))); }
void S_free(X* x) { free((void*)S_drop(x)); }

/* TODO: Could this be done "easily" in SLOTH */
void S_inspect(X* x) {
  /* TODO: Show ASCII representation */
  /* TODO: Use vectored I/O */
  C i = 0, j;
  C n = S_drop(x);
  B* a = (B*)S_drop(x);
  while (i < n) {
    /* Do with type! */
    printf("\n%p: ", a + i);
    for (j = 0; j < 4 && i < n; j++, i++) {
      printf("%02X ", (unsigned char)a[i]);
    }
    if (i < n) {
      printf("- ");
      for (j = 0; j < 4 && i < n; j++, i++) {
        printf("%02X ", (unsigned char)a[i]);
      }
    }
  }
  printf("\n");
}

void S_branch(X* x) { 
  S_rot(x); 
  if (!S_drop(x)) { S_swap(x); }
  S_drop(x);
  S_call(x);
}

void S_create(X* x) {
  C l = S_drop(x);
  B* s = (B*)S_drop(x);
	W* w = (W*)(x->b + S_HERE(x));
	w->l = (W*)S_LATEST(x);
	S_LATEST(x) = (C)w;
	w->c = ((B*)w) + 2*sizeof(C) + 2 + l;
	w->f = 0;
	w->s = l;
	strncpy(w->n, s, l);
	S_lit(x, (C)w->c);
	S_HERE(x) += 2*sizeof(C) + 2 + l;
}

void S_find(X* x) {
  C l = S_drop(x);
  B* s = (B*)S_drop(x);
	W* w = (W*)S_LATEST(x);
	while (w) {
		if (w->s == l && !strncmp(w->n, s, l)) {
			S_lit(x, (C)w);
			return;
		}
		w = w->l;
	}
	S_lit(x, (C)s);
	S_lit(x, l);
	S_lit(x, 0);
}

void S_parse_symbol(X* x) {
  C l = 0;
  B* s = x->ip;
  while (!isspace(S_token(x))) { l++; }
  S_lit(x, (C)s);
  S_lit(x, l);
}

void S_cfa(X* x) {
  W* w = (W*)S_drop(x);
  S_lit(x, (C)w->c);
}

void S_symbol(X* x) {
	W* w;
  if (x->b == 0) {
    x->b = malloc(S_DEFAULT_DICT_SIZE);
    S_BLOCK_SIZE(x) = S_DEFAULT_DICT_SIZE;
    S_HERE(x) = 3*sizeof(C);
    S_LATEST(x) = 0;
  }
  S_parse_symbol(x);
  S_find(x);
  if (TS(x)) {
    w = (W*)S_drop(x);
    S_lit(x, (C)w->c);
  } else {
    S_drop(x);
    S_create(x);
  }
}

void S_to_number(X* x) {
  C l = S_drop(x);
  B* s = (B*)S_drop(x);
  char *ptr;
  C n = strtol(s, &ptr, 10);
  S_lit(x, n);
  S_lit(x, (C)ptr);
  S_lit(x, l - (C)(ptr - s));
}

void S_qcompile(X* x, C e) { 
  C l = 0, t = 1; 
  B* q = (B*)S_drop(x);
  while (t) {
    if (q[l] == '[') t++;
    if (q[l] == ']') t--;
    l++;
  }
  strncpy(x->b + S_HERE(x), q, l + e);
  S_HERE(x) += l + e; 
}

void S_parse_literal(X* x) { 
	C n = 0; 
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); } 
	S_lit(x, n); 
}

void S_parse_quotation(X* x) { 
	C t = 1; 
	B c; 
	S_lit(x, (C)(++x->ip)); 
	while (t) { 
    switch (S_token(x)) { 
    case '[': t++; break; 
    case ']': t--; break;
    } 
  }
}

void S_parse_string(X* x) {
  S_lit(x, (C)(++x->ip));
  while (S_token(x) != '"') {}
  S_lit(x, (C)(x->ip - TS(x)) - 1);
}

void S_inner(X* x) {
	B buf[1024];
	C frame = x->rp;
	do {
    S_trace(x);
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
		case '[': 
      S_parse_quotation(x); break;
    case '"':
      S_parse_string(x); break;
		case 0: case ']': case '}':
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
      case '\'': S_lit(x, (C)S_token(x)); break;
      case '#': S_lit(x, *((C*)x->ip)); x->ip += sizeof(C); break;
      case '@': S_lit(x, (C)(x->ip + ((B)S_token(x)))); break;
      /* Stack */
			case 's': S_swap(x); break;
			case 'd': S_dup(x); break;
			case 'o': S_over(x); break;
			case 'r': S_rot(x); break;
			case '_': S_drop(x); break;
      /* Arithmetics */
			case '+': S_add(x); break;
			case '-': S_sub(x); break;
			case '*': S_mul(x); break;
			case '/': S_div(x); break;
			case '%': S_mod(x); break;
      /* Bitwise */
			case '&': S_and(x); break;
			case '|': S_or(x); break;
			case '^': S_xor(x); break;
			case '!': S_not(x); break;
			case '~': S_invert(x); break;
      /* Comparison */
			case '<': S_lt(x); break;
			case '=': S_eq(x); break;
			case '>': S_gt(x); break;
      /* Execution */
      case ')': S_from_T(x); break;
      case '(': S_to_T(x); break;
      case 'u': S_peek_T(x, 0); break;
      case 'v': S_peek_T(x, 1); break;
      case 'w': S_peek_T(x, 2); break;
      case 'x': S_peek_T(x, 3); break;
      case 'y': S_peek_T(x, 4); break;
      case 'a': S_call(x); break;
      case 'z': S_zcall(x); break;
      /*case 'q': exit(0); break;*/
      /* Memory */
      case 'm': S_malloc(x); break;
      case 'f': S_free(x); break;
      case 'c': S_lit(x, sizeof(C)); break;
      case ':': S_bfetch(x); break;
      case ';': S_bstore(x); break;
			case '.': S_cfetch(x); break;
			case ',': S_cstore(x); break;
      case 'b': S_lit(x, (C)(&x->b)); break;
      /* Input/output */
			case 'k': x->key(x); break;
			case 'e': x->emit(x); break;
      /* Helpers */
      case '?': S_branch(x); break;
      case 'i': S_inspect(x); break;
      case '\\': S_symbol(x); break;
      case '$': S_parse_symbol(x); S_find(x); S_cfa(x); S_call(x); break;
      case 'g': S_qcompile(x, -1); break;
      case 'q': S_qcompile(x, 0); break;
      case 'h': S_create(x); break;
      case '`': S_find(x); break;
      case 'n': S_to_number(x); break;
      }
    }
	} while(1);
}

#endif
