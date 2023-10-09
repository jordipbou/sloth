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
#define DPUSH2(x, u, v) DPUSH(x, u); DPUSH(x, v)
#define DPUSH3(x, u, v, w) DPUSH2(x, u, v); DPUSH(x, w)
#define CPUSH(x, u) (x->ds[--x->cp] = (I)(u))
#define RPUSH(x, u) (x->rs[x->rp++] = (B*)(u))

#define DPOP(x) (x->ds[--x->dp])
#define CPOP(x) (x->ds[x->cp++])
#define RPOP(x) (x->rs[--x->rp])

#define DVAR(x, t, v) t v = (t)DPOP(x)
#define DVAR2(x, t1, v1, t2, v2) DVAR(x, t1, v1); DVAR(x, t2, v2)
#define DVAR3(x, t1, v1, t2, v2, t3, v3) DVAR2(x, t1, v1, t2, v2); DVAR(x, t3, v3)

#define T(x) (x->ds[x->dp - 1])
#define N(x) (x->ds[x->dp - 2])
#define NN(x) (x->ds[x->dp - 3])

#define R(x) (x->rs[x->rp - 1])

#define TAIL_CALL(a) ((a) == 0 || *(a) == 0 || *(a) == 10 || *(a) == '}' || *(a) == ']')

V call(X* x, I i) {
  DVAR(x, B*, q);
  if (!(TAIL_CALL(x->ip + i))) {
    RPUSH(x, x->ip + i);
  }
  x->ip = q;
}

V ret(X* x) { if (x->rp > 0) x->ip = RPOP(x); else x->ip = 0; }

V forward_jump(X* x) {
  I16 d;
  x->ip++;
  DPUSH(x, x->ip + 2);
  d = *((I16*)x->ip);
  x->ip += d;
}

V lit(X* x) { DPUSH(x, *((I16*)x->ip + 1)); x->ip += 3; }
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

V times(X* x) { DVAR2(x, B*, q, I, n); for(;n > 0; n--) eval(x, q); }
V branch(X* x) { DVAR3(x, B*, f, B*, t, I, b); b ? eval(x, t) : eval(x, f); }

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
    case '[': forward_jump(x); return;
    case '{': parse_quotation(x); return;
    case 0: 
    case 10: 
    case ']':
    case '}': ret(x); return;
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
      case '2': DPUSH(x, 2); break;
      
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

