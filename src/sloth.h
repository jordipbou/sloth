#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<string.h>

#define V void 

typedef char B;
typedef intptr_t C;
typedef double F;

typedef struct { C t; C l; union { C i; F f; V *p; } v; } O;
typedef struct _W { struct _W* l; B* c; B f; B n[1]; } W;

#define IMMEDIATE_MASK 0x80
#define HIDDEN_MASK 0x40
#define UNUSED_MASK 0x20
#define LEN_MASK 3F

enum { INT = 2, FLOAT = 3, MANAGED = 5, STRING = 7, RETURN = 11, HANDLER = 13 } T;

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _X {
  O* s; C sp; C ss;
  O* r; C rp; C rs;
	B* ip;
  B* b;
	V (*key)(struct _X*);
	V (*emit)(struct _X*);
	V (*trace)(struct _X*);
  V (**ext)(struct _X*);
  C err;
  C tr;
} X;

#define EXT(x, l) (x->ext[l - 'A'])

#define S_DICT_SIZE 65536

#define S_BLOCK_SIZE(x) (((C*)x->b)[0])
#define S_HERE(x) (((C*)x->b)[1])
#define S_LATEST(x) (((C*)x->b)[2])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(O));
  x->r = malloc(RSTACK_SIZE*sizeof(O));
	x->sp = x->rp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ext = malloc(26*sizeof(C));
  x->ip = 0;
  x->b = 0;
  x->err = 0;
  x->tr = 0;
	return x;
}

V S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define TS(x) (&x->s[x->sp - 1])
#define NS(x) (&x->s[x->sp - 2])
#define NNS(x) (&x->s[x->sp - 3])

V S_lit(X* x, C v) { O* o = &x->s[x->sp]; o->t = INT; o->l = 0; o->v.i = v; x->sp++; }
V S_str(X* x, B* s, C l) { O* o = &x->s[x->sp]; o->t = STRING; o->l = l; o->v.p = s; x->sp++; }
/* dup needs clone for other types! */
V S_dup(X* x) { S_lit(x, TS(x)->v.i); }
/* over needs clone also */
V S_over(X* x) { S_lit(x, NS(x)->v.i); }

#define ROT(a, b, c, p) { C t = a->p; a->p = b->p; b->p = c->p; c->p = t; }
V S_rot(X* x) {
  ROT(NNS(x), NS(x), TS(x), t);
  ROT(NNS(x), NS(x), TS(x), l);
  ROT(NNS(x), NS(x), TS(x), v.i);
}

#define SWAP(a, b, p) { C t = a->p; a->p = b->p; b->p = t; }
V S_swap(X* x) {
  SWAP(TS(x), NS(x), t);
  SWAP(TS(x), NS(x), l);
  SWAP(TS(x), NS(x), v.i);
}

V S_drop(X* x) {
  O o = x->s[--x->sp];
  if (o.t % MANAGED == 0) {
    free(o.v.p);
  }
}
C S_drop_C(X* x) { return x->s[--x->sp].v.i; }
F S_drop_F(X* x) { return x->s[--x->sp].v.f; }

V S_add(X* x) { NS(x)->v.i = NS(x)->v.i + TS(x)->v.i; --x->sp; }
V S_sub(X* x) { NS(x)->v.i = NS(x)->v.i - TS(x)->v.i; --x->sp; }
V S_mul(X* x) { NS(x)->v.i = NS(x)->v.i * TS(x)->v.i; --x->sp; }
V S_div(X* x) { NS(x)->v.i = NS(x)->v.i / TS(x)->v.i; --x->sp; }
V S_mod(X* x) { NS(x)->v.i = NS(x)->v.i % TS(x)->v.i; --x->sp; }

V S_and(X* x) { NS(x)->v.i = NS(x)->v.i & TS(x)->v.i; --x->sp; }
V S_or(X* x) { NS(x)->v.i = NS(x)->v.i | TS(x)->v.i; --x->sp; }
V S_xor(X* x) { NS(x)->v.i = NS(x)->v.i ^ TS(x)->v.i; --x->sp; }
V S_not(X* x) { TS(x)->v.i = !TS(x)->v.i; }
V S_invert(X* x) { TS(x)->v.i = ~TS(x)->v.i; }

V S_lt(X* x) { NS(x)->v.i = NS(x)->v.i < TS(x)->v.i; --x->sp; }
V S_eq(X* x) { NS(x)->v.i = NS(x)->v.i == TS(x)->v.i; --x->sp; }
V S_gt(X* x) { NS(x)->v.i = NS(x)->v.i > TS(x)->v.i; --x->sp; }

