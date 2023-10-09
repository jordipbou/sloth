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

typedef struct _Context X;

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Word { struct _Word* p; B f; I16 c; I16 l; B n[1]; } W;
typedef struct _Environment {	struct _Environment* p;	W* l; } E;

#define HEAP_SIZE 64*1024

typedef struct _System { B* ibuf; I16 hp; B h[HEAP_SIZE];	E* e;	B st; } S;

typedef V (*F)(X*);

#define DSIZE 64
#define RSIZE 64

struct _Context { I ds[DSIZE]; I dp; I cp; B* rs[RSIZE]; I rp; B* ip; S* s; F* x; };

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

#define DT(x) (x->ds[x->dp - 1])
#define DN(x) (x->ds[x->dp - 2])
#define DNN(x) (x->ds[x->dp - 3])

#define RT(x) (x->rs[x->rp - 1])

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

#define DDROP(x) (x->dp--)
V swap(X* x) { I t = DT(x); DT(x) = DN(x); DN(x) = t; }
V over(X* x) { DPUSH(x, DN(x)); }
V dup(X* x) { DPUSH(x, DT(x)); }
V rot(X* x) { I t = DNN(x); DNN(x) = DN(x); DN(x) = DT(x); DT(x) = t; }

V iadd(X* x) { DN(x) = DN(x) + DT(x); DDROP(x); }
V isub(X* x) { DN(x) = DN(x) - DT(x); DDROP(x); }
V imul(X* x) { DN(x) = DN(x) * DT(x); DDROP(x); }
V idiv(X* x) { DN(x) = DN(x) / DT(x); DDROP(x); }
V imod(X* x) { DN(x) = DN(x) % DT(x); DDROP(x); }

V and(X* x) { DN(x) = DN(x) & DT(x); DDROP(x); }
V or(X* x) { DN(x) = DN(x) | DT(x); DDROP(x); }
V xor(X* x) { DN(x) = DN(x) ^ DT(x); DDROP(x); }
V not(X* x) { DT(x) = !DT(x); }
V inverse(X* x) { DT(x) = ~DT(x); }

V lt(X* x) { DN(x) = DN(x) < DT(x); DDROP(x); }
V eq(X* x) { DN(x) = DN(x) == DT(x); DDROP(x); }
V gt(X* x) { DN(x) = DN(x) > DT(x); DDROP(x); }

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
  printf("[%ld] ", x->s->st);
  for (i = 0; i < x->dp; i++) printf("%ld ", x->ds[i]);
  printf(".. ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" : ");
    dump_code(x, x->rs[i]); 
  }
  printf("[%ld]", x->rp);
  printf(" <%s>", x->s->ibuf);
  printf("\n");
}
              
V step(X* x) {
  dump_context(x);
  switch (*x->ip) {
    case '[': forward_jump(x); return;
    case '{': parse_quotation(x); return;
    case 0: 
    case 10: 
    case ']':
    case '}': ret(x); return;
    case 'e': call(x, 1); return;
    case '0': DPUSH(x, 0); break;
    case '1': DPUSH(x, 1); break;
    case '2': DPUSH(x, 2); break;
    case '#': DPUSH(x, *((I16*)(x->ip + 1))); x->ip += 3; return;
    /* This one needs access to system, should be an extension */
    case '$': DPUSH(x, x->s->h + *((I16*)(x->ip + 1))); x->ip += 3; call(x, 0); return;
    case '_': DDROP(x); break;
    case 's': swap(x); break;
    case 'o': over(x); break;
    case 'd': dup(x); break;
    case 'r': rot(x); break;
    case '+': iadd(x); break;
    case '-': isub(x); break;
    case '*': imul(x); break;
    case '/': idiv(x); break;
    case '%': imod(x); break;
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
    case 'w': /* word/s inspection */ break;
    case 'x': /* context reflection */ break;
    case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, *x->ip)(x);
      break;
  }
  x->ip++;
}
              
V inner(X* x) { I rp = x->rp; while(x->rp >= rp && x->ip) step(x); }
V eval(X* x, B* q) { DPUSH(x, q); call(x, 0); inner(x); }

X* init_VM() { return malloc(sizeof(X)); }
X* init_EXT(X* x) { x->x = malloc(26*sizeof(F*)); return x; }

/* Start of REPL/state based part/word definitions */

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
	E* e = x->s->e;
	while (e) {
		W* w = e->l;
		while (w) {
			if (w->l == l && !strncmp(w->n, n, l)) {
        DPUSH3(x, n, l, w);
        return;
			}
			w = w->p;
		}
		e = e->p;
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
  if (!n && s == e) { /* Conversion error */ }
  else DPUSH(x, n); 
}

B* here(X* x) { return &x->s->h[x->s->hp]; }

/* Compilation */

V cbyte(X* x, B b) { x->s->h[x->s->hp] = b; x->s->hp += 1; }
V ci16(X* x, I16 l) { *((I16*)&x->s->h[x->s->hp]) = l; x->s->hp += 2; }
V cword(X* x, W* w) { cbyte(x, '$'); ci16(x, w->c); }
V cnum(X* x, I16 n) { 
  if (n == 0) cbyte(x, '0');
  else if (n == 1) cbyte(x, '1');
  else if (n == 2) cbyte(x, '2');
  else {
    cbyte(x, '#'); 
    ci16(x, n); 
  }
}

V compile_inline(X* x, W* w) { B* c = x->s->h + w->c; while (*c) { cbyte(x, *c); c++; } }
V compile(X* x) { DVAR3(x, W*, w, I, _, B*, __); if (strlen(x->s->h + w->c) < 3) compile_inline(x, w); else cword(x, w); }
V interpret(X* x) { DVAR3(x, W*, w, I, _, B*, __); eval(x, x->s->h + w->c); }

W* aword(X* x, I l) { W* w = (W*)here(x); x->s->hp += sizeof(W) + l; return w; }
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
      /* This could be extracted to an extension, except assembler */
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
  x->s = malloc(sizeof(S));
  x->s->e = (E*)x->s->h;
  x->s->e->p = 0;
  x->s->e->l = 0;
  x->s->hp += sizeof(E);

  /* Add extension */
  EXT(x, 'S') = &sloth_ext;

  /* Add the necessary words */
  evaluate(x, ": dup \\d ; : drop \\_ ; : + \\+ ;");
  evaluate(x, ": swap \\s ; : - \\- ; : times \\t ;");
  evaluate(x, ": over \\o ; : execute \\e ;");
  evaluate(x, ": fib 2 - 1 swap 1 swap [ swap over + ] times swap drop ;");
  
  return x;
}

#endif
