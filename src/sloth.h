/* TODO: Errors */
/* IDEAS: Implement registers A, B, X, Y (in return stack as ReForth?) */
/* IDEAS: Implement recognizers? */

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */

#define V void /* inline void ? */

typedef char B;
typedef intptr_t C;

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _X {
  C* s; C sp; C ss;
  B** r; C rp; C rs;
	B* ip;
  B* b;
	void (*key)(struct _X*);
	void (*emit)(struct _X*);
  void (*trace)(struct _X*);
  void (**ext)(struct _X*);
  C err;
  C tr;
} X;

#define EXT(x, l) (x->ext[l - 'A'])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(C));
  x->r = malloc(RSTACK_SIZE*sizeof(C));
	x->sp = x->rp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ext = malloc(26*sizeof(C));
  x->ip = x->b = 0;
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

V S_lit(X* x, C v) { x->s[x->sp++] = v; }

/* Parsing */

V S_parse_literal(X* x) { 
	C n = 0; 
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); } 
	S_lit(x, n); 
}

V S_parse_quotation(X* x) { 
	C t = 1; 
	S_lit(x, (C)(x->ip)); 
	while (t) { 
    switch (S_token(x)) { 
    case '[': t++; break; 
    case ']': t--; break;
    } 
  }
}

/* Stack instructions */

