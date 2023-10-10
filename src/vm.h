#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

typedef void V;
typedef char B;
typedef int16_t I16;
typedef intptr_t I;

typedef struct _Symbol {
  struct _Symbol* p;
  B f;
  I16 c;
  I16 l;
  B n[1];
} S;

#define DSTACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context { 
  I ds[DSTACK_SIZE]; 
  I dp; 
  I cp; 
  B* rs[RSTACK_SIZE]; 
  I rp; 
  B* ip; 
  B* d; 
  V (**x)(struct _Context*);
  S* l;
} X;

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

V istore(X* x) { L2(x, I*, a, I, v); *a = v; }
V bstore(X* x) { L2(x, B*, a, B, v); *a = v; }
V ifetch(X* x) { L1(x, I*, a); DPUSH(x, *a); }
V bfetch(X* x) { L1(x, B*, a); DPUSH(x, *a); }

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

V parse_symbol(X* x) {
  L1(x, B**, i);
  while (*i && **i && isspace(**i)) (*i)++;
  DPUSH(x, *i);
  while (*i && **i && !isspace(**i)) (*i)++;
  DPUSH(x, *i - T(x));
}

V find_symbol(X* x) {
  L2(x, I, l, B*, n);
  S* s = x->l;
  while (s) {
    if (s->l == l && !strncmp(s->n, n, l)) break;
    s = s->p;
  }
  DPUSH(x, s);
}

V create_symbol(X* x) {
  L2(x, I, l, B*, n);
  S* s = malloc(sizeof(S) + l);
  s->p = x->l;
  x->l = s;
  s->f = 0;
  s->c = 0;
  s->l = l;
  strncpy(s->n, n, l);
  s->n[l] = 0;
}

#define PEEK(x) (*x->ip)
#define TOKEN(x) (*(x->ip++))
                
V step(X* x) {
  dump_context(x);
  switch (PEEK(x)) {
    case '[': qjump(x); return;
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
      case '{': parse_quotation(x); return;
      case '\'': DPUSH(x, &x->ip); parse_symbol(x); return;
      case 'f': find_symbol(x); break;
      case 'c': create_symbol(x); break;
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
      case ',': istore(x); break;
      case ';': bstore(x); break;
      case '.': ifetch(x); break;
      case ':': bfetch(x); break;
      case 't': times(x); break;
      case '?': branch(x); break;
      case 'x': /* context reflection */ break;
    }
  }
}
              
V inner(X* x) { I rp = x->rp; while(x->rp >= rp && x->ip) step(x); }
V eval(X* x, B* q) { DPUSH(x, q); call(x, 0); inner(x); }

X* init_VM() { return malloc(sizeof(X)); }
X* init_EXT(X* x) { x->x = malloc(26*sizeof(I)); return x; }

#endif