#define MOVE(a, b) a->t = b->t; a->l = b->l; a->v.i = b->v.i;
O* S_push(X* x) { O* a = &x->r[x->rp++]; O* b = &x->s[--x->sp]; MOVE(a, b); return b; }
V S_pop(X* x) { O* a = &x->s[x->sp++]; O* b = &x->r[--x->rp]; MOVE(a, b); }
/*V S_peek_R(X* x, C n) { O* a = &x->s[x->sp++]; O* b = &x->r[x->rp - 1 - n]; MOVE(a, b); }*/

V S_IP_to_R(X* x) { O* o = &x->r[x->rp++]; o->t = RETURN; o->l = 0; o->v.p = x->ip; }
B* S_return(X* x, C frame) {
  O o;
  while (x->rp > 0 && x->rp > frame) {
    o = x->r[--x->rp];
    if (o.t % MANAGED == 0) {
      free(o.v.p);
    } else if (o.t % RETURN == 0) {
      x->ip = o.v.p;
      return x->ip;
    }
  }
	x->ip = 0;
	return x->ip;
}
V S_jump(X* x) { x->ip = S_push(x)->v.p; }
V S_zjump(X* x) { S_swap(x); if (!S_drop_C(x)) S_jump; else S_drop(x); }
V S_call(X* x) { 
  B t = S_peek(x); 
  if (t && t != ']' && t != '}') { 
		S_IP_to_R(x); 
  }
	S_jump(x);
}
/* It should be better to directly push to R */
V S_eval(X* x, B* q) { S_lit(x, (C)q); S_call(x); S_inner(x); }

V S_bstore(X* x) { B* a = (B*)S_drop_C(x); *a = (B)S_drop_C(x); }
V S_cstore(X* x) { C* a = (C*)S_drop_C(x); *a = S_drop_C(x); }

V S_bfetch(X* x) { S_lit(x, *((B*)S_drop_C(x))); }
V S_cfetch(X* x) { S_lit(x, *((C*)S_drop_C(x))); }

V S_malloc(X* x) { S_lit(x, (C)malloc(S_drop_C(x))); }
V S_free(X* x) { free((V*)S_drop_C(x)); }