V S_dup(X* x) { S_lit(x, TS(x)); }
V S_over(X* x) { S_lit(x, NS(x)); }
V S_rot(X* x) { C t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
V S_swap(X* x) { C t = TS(x); TS(x) = NS(x); NS(x) = t; }
C S_drop(X* x) { return x->s[--x->sp]; }

V S_add(X* x) { NS(x) = NS(x) + TS(x); --x->sp; }
V S_sub(X* x) { NS(x) = NS(x) - TS(x); --x->sp; }
V S_mul(X* x) { NS(x) = NS(x) * TS(x); --x->sp; }
V S_div(X* x) { NS(x) = NS(x) / TS(x); --x->sp; }
V S_mod(X* x) { NS(x) = NS(x) % TS(x); --x->sp; }

V S_and(X* x) { NS(x) = NS(x) & TS(x); --x->sp; }
V S_or(X* x) { NS(x) = NS(x) | TS(x); --x->sp; }
V S_xor(X* x) { NS(x) = NS(x) ^ TS(x); --x->sp; }
V S_not(X* x) { TS(x) = !TS(x); }
V S_invert(X* x) { TS(x) = ~TS(x); }

V S_lt(X* x) { NS(x) = NS(x) < TS(x); --x->sp; }
V S_eq(X* x) { NS(x) = NS(x) == TS(x); --x->sp; }
V S_gt(X* x) { NS(x) = NS(x) > TS(x); --x->sp; }

V S_push(X* x) { x->r[x->rp++] = (B*)x->s[--x->sp]; }
V S_pop(X* x) { x->s[x->sp++] = (C)x->r[--x->rp]; }
V S_jump(X* x) { x->ip = (B*)S_drop(x); }
V S_zjump(X* x) { S_swap(x); if (!S_drop(x)) S_jump(x); else S_drop(x); }

#define TC(x, t) (x->ip && t && t != ']' && t != '}')
V S_call(X* x) { B t = S_peek(x); if TC(x, t) x->r[x->rp++] = x->ip; S_jump(x); }

B* S_return(X* x, C f) {
  if (x->rp > f && x->rp > 0) {
    x->ip = x->r[--x->rp];
  }	else {
    x->ip = 0;
  }
  return x->ip;
}

V S_eval(X* x, B* q) { S_lit(x, (C)q); S_call(x); S_inner(x); }

V S_bstore(X* x) { B* a = (B*)S_drop(x); *a = (B)S_drop(x); }
V S_cstore(X* x) { C* a = (C*)S_drop(x); *a = S_drop(x); }
V S_bfetch(X* x) { S_lit(x, *((B*)S_drop(x))); }
V S_cfetch(X* x) { S_lit(x, *((C*)S_drop(x))); }

V S_malloc(X* x) { S_lit(x, (C)malloc(S_drop(x))); }
V S_free(X* x) { free((void*)S_drop(x)); }

V S_branch(X* x) { 
  S_rot(x); 
  if (!S_drop(x)) { S_swap(x); }
  S_drop(x);
  S_call(x);
}

V S_times(X* x) {
  B* q = (B*)S_drop(x);
  C n = S_drop(x);
  for (; n > 0; n--) S_eval(x, q);
}

V S_while(X* x) {
  B* q = (B*)S_drop(x);
  B* c = (B*)S_drop(x); 
  do {
    S_eval(x, c);
    if (S_drop(x)) S_eval(x, q);
    else break;
  } while (1);
}

void S_inner(X* x) {
  C frame = x->rp;
  do {
#ifndef S_NO_TRACING
    if (x->tr) x->trace(x);
#endif
    switch (S_peek(x)) {
    case 0: if (!S_return(x, frame)) { return; } break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, S_token(x))(x);
      break;
    default:
      switch (S_token(x)) {
      case '[': S_parse_quotation(x); break;
      case '\'': S_lit(x, (C)S_token(x)); break;
      case '#': S_lit(x, *((C*)x->ip)); x->ip += sizeof(C); break;
      case '@': S_lit(x, (C)(x->ip + ((B)S_token(x)))); break;
      /* Stacks */
      case '_': S_drop(x); break;
      case 's': S_swap(x); break;
      case 'd': S_dup(x); break;
      case 'o': S_over(x); break;
      case 'r': S_rot(x); break;
      case '(': S_push(x); break;
      case ')': S_pop(x); break;
      /* Arithmetics */
      case '+': S_add(x); break;
      case '-': S_sub(x); break;
      case '*': S_mul(x); break;
      case '/': S_div(x); break;
      case '%': S_mod(x); break;
      /* Bitwise */
      case '!': S_not(x); break;
      case '~': S_invert(x); break;
      case '&': S_and(x); break;
      case '|': S_or(x); break;
      case '^': S_xor(x); break;
      /* Comparison */
      case '<': S_lt(x); break;
      case '=': S_eq(x); break;
      case '>': S_gt(x); break;
      /* Execution */
      case ']': case '}': if (!S_return(x, frame)) { return; } break;
      case 'x': S_call(x); break;
      case 'j': S_jump(x); break;
      case 'z': S_zjump(x); break;
      case 'y': exit(0); break;
      /* Helpers */
      case '?': S_branch(x); break;
      case 'n': S_times(x); break;
      case 'w': S_while(x); break;
      case 'u': x->tr = 0; break;
      case 'v': x->tr = 1; break;
      /* Memory */
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      case '.': S_cfetch(x); break;
      case ':': S_bfetch(x); break;
      case ',': S_cstore(x); break;
      case ';': S_bstore(x); break;
      case 'b': S_lit(x, (C)&x->b); break;
      case 'c': S_lit(x, sizeof(C)); break;
      /* Input/output */
      case 'k': x->key(x); break;
      case 'e': x->emit(x); break;
      }
    }
  } while(1);
}

/* FORTH */

typedef struct _W { struct _W* p; B* i; B* c; C h; C l; B n[1]; } W;

#define SF_DICT_SIZE 65536
#define SF_TIB_SIZE 1024

typedef struct { C sz; C h; W* l; C st; C src_id; B* src; C src_sz; C in; B tib[SF_TIB_SIZE]; } D;

W* SF_header(X* x, B* n) {
  D* d = (D*)x->b;
  B l = strlen(n);
  /* SF_align(x); */
  W* w = (W*)(x->b + d->h);
  d->h += sizeof(W) + l - sizeof(C) + 1;
  w->p = d->l;
  d->l = w;
  w->h = 1;
  w->i = x->b + d->h;
  w->c = 0;
  w->l = l;
  strncpy(w->n, n, l);
  w->n[l] = 0;
  return w;
}

C SF_parse_name(X* x) {
  D* d = (D*)x->b;
  while (d->in < d->src_sz && d->src[d->in] != 0 && isspace(d->src[d->in])) {
    d->in++;
  } 
  S_lit(x, (C)(d->src + d->in));
  while (d->in < d->src_sz && d->src[d->in] != 0 && !isspace(d->src[d->in])) {
    d->in++;
  }
  S_lit(x, ((C)(d->src + d->in)) - TS(x));
  return TS(x);
}