/*
#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Word { 
  I16 previous; 
  B flags; 
  I16 code; 
  I16 nlen; 
  B name[1]; 
} W;

#define SYSTEM_SIZE 64*1024

typedef struct _System { 
  I16 here; 
  I16 latest; 
  B* ibuf; 
  B st; 
} S;

#define HEAP(x) (x->s)
#define ADDR(x, a) (x->s + a)
#define ALLOT(x, n) (x->s->here += n)
#define HERE(x) (x->s + x->s->here)

#define TOKEN(cond) (x->s->ibuf && *x->s->ibuf && cond)
V parse_spaces(X* x) { while (TOKEN(isspace(*x->s->ibuf))) { x->s->ibuf++; } }
V parse_non_spaces(X* x) { while (TOKEN(!isspace(*x->s->ibuf))) { x->s->ibuf++; } }
V parse_name(X* x) { 
  parse_spaces(x); 
  DPUSH(x, x->s->ibuf); 
  parse_non_spaces(x); 
  DPUSH(x, (x->s->ibuf - ((B*)DT(x)))); 
}

V find_name(X* x) {
	DVAR2(x, I, l, B*, n);
	I16 wp = x->s->latest;
	while (wp) {
    W* w = (W*)ADDR(x, wp);
		if (w->nlen == l && !strncmp(w->name, n, l)) {
      DPUSH3(x, n, l, wp);
      return;
		}
		wp = w->previous;
	}
  DPUSH3(x, n, l, 0);
}

V asm(X* x) { 
  DVAR2(x, I, l, B*, c); 
  B t = c[l]; 
  c[l] = 0; 
  eval(x, c + 1); 
  c[l] = t;
}

V num(X* x) { 
  DVAR2(x, I, _, B*, s); 
  B* e; 
  I n = strtol(s, &e, 10); 
  if (!n && s == e) { }
  else DPUSH(x, n); 
}

V cbyte(X* x, B b) { *(B*)HERE(x) = b; ALLOT(x, 1); }
V ci16(X* x, I16 l) { *(I16*)HERE(x) = l; ALLOT(x, 2); }
V cword(X* x, W* w) { cbyte(x, '$'); ci16(x, w->code); }
V cnum(X* x, I16 n) { 
  if (n == 0) cbyte(x, '0');
  else if (n == 1) cbyte(x, '1');
  else if (n == 2) cbyte(x, '2');
  else {
    cbyte(x, '#'); 
    ci16(x, n); 
  }
}

V inlined(X* x, W* w) { 
  B* c = (B*)ADDR(x, w->code);
  while (*c) { 
    cbyte(x, *c); 
    c++; 
  } 
}
V compile(X* x) { 
  DVAR3(x, W*, w, I, _, B*, __); 
  if (strlen((B*)ADDR(x, w->code)) < 3) inlined(x, w); 
  else cword(x, w); 
}
V interpret(X* x) { 
  DVAR3(x, W*, w, I, _, B*, __); 
  eval(x, (B*)ADDR(x, w->code)); 
}

W* aword(X* x, I l) { W* w = (W*)(x); x->s->hp += sizeof(W) + l; return w; }
V slatest(X* x, W* w) { w->p = x->s->e->l; x->s->e->l = w; }
V sname(W* w, I l, B* n) { w->l = l; strncpy(w->n, n, l); w->n[l] = 0; }
V scode(X* x, W* w) { w->c = x->s->hp; }
V header(X* x) { DVAR2(x, I, l, B*, n); W* w = aword(x, l); slatest(x, w); sname(w, l, n); scode(x, w); }
V create(X* x) { DVAR2(x, I, _, B*, __); parse_name(x); header(x); x->s->st = 1; }

V semicolon(X* x) { 
  DVAR2(x, I, _, B*, __); 
  x->s->e->l->f &= ~HIDDEN; 
  x->s->st = 0; 
  cbyte(x, 0); 
}

V start_quotation(X* x) { 
	DVAR2(x, I, _, B*, __);
  cbyte(x, '[');
  CPUSH(x, x->s->hp);
  ci16(x, 0);
  if (!x->s->st) DPUSH(x, &x->s->h[x->s->hp]);
  x->s->st++;
}

V end_quotation(X* x) {
	I i;
	DVAR2(x, I, _, B*, __);
  cbyte(x, ']'); 
  i = CPOP(x);
  *((I16*)&x->s->h[i]) = (I16)(0 - (i - x->s->hp));
  x->s->st--;
}

V evaluate(X* x, B* s) {
	I l;
	B* t;
  I i;
  W* w;
	x->s->ibuf = s;
	while (x->s->ibuf && *x->s->ibuf && *x->s->ibuf != 10) {
    dump_context(x);
		parse_name(x);
		if (!DT(x)) { DDROP(x); DDROP(x); return; }
		find_name(x);
		if (DT(x)) {
			if (x->s->st) compile(x);
			else interpret(x);
		} else {
			DDROP(x);
			l = DT(x);
			t = (B*)DN(x);
			if (l == 1 && t[0] == ':') create(x);
      else if (l == 1 && t[0] == ';') semicolon(x);
      else if (l == 1 && t[0] == '[') start_quotation(x);
      else if (l == 1 && t[0] == ']') end_quotation(x);
			else if (t[0] == '\\') {
				if (x->s->st) { 
          DDROP(x); DDROP(x);
          for (i = 1; i < l; i++) {
            cbyte(x, t[i]);
          }
				} else {
					parse_spaces(x);
					asm(x);
				}
			} else {
				num(x);
        if (x->s->st) cnum(x, DPOP(x));
			}
		}
	}
}

V see_word(X* x) {
  DVAR(x, W*, w);
  printf(": %.*s %s ;\n", w->l, w->n, &x->s->h[w->c]);  
}

V sloth_ext(X* x) {
  x->ip++;
  switch (*x->ip) {
    case 'f': find_name(x); break;
    case 'l': DPUSH(x, x->s->e->l); break;
    case 'p': parse_name(x); break;
    case 's': see_word(x); break;
  } 
}

X* init_SLOTH() {
  X* x = init_EXT(init_VM());
  x->s = malloc(SYSTEM_SIZE);
  x->s->latest = 0;
  x->s->here += sizeof(E);

  EXT(x, 'S') = &sloth_ext;

  evaluate(x, ": dup \\d ; : drop \\_ ; : + \\+ ;");
  evaluate(x, ": swap \\s ; : - \\- ; : times \\t ;");
  evaluate(x, ": over \\o ; : execute \\e ;");
  evaluate(x, ": fib 2 - 1 swap 1 swap [ swap over + ] times swap drop ;");
  
  return x;
}
*/
#endif