V S_inspect(X* x) {
  /* TODO: Show ASCII representation */
  /* TODO: Use vectored I/O */
  C i = 0, j;
  C n = S_drop_C(x);
  B* a = (B*)S_drop_C(x);
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

V S_branch(X* x) { 
  S_rot(x); 
  if (!S_drop_C(x)) { S_swap(x); }
  S_drop(x);
  S_call(x);
}

V S_times(X* x) { 
  B* q = (B*)S_drop_C(x); 
  C n = S_drop_C(x); 
  while (n-- > 0) { S_eval(x, q); } 
}

V S_while(X* x) { 
  B* q = (B*)S_drop_C(x);
  B* c = (B*)S_drop_C(x);
  do { 
    S_eval(x, c); 
    if (!S_drop_C(x)) break; 
    S_eval(x, q); 
  } while(1);
}
/*
V S_allot(X* x) { 
  S_HERE(x) += S_drop(x); 
}
*/

V S_create(X* x) {
	/*
  C l = S_drop_C(x);
  B* s = (B*)S_drop_C(x);
  B* w = x->b + S_HERE(x);
	*((B**)w) = (B*)S_LATEST(x);
	S_LATEST(x) = (C)w;
	S_FLAGS(w) = 0;
	S_NL(w) = l;
	strncpy(S_NFA(w), s, l);
	S_lit(x, (C)S_CFA(w));
	S_HERE(x) += sizeof(B**) + 2 + l;
	*/
}

V S_find(X* x) {
/*
  C l = S_drop_C(x);
  B* s = (B*)S_drop_C(x);
  B* w = (B*)S_LATEST(x);
  while (w) {
		if (S_NL(w) == l && !strncmp(S_NFA(w), s, l)) {
			S_lit(x, (C)w);
			return;
		}
		w = *((B**)w);
	}
  S_lit(x, (C)s);
  S_lit(x, l);
  S_lit(x, 0);
	*/
}

V S_symbol(X* x) {
/*
  C l = 0;
	B* s = x->ip;
	B* w;
  if (x->b == 0) {
    x->b = malloc(S_DICT_SIZE);
    S_BLOCK_SIZE(x) = S_DICT_SIZE;
    S_HERE(x) = 3*sizeof(C);
    S_LATEST(x) = 0;
  }
	while (!isspace(S_token(x))) { l++; }
  S_lit(x, (C)s);
  S_lit(x, l);
  S_find(x);
  if (TS(x)->v.i) {
    w = (B*)S_drop_C(x);
    S_lit(x, (C)S_CFA(w));
  } else {
    S_drop(x);
    S_create(x);
  }
	*/
}

V S_to_number(x) {
/*
  C l = S_drop_C(x);
  B* s = (B*)S_drop_C(x);
  char *ptr;
  C n = strtol(s, &ptr, 10);
  S_lit(x, n);
  S_lit(x, (C)ptr);
  S_lit(x, l - (C)(ptr - s));
*/
}

/*
V S_bcompile(X* x) { 
  B v = (B)S_drop(x);
  *(x->b + S_HERE(x)) = v;
  S_HERE(x)++;
}

V S_ccompile(X* x) {
  C v = S_drop(x);
  *((C*)(x->b + S_HERE(x))) = v;
  S_HERE(x) += sizeof(C);
}
*/
V S_qcompile(X* x, C e) { 
  C l = 0, t = 1; 
  B* q = (B*)S_drop_C(x);
  while (t) {
    if (q[l] == '[') t++;
    if (q[l] == ']') t--;
    l++;
  }
  strncpy(x->b + S_HERE(x), q, l + e);
  S_HERE(x) += l + e; 
}

V S_accept(X* x) { 
	C i = 0;
  C l1 = S_drop_C(x);
  B* s1 = (B*)S_drop_C(x);
	do { 
		x->key(x); 
		if (TS(x)->v.i == 10) {
			S_drop(x);
			break;
		} else if (TS(x)->v.i == 127) {
      S_drop(x);
      if (i > 0) {
        S_lit(x, '\b');
        x->emit(x);
        S_lit(x, ' ');
        x->emit(x);
        S_lit(x, '\b');
        x->emit(x);
        i--;
      }
    } else {
			s1[i++] = TS(x)->v.i;
			x->emit(x);
		}
	} while(i < l1);
	S_lit(x, i);
}

V S_type(X* x) {
	C i = 0;
	C n = S_drop_C(x);
	B* s = (B*)S_drop_C(x);
	while (n-- > 0) { S_lit(x, s[i++]); x->emit(x); }
}

V S_parse_literal(X* x) { 
	C n = 0; 
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); } 
	S_lit(x, n); 
}

V S_parse_quotation(X* x) { 
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

V S_parse_string(X* x) {
	B* s = ++x->ip;
  while (S_token(x) != '"') {}
	S_str(x, s, (C)(x->ip - s - 1));
}

V S_inner(X* x) {
	C frame = x->rp;
	do {
#ifndef S_NO_TRACING
		if (x->tr) x->trace(x);
#endif
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
		case '[': 
      S_parse_quotation(x); break;
    case '"':
      S_parse_string(x); break;
		case 0: case ']': case '}':
      if (!S_return(x, frame)) return;
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
      case ')': S_pop(x); break;
      case '(': S_push(x); break;
			case 'x': S_call(x); break;
			case 'j': S_jump(x); break;
      case 'z': S_zjump(x); break;
      case 'y': exit(0); break;
      /* Memory */
      case ':': S_bfetch(x); break;
      case ';': S_bstore(x); break;
			case '.': S_cfetch(x); break;
			case ',': S_cstore(x); break;
      case 'b': S_lit(x, (C)(&x->b)); break;
      case 'c': S_lit(x, sizeof(C)); break;
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      /* Helpers */
      case '?': S_branch(x); break;
      case 't': S_times(x); break;
      case 'w': S_while(x); break;
			case 'u': x->tr = 0; break;
			case 'v': x->tr = 1; break;
			/* Dictionary */
      case 'i': S_inspect(x); break;
      case '\\': S_symbol(x); break;
      case '$': S_symbol(x); S_call(x); break;
      case 'g': S_qcompile(x, -1); break;
      case 'q': S_qcompile(x, 0); break;
      case 'h': S_create(x); break;
      case '`': S_find(x); break;
      case 'a': S_accept(x); break;
      case 'p': S_type(x); break;
      case 'n': S_to_number(x); break;
      /* Input/output */
			case 'k': x->key(x); break;
			case 'e': x->emit(x); break;
      }
    }
	} while(1);
}

#endif
