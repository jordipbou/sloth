#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

/* DODO Extensible Virtual Machine */

typedef void V;
typedef char B;
typedef int16_t I16;
typedef intptr_t I;

#define DSTACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context X;
typedef V (*F)(X*);

struct _Context { 
  I ds[DSTACK_SIZE]; 
  I dp; 
  I cp; 
  B* rs[RSTACK_SIZE]; 
  I rp; 
  B* ip; 
  B* d; 
  F* x; 
};

#define EXT(x, l) (x->x[l - 'A'])

#define DPUSH(x, u) (x->ds[x->dp++] = (I)(u))
#define CPUSH(x, u) (x->ds[--x->cp] = (I)(u))

#define DPOP(x) (x->ds[--x->dp])
#define CPOP(x) (x->ds[x->cp++])

#define L1(x, t, v) t v = (t)DPOP(x)
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)

#define T(x) (x->ds[x->dp - 1])
#define N(x) (x->ds[x->dp - 2])
#define NN(x) (x->ds[x->dp - 3])

#define TAIL(a) ((a) == 0 || *(a) == 0 || *(a) == 10 || *(a) == '}' || *(a) == ']')
V save_ip(X* x, I i) { x->rs[x->rp++] = x->ip + i; }
V call(X* x, I i) { L1(x, B*, q); if (!TAIL(x->ip + i)) save_ip(x, i); x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->rs[--x->rp]; else x->ip = 0; }
V qjump(X* x) { I16 d; DPUSH(x, x->ip + 3); d = *((I16*)x->ip); x->ip += d + 1; }

V lit(X* x) { DPUSH(x, *((I16*)(x->ip + 1))); x->ip += 3; }
V word(X* x) { lit(x); T(x) += (I)x->d; call(x, 0); }
  
#define DDROP(x) (x->dp--)
V swap(X* x) { I t = T(x); T(x) = N(x); N(x) = t; }
V over(X* x) { DPUSH(x, N(x)); }
V dup(X* x) { DPUSH(x, T(x)); }
V rot(X* x) { I t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }

#define OP2(x, op) N(x) = N(x) op T(x); DDROP(x)
V add(X* x) { OP2(x, +); }
V sub(X* x) { OP2(x, -); }
V mul(X* x) { OP2(x, *); }
V division(X* x) { OP2(x, /); }
V mod(X* x) { OP2(x, %); }

V and(X* x) { OP2(x, &); }
V or(X* x) { OP2(x, |);}
V xor(X* x) { OP2(x, ^); }
V not(X* x) { T(x) = !T(x); }
V inverse(X* x) { T(x) = ~T(x); }

V lt(X* x) { OP2(x, <); }
V eq(X* x) { OP2(x, ==); }
V gt(X* x) { OP2(x, >); }

V eval(X* x, B* q);

V times(X* x) { L2(x, B*, q, I, n); for(;n > 0; n--) eval(x, q); }
V branch(X* x) { L3(x, B*, f, B*, t, I, b); b ? eval(x, t) : eval(x, f); }

#define BLOCK(a, b) \
  { \
    I t = 1; \
    while (t) { \
      if ((a) == 0 || *(a) == 0 || *(a) == 10) break; \
      if (*(a) == '{') t++; \
      if (*(a) == '}') t--; \
      b; \
      (a)++; \
    } \
  }

V parse_quotation(X* x) { x->ip++; DPUSH(x, x->ip); BLOCK(x->ip, {}); }
 
#define S_pr(s, n, f, a) { C t; s += t = sprintf(s, f, a); n += t; }
              
V dump_code(X* x, B* c) {
  I t = 1;
  while (t) {
    if (c == 0 || *c == 0 || *c == 10) break;
    if (*c == '{') t++;
    if (*c == '}') t--;
    if (*c == '#') { printf("#%d", *((I16*)(c+1))); c += 3; }
    if (*c == '[') { printf("["); c += 3; }
    else {
      printf("%c", *c);
      c++;
    }
  }
}
V dump_context(X* x) {
  I i;
  B* t;
  for (i = 0; i < x->dp; i++) printf("%ld ", x->ds[i]);
  printf(": ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" : ");
    dump_code(x, x->rs[i]); 
  }
  printf("\n");
}

#define PEEK(x) (*x->ip)
#define TOKEN(x) (*(x->ip++))
                
