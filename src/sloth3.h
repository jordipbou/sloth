#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */

#define V void /* inline void ? */

typedef char B;
typedef intptr_t C;

/* Contexts */

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Symbol {
  struct _Symbol* previous;
	C flags;
	B* interpretation;
	B* compilation;
  C nlen;
	B name[sizeof(C)];
} S;

typedef struct _Context {
  C* s; C sp; C ss;
  B** r; C rp; C rs;
	B* ip;
	B* ibuf;
  C err;
	S* latest;
	C state; /* could be merged with err */
  void (**ext)(struct _Context*, void* st);
  void* st[26];
} X;

#define EXT(x, l) (x->ext[l - 'A'])
#define ST(x, l) (x->st[l - 'A'])

#define LOCAL(x, t1, v1) t1 v1 = (t1)S_drop(x)

#define LOCALS2(x, t1, v1, t2, v2) \
  t1 v1 = (t1)S_drop(x); \
  t2 v2 = (t2)S_drop(x)

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(C));
  x->r = malloc(RSTACK_SIZE*sizeof(C));
	x->sp = x->rp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ext = malloc(26*sizeof(C));
  x->ip = 0;
  x->err = 0;
	return x;
}

#include "trace.h"

void S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); if (x->ip) x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])

#define S_lit(x, v) (((X*)(x))->s[((X*)(x))->sp++] = (C)(v))

/* Parsing */

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

/* Can be done with a step function as a macro, as it was done? */
/* That way, tracing does not belong to here */
void S_inner(X* x) {
  B l;
  C frame = x->rp;
  do {
    /*EXT(x, 'T')(x, 0);*/
    S_trace(x);
    switch (S_peek(x)) {
    case 0: if (!S_return(x, frame)) { return; } break;
		/* Let's try this directly, if space, just return */
		case ' ': return; break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      l = S_token(x);
      EXT(x, l)(x, ST(x, l));
      break;
    default:
      switch (S_token(x)) {
      case '[': S_parse_quotation(x); break;
      /* Literals */
      case '0': S_lit(x, 0); break;
      case '1': S_lit(x, 1); break;
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
      case 'i': S_call(x); break;
      case 'j': S_jump(x); break;
      case 'z': S_zjump(x); break;
      case 'q': /* Set error */ exit(0); break;
      /* Helpers */
      case '?': S_branch(x); break;
      case 't': S_times(x); break;
      case 'w': S_while(x); break;
      /* Memory */
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      case '.': S_cfetch(x); break;
      case ':': S_bfetch(x); break;
      case ',': S_cstore(x); break;
      case ';': S_bstore(x); break;
      case 'c': S_lit(x, sizeof(C)); break;
      }
    }
  } while(1);
}

V S_parse_space(X* x) { while (x->ibuf && *x->ibuf && isspace(*x->ibuf)) { x->ibuf++; } }
V S_parse_non_space(X* x) { while (x->ibuf && * x->ibuf && !isspace(*x->ibuf)) { x->ibuf++; } }
V S_parse_name(X* x) { 
	S_parse_space(x); 
	S_lit(x, x->ibuf); 
	S_parse_non_space(x); 
	S_lit(x, (x->ibuf - ((B*)TS(x)))); 
}
V S_find_name(X* x) {
	C n = TS(x);
  B* s = (B*)NS(x);
	S* w = x->latest;
	while (w) {
	  if (w->nlen == n && !strncmp(w->name, s, n)) break;
		w = w->previous;
	}
	S_lit(x, w);
}

V S_evaluate(X* x, B* s) {
	B* p;
	S* w;
	C n;
	x->ibuf = s;
  while (x->ibuf && *x->ibuf) {
		S_parse_space(x);
		if (x->ibuf && *x->ibuf) {
			if (*x->ibuf == '\\') {
				x->ibuf++;
				S_eval(x, x->ibuf);
				S_parse_non_space(x);
			} else {
				S_parse_name(x);
				S_find_name(x);
				w = (S*)S_drop(x);
				n = S_drop(x);
				p = (B*)S_drop(x);
				if (w) {
					if (!x->state) {
						if (w->interpretation) {
							S_eval(x, w->interpretation);
						} else {
							/* Word can not be interpreted */
						}
					} else {
						if (w->compilation) {
							S_eval(x, w->compilation);
						} else {
							/* TODO: compile */
						}
					}
				} else {
					n = strtol(p, &w, 10);
					if (n == 0 && w == p) {
						/* no valid conversion, word not found */
					} else {
						S_lit(x, n);
					}
				}
			}
		}
		S_trace(x);
	}
}

V S_primitive(X* x, B* n, B* i, B* c) {
  C l = strlen(n);
  S* s = malloc(sizeof(S) + l - sizeof(C) + 1);
  s->previous = x->latest;
  x->latest = s;
  s->interpretation = i;
  s->compilation = c;
  s->nlen = l;
  strcpy(s->name, n);
}

X* SF_init() {
  X* x = S_init();
  x->state = 0;

  /* Primitives could be defined in Sloth by using the \ sigil */
  S_primitive(x, "dup", "d", 0);
  S_primitive(x, "swap", "s", 0);
  S_primitive(x, "drop", "_", 0);
  S_primitive(x, "over", "o", 0);
  S_primitive(x, "rot", "r", 0);

  S_primitive(x, ">r", "(", 0);
  S_primitive(x, "r>", ")", 0);

  S_primitive(x, "+", "+", 0);
  S_primitive(x, "-", "-", 0);
  S_primitive(x, "*", "*", 0);
  S_primitive(x, "/", "/", 0);
  S_primitive(x, "mod", "%", 0);

	S_primitive(x, "bye", "q", 0);

  return x;
}

#endif