V SF_find_name(X* x) {
  B l = (B)S_drop(x);
  B* n = (B*)S_drop(x);
  W* w = ((D*)x->b)->l;
  while (w != 0) {
    if (w->l == l && !strncmp(w->n, n, l)) break;
    w = w->p;
  }
  S_lit(x, (C)w);
}

V SF_compile(X* x) {
  /* Just copy the bytecode into the new word? */
}

V SF_to_number(X* x) {
  B l = (B)S_drop(x);    
  B* s = (B*)S_drop(x);
  char* endptr;
  C n = strtol(s, &endptr, 0);
  S_lit(x, n);
  S_lit(x, s);
  S_lit(x, endptr - s);
}

V SF_interpret(X* x) {
  while (SF_parse_name(x)) {
    x->trace(x);
    S_over(x);
    S_over(x);
    SF_find_name(x);
    if (TS(x)) {
      printf("WORD FOUND!\n");
      W* w = S_drop(x);
      S_drop(x);
      S_drop(x);
      if (((D*)x->b)->st) {
        if (w->c) {
          S_eval(x, w->c);
        } else {
          printf("NON DUAL WORD\n");
          S_lit(x, (C)w->i);
          printf("COMPILING\n");
          SF_compile(x);
        }
      } else {
        S_eval(x, w->i);
      }
    } else {
      printf("WORD NOT FOUND!\n");
      S_drop(x);
      SF_to_number(x);
      x->trace(x);
      if (TS(x)) {
        S_drop(x);
        S_drop(x);
      } else {
        /* Error */
      }
    }
  }
  S_drop(x);
  S_drop(x);
}

V SF_refill(X* x) {
  D* d = (D*)x->b;
  fgets(d->src, d->src_sz, stdin);
  d->in = 0;
}

V SF_outer(X* x) {
  W* w = ((D*)x->b)->l;
  C i;
  printf("WORDS:\n");
  while (w) {
    printf(": %.*s ;code: ", w->l, w->n);
    for (i = 0; w->i[i] != ']'; i++) {
      printf("%c", w->i[i]);
    }
    printf("]\n");
    w = w->p;
  }
  while (1) {
    SF_refill(x);
    SF_interpret(x);   
    /* Check errors and print OK and all that */
    x->trace(x);
  }
}

V SF_add_primitive(X* x, B* n, B* c) {
  D* d = (D*)x->b;
  SF_header(x, n);
  strcpy(x->b + d->h, c);
  d->h += strlen(c);
  *(x->b + d->h) = ']';
  d->h++;
}

X* SF_init(X* x) {
  /* TODO: Forth C implemented words should be added as an extension to Sloth! */
  x->b = malloc(SF_DICT_SIZE);
  D* d = (D*)x->b;
  d->sz = SF_DICT_SIZE;
  d->h = sizeof(D);
  d->l = 0;
  d->st = 0;
  d->src_id = 0;
  d->src = d->tib;
  d->src_sz = SF_TIB_SIZE;
  d->in = 0;
  memset(d->tib, 0, SF_TIB_SIZE);

  SF_add_primitive(x, "swap", "s");
  SF_add_primitive(x, "drop", "_");
  SF_add_primitive(x, "dup", "d");
  SF_add_primitive(x, "over", "o");
  SF_add_primitive(x, "rot", "r");

  SF_add_primitive(x, "+", "+");
  SF_add_primitive(x, "-", "-");
  SF_add_primitive(x, "*", "*");
  SF_add_primitive(x, "/", "/");
  SF_add_primitive(x, "mod", "%");

  SF_add_primitive(x, "and", "&");
  SF_add_primitive(x, "or", "|");
  SF_add_primitive(x, "xor", "^");
  SF_add_primitive(x, "not", "!");
  SF_add_primitive(x, "invert", "~");

  SF_add_primitive(x, "=", "=");
  SF_add_primitive(x, "<", "<");
  SF_add_primitive(x, ">", ">");

  SF_add_primitive(x, "execute", "x");
  SF_add_primitive(x, "jump", "j");
  SF_add_primitive(x, "zjump", "z");

  return x;
}

#endif