V step(X* x) {
  dump_context(x);
  switch (PEEK(x)) {
    case '[': qjump(x); return;
    case '{': parse_quotation(x); return;
    case 0: case 10: 
    case ']': case '}': 
      ret(x); return;
    case 'e': call(x, 1); return;
    case '#': lit(x); return;
    case '$': word(x); return;
    case 'A': case 'B': 
    case 'C': case 'D':
    case 'E': case 'F': 
    case 'G': case 'H':
    case 'I': case 'J': 
    case 'K': case 'L':
    case 'M': case 'N': 
    case 'O': case 'P':
    case 'Q': case 'R': 
    case 'S': case 'T':
    case 'U': case 'V': 
    case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, TOKEN(x))(x);
      break;
    default:
      switch (TOKEN(x)) {
      case '0': DPUSH(x, 0); break;
      case '1': DPUSH(x, 1); break;
      case '_': DDROP(x); break;
      case 's': swap(x); break;
      case 'o': over(x); break;
      case 'd': dup(x); break;
      case 'r': rot(x); break;
      case '+': add(x); break;
      case '-': sub(x); break;
      case '*': mul(x); break;
      case '/': division(x); break;
      case '%': mod(x); break;
      case '&': and(x); break;
      case '|': or(x); break;
      case '!': not(x); break;
      case '^': xor(x); break;
      case '~': inverse(x); break;
      case '<': lt(x); break;
      case '=': eq(x); break;
      case '>': gt(x); break;
      case 't': times(x); break;
      case '?': branch(x); break;
      case 'x': /* context reflection */ break;
    }
  }
}
              
V inner(X* x) { I rp = x->rp; while(x->rp >= rp && x->ip) step(x); }
V eval(X* x, B* q) { DPUSH(x, q); call(x, 0); inner(x); }

X* init_VM() { return malloc(sizeof(X)); }
X* init_EXT(X* x) { x->x = malloc(26*sizeof(F*)); return x; }

/* SLOTH Extensible language */

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Word { 
  I16 p; 
  B f; 
  I16 c; 
  I16 l; 
  B n[1]; 
} W;

#define DICT_SIZE 64*1024

typedef struct _Dictionary { 
  I16 h; 
  I16 l; 
  B* i; 
  B s; 
} DICT;

#define D(x) ((DICT*)x->d)
#define IBUF(x) (D(x)->i)

#define TO_IDX(x, p) (((I)p) - ((I)x->d))
#define TO_PTR(x, i) (((I)i) + ((I)x->d))

#define HERE(x) (D(x)->h)
#define THERE(x) (TO_PTR(x, HERE(x)))
#define ALLOT(x, n) (HERE(x) += n)

/* Compilation */

#define C(x, t, v) *((t*)THERE(x)) = v; ALLOT(x, sizeof(t))

V cword(X* x, W* w) { C(x, B, '$'); C(x, I16, w->c); }

V cnum(X* x, I16 n) { 
  if (n == 0) { C(x, B, '0'); }
  else if (n == 1) { C(x, B, '1'); }
  else {
    C(x, B, '#'); 
    C(x, I16, n); 
  }
}
                
V inlined(X* x, W* w) { 
  B* c = (B*)TO_PTR(x, w->c);
  while (*c) { 
    C(x, B, *c); 
    c++; 
  } 
}
                
V compile(X* x) { 
  L3(x, W*, w, I, _, B*, __); 
  if (strlen((B*)TO_PTR(x, w->c)) < 3) {
    inlined(x, w);
  } else {
    cword(x, w); 
  }
}

/* Parsing */

#define CHAR(cond) (IBUF(x) && *(IBUF(x)) && cond)
V parse_spaces(X* x) { while (CHAR(isspace(*IBUF(x)))) { IBUF(x)++; } }
V parse_non_spaces(X* x) { while (CHAR(!isspace(*IBUF(x)))) { IBUF(x)++; } }
V parse_name(X* x) { 
  parse_spaces(x); 
  DPUSH(x, IBUF(x)); 
  parse_non_spaces(x); 
  DPUSH(x, IBUF(x) - ((B*)T(x))); 
}

