#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy, memset, strncmp */

#define V void /* inline void ? */

typedef char B;
typedef intptr_t C;

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _W { struct _W* l; B* c; B f; B s; B n[1]; } W;

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

#define S_DEFAULT_DICT_SIZE 8192

#define S_BLOCK_SIZE(x) (((C*)x->b)[0])
#define S_HERE(x) (((C*)x->b)[1])
#define S_LATEST(x) (((C*)x->b)[2])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(C));
  x->r = malloc(RSTACK_SIZE*sizeof(C));
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

void S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])

V S_lit(X* x, C v) { x->s[x->sp++] = v; }
#define S_STR(x, s) S_lit(x, (C)s); S_lit(x, (C)strlen(s))

/* Parsing */

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
  S_lit(x, (C)(++x->ip));
  while (S_token(x) != '"') {}
  S_lit(x, (C)(x->ip - TS(x)) - 1);
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

/* TODO: Modify inspect to use key/emit */
V S_inspect(X* x) {
  C i = 0, j;
  B* a = (B*)S_drop(x);
	for (i = 0; i < 6; i++) {
		printf("\n%p ", a + 8*i);
		for (j = 0; j < 8; j++) {
			printf("%02X ", (unsigned char)a[8*i + j]);
		}
		for (j = 0; j < 8; j++) {
			if (a[8*i + j] < 32 || a[8*i + j] > 126) printf(".");
			else printf("%c", a[8*i + j]);
		}
	}
	printf("\n");
}

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

V S_create(X* x) {
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

V S_find(X* x) {
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

V S_parse_symbol(X* x) {
  C l = 0;
  B* s = x->ip;
  while (!isspace(S_token(x))) { l++; }
  S_lit(x, (C)s);
  S_lit(x, l);
}

V S_symbol(X* x, C c) {
	W* w;
	/* TODO: S_init_block(x); */
  if (x->b == 0) {
    x->b = malloc(S_DEFAULT_DICT_SIZE);
    S_BLOCK_SIZE(x) = S_DEFAULT_DICT_SIZE;
    S_HERE(x) = 3*sizeof(C);
    S_LATEST(x) = 0;
  }
  S_parse_symbol(x);
  S_find(x);
  if (TS(x)) {
    TS(x) = (C)(((W*)TS(x))->c);
    if (c) S_call(x);
  } else {
    S_drop(x);
    S_create(x);
  }
}

V S_allot(X* x) { S_HERE(x) += S_drop(x); }

V S_qcompile(X* x, C e) { 
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

void S_load_file(X* x) {
  C l = S_drop(x); 
  B* s = (B*)S_drop(x);
  FILE* fptr;
  B buf[1024];
  B tmp[1024];
  strncpy(tmp, s, l);
  tmp[l] = 0;
  printf("Trying to open: %s\n", tmp);
  fptr = fopen(tmp, "r");
  if (!fptr) {
    printf("Can't load file\n");
    return;
  }
  while (fgets(buf, 255, fptr)) {
    S_eval(x, buf);
  }
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
    case '[': 
      S_parse_quotation(x); break;
    case '"':
      S_parse_string(x); break;
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
      case 'y': exit(0); break;
      /* Helpers */
      case '?': S_branch(x); break;
      case 't': S_times(x); break;
      case 'w': S_while(x); break;
      case 'u': x->tr = 0; break;
      case 'v': x->tr = 1; break;
      /* Memory */
      case 'b': S_lit(x, (C)(&x->b)); break;
      case 'c': S_lit(x, sizeof(C)); break;
      case '{': S_inspect(x); break;
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      case '.': S_cfetch(x); break;
      case ':': S_bfetch(x); break;
      case ',': S_cstore(x); break;
      case ';': S_bstore(x); break;
      /* Dictionary */
      case 'a': S_allot(x); break;
      case 'h': S_create(x); break;
      case '\\': S_symbol(x, 0); break;
      case '`': S_find(x); break;
      case '$': S_symbol(x, 1); break;
      case 'g': S_qcompile(x, -1); break;
      case 'q': S_qcompile(x, 0); break;
      /* Input/output */
      case 'k': x->key(x); break;
      case 'e': x->emit(x); break;
      case 'l': S_load_file(x); break;
      }
    }
  } while(1);
}

#endif