V find_name(X* x) {
	L2(x, I, l, B*, n);
	I16 wa = D(x)->l;
	while (wa) {
    W* w = (W*)TO_PTR(x, wa);
		if (w->l == l && !strncmp(w->n, n, l)) {
      DPUSH(x, n);
      DPUSH(x, l);
      DPUSH(x, w);
      return;
		}
		wa = w->p;
	}
  DPUSH(x, n);
  DPUSH(x, l);
  DPUSH(x, 0);
}

V asm(X* x) { 
  L2(x, I, l, B*, c); 
  B t = c[l]; 
  c[l] = 0; 
  eval(x, c + 1); 
  c[l] = t;
}

V num(X* x) { 
  L2(x, I, _, B*, s); 
  B* e; 
  I n = strtol(s, &e, 10); 
  if (!n && s == e) { }
  else DPUSH(x, n); 
}

V interpret(X* x) { 
  L3(x, W*, w, I, _, B*, __); 
  eval(x, (B*)TO_PTR(x, w->c)); 
}

/* Word definition */

V header(X* x) { 
  L2(x, I, l, B*, n); 
  W* w = (W*)THERE(x);
  ALLOT(x, sizeof(W) + l); 
  w->p = D(x)->l;
  D(x)->l = TO_IDX(x, w);
  w->l = l;
  strncpy(w->n, n, l);
  w->n[l] = 0;
  w->c = HERE(x);
}

V create(X* x) { 
  L2(x, I, _, B*, __); 
  parse_name(x); 
  header(x); 
  D(x)->s = 1; 
}

V semicolon(X* x) { 
  L2(x, I, _, B*, __); 
  ((W*)TO_PTR(x, D(x)->l))->f = ~HIDDEN;
  D(x)->s = 0; 
  C(x, B, 0); 
}

V start_quotation(X* x) { 
	L2(x, I, _, B*, __);
  C(x, B, '[');
  CPUSH(x, HERE(x));
  C(x, I16, 0);
  if (!D(x)->s) DPUSH(x, THERE(x));
  D(x)->s++;
}

V end_quotation(X* x) {
	I i;
	L2(x, I, _, B*, __);
  C(x, B, ']'); 
  i = CPOP(x);
  *((I16*)THERE(x)) = (I16)(0 - (i - HERE(x)));
  D(x)->s--;
}

V evaluate(X* x, B* s) {
	I l;
	B* t;
  I i;
  W* w;
	IBUF(x) = s;
	while (IBUF(x) && *IBUF(x) && *IBUF(x) != 10) {
    dump_context(x);
		parse_name(x);
		if (!T(x)) { DDROP(x); DDROP(x); return; }
		find_name(x);
		if (T(x)) {
			if (D(x)->s) compile(x);
			else interpret(x);
		} else {
			DDROP(x);
			l = T(x);
			t = (B*)N(x);
			if (l == 1 && t[0] == ':') create(x);
      else if (l == 1 && t[0] == ';') semicolon(x);
      else if (l == 1 && t[0] == '[') start_quotation(x);
      else if (l == 1 && t[0] == ']') end_quotation(x);
			else if (t[0] == '\\') {
				if (D(x)->s) { 
          DDROP(x); DDROP(x);
          for (i = 1; i < l; i++) {
            C(x, B, t[i]);
          }
				} else {
					parse_spaces(x);
					asm(x);
				}
			} else {
				num(x);
        if (D(x)->s) cnum(x, DPOP(x));
			}
		}
	}
}

/*
V sloth_ext(X* x) {
  x->ip++;
  switch (*x->ip) {
    case 'f': find_name(x); break;
    case 'l': DPUSH(x, x->s->e->l); break;
    case 'p': parse_name(x); break;
    case 's': see_word(x); break;
  } 
}
*/

X* init_SLOTH() {
  X* x = init_EXT(init_VM());
  x->d = malloc(DICT_SIZE);
  D(x)->l = 0;
  HERE(x) += sizeof(DICT);

/*
  EXT(x, 'S') = &sloth_ext;
*/
/*
  evaluate(x, ": dup \\d ; : drop \\_ ; : + \\+ ;");
  evaluate(x, ": swap \\s ; : - \\- ; : times \\t ;");
  evaluate(x, ": over \\o ; : execute \\e ;");
  evaluate(x, ": fib 2 - 1 swap 1 swap [ swap over + ] times swap drop ;");
*/ 
  return x;
}

#endif