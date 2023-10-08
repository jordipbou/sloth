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

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Word { struct _Word* p; B f; I16 c; I16 l; B n[1]; } W;
typedef struct _Environment {	struct _Environment* p;	W* l; } E;

#define HEAP_SIZE 64*1024

typedef struct _System {
  B* ibuf;                 /* Input buffer */
  I hp;                    /* Heap pointer (HERE) */
  B h[HEAP_SIZE];          /* Heap */
	E* e;                    /* Environment */
	B st;                    /* Compilation state */
} S;

#define DSTACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context {
  I ds[DSTACK_SIZE];       /* Data stack */
  I dp;                    /* Data stack pointer */ 
  I cp;                    /* Control flow stack pointer -uses top of data stack- */
  B* rs[RSTACK_SIZE];      /* Return stack */
  I rp;                    /* Return stack pointer */
	B* ip;                   /* Instruction pointer */
	struct _System* s;       /* System (input buffer, memory) */
} X;

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

V ret(X* x) { x->ip = x->ip ? RPOP(x) : 0; }

V forward_jump(X* x) {
  I16 d;
  x->ip++;
  DPUSH(x, x->ip + 2);
  d = *((I16*)x->ip);
  x->rp += d;
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
  printf(".. ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" : ");
    dump_code(x, x->rs[i]); 
  }
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
    case '#': DPUSH(x, *((I16*)(x->ip + 1))); x->ip += 3; return;
    case '$': DPUSH(x, x->s->h + *((I16*)(x->ip + 1))); x->ip += 3; return;
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
  }
  x->ip++;
}
              
V inner(X* x) { I rp = x->rp; while(x->rp >= rp && x->ip) step(x); }
V eval(X* x, B* q) { DPUSH(x, q); call(x, 0); inner(x); }

X* init_VM() { return malloc(sizeof(X)); } 


#define TOKEN(cond) (x->s->ibuf && *x->s->ibuf && cond)
V spaces(X* x) { while (TOKEN(isspace(*x->s->ibuf))) { x->s->ibuf++; } }
V non_spaces(X* x) { while (TOKEN(!isspace(*x->s->ibuf))) { x->s->ibuf++; } }
V parse_name(X* x) { spaces(x); DPUSH(x, x->s->ibuf); non_spaces(x); DPUSH(x, (x->s->ibuf - ((B*)DT(x)))); }

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

V asm(X* x) { DVAR2(x, I, l, B*, c); B t = c[l]; c[l] = 0; eval(x, c + 1); c[l] = t; }
V num(X* x) { DVAR2(x, I, _, B*, s); B* e; I n = strtol(s, &e, 10); (!n && s == e) ? 0 : DPUSH(x, n); }

B* hr(X* x) { return &x->s->h[x->s->hp]; }
V bc(X* x, B b) { x->s->h[x->s->hp] = b; x->s->hp += 1; }
V lc(X* x, I16 l) { *((I16*)&x->s->h[x->s->hp]) = l; x->s->hp += 2; }
V wc(X* x, W* w) { bc(x, '$'); lc(x, w->c); }
V nc(X* x, I16 n) { bc(x, '#'); lc(x, n); }

V compile_inline(X* x, W* w) { B* c = x->s->h + w->c; while (c) { bc(x, *c); c++; } }
V compile(X* x) { DVAR3(x, W*, w, I, _, B*, __); if (strlen(x->s->h + w->c) < 3) compile_inline(x, w); else wc(x, w); }
V interpret(X* x) { DVAR3(x, W*, w, I, _, B*, __); eval(x, x->s->h + w->c); }

W* aword(X* x, I l) { W* w = (W*)hr(x); x->s->hp += sizeof(W) + l; return w; }
V slatest(X* x, W* w) { w->p = x->s->e->l; x->s->e->l = w; }
V sname(W* w, I l, B* n) { w->l = l; strncpy(w->n, n, l); w->n[l] = 0; }
V scode(X* x, W* w) { w->c = x->s->hp; }
V header(X* x) { DVAR2(x, I, l, B*, n); W* w = aword(x, l); slatest(x, w); sname(w, l, n); scode(x, w); }
V create(X* x) { DVAR2(x, I, _, B*, __); parse_name(x); header(x); x->s->st = 1; }

V reveal(X* x) { DVAR2(x, I, _, B*, __); x->s->e->l->f &= ~HIDDEN; x->s->st = 0; bc(x, 0); }

V start_quotation(X* x) { 
	DVAR2(x, I, _, B*, __);
  bc(x, '[');
  CPUSH(x, x->s->hp);
  lc(x, 0);
  if (!x->s->st) DPUSH(x, &x->s->h[x->s->hp]);
  x->s->st++;
}

V end_quotation(X* x) {
	I i;
	DVAR2(x, I, _, B*, __);
  bc(x, ']'); 
  i = CPOP(x);
  *((I16*)&x->s->h[i]) = (I16)(0 - (i - x->s->hp));
  x->s->st--;
}

V S_evaluate(X* x, B* s) {
	I l;
	B* t;
  I i;
  W* w;
	x->s->ibuf = s;
	while (x->s->ibuf && *x->s->ibuf && *x->s->ibuf != 10) {
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
      else if (l == 1 && t[0] == ';') reveal(x);
      else if (l == 1 && t[0] == '[') start_quotation(x);
      else if (l == 1 && t[0] == ']') end_quotation(x);
			else if (t[0] == '\\') {
				if (x->s->st) { 
          DDROP(x); DDROP(x);
          for (i = 1; i < l; i++) {
            bc(x, t[i]);
          }
				} else {
					spaces(x);
					asm(x);
				}
			} else {
				num(x);
        if (x->s->st) {
          bc(x, '#');
          lc(x, DPOP(x));
        }
			}
		}
	}
}

X* init_SLOTH() {
  X* x = init_VM();
  x->s = malloc(sizeof(S));
  x->s->e = (E*)x->s->h;
  x->s->e->p = 0;
  x->s->e->l = 0;
  x->s->hp += sizeof(E);
  return x;
}

/*
typedef struct _Word { struct _Word* p; C f; B* c; C l;	B n[1]; } W;
typedef struct _Environment {	struct _Environment* p;	W* l; } E;

#ifndef DICT_SIZE
#define DICT_SIZE 64*1024
#endif

typedef struct _System {
} S;

S* S_init_system() { return malloc(sizeof(S)); }


typedef struct _Context {
  C ds[STACK_SIZE]; C sp;
  B* as[RSTACK_SIZE]; C rp;
	B* ip;
  C err;
	S* s;
  V (*ext[26])(struct _Context*);
  V* st[26];
  B* ibuf;
  E* e;
} X;

#define NNF	-1

#define EXT(x, l) (x->ext[l - 'A'])
#define ST(x, l) (x->st[l - 'A'])

#define S_push(x, v) (x->ds[x->sp++] = (C)(v))
#define P1(x, v1) S_push(x, v1)
#define P2(x, v1, v2) S_push(x, v1); S_push(x, v2)
#define P3(x, v1, v2, v3) S_push(x, v1); S_push(x, v2); S_push(x, v3)

#define T(x) (x->ds[x->sp - 1])
#define N(x) (x->ds[x->sp - 2])
#define NN(x) (x->ds[x->sp - 3])
#define R(x) (x->as[x->rp - 1])

V S_eval(X* x, B* s);
V S_trace(X* x);

C S_block(X* x, B* tk) { 
	C t = 1; 
	B* s = tk;
	for (;t && tk && *tk && *tk != 10; tk++) { 
		if (*tk == '{') { t++; }
		if (*tk == '}' || *tk == ']') { t--; }
	}
	return tk - s;
}
V S_parse_block(X* x) { S_push(x, x->ip + 1); x->ip += S_block(x, x->ip + 1); }
C is_number(B c) { return c >= 48 && c <= 57; }
V S_parse_number(X* x) { C n = 0; x->ip += 1; while (is_number(*x->ip)) { n = n*10 + ((*x->ip++) - 48); } x->ip -= 1; S_push(x, n); }
V S_char(X* x) { x->ip += 1; S_push(x, *x->ip); }
V S_lit(X* x) { S_push(x, ((*x->ip) << 8 + *(x->ip + 1)) & 0xEFFF); x->ip++; }

V S_quit(X* x) { exit(0); }

V S_dup(X* x) { S_push(x, T(x)); }
C S_drop(X* x) { return x->ds[--x->sp]; }
V S_swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V S_over(X* x) { S_push(x, N(x)); }

V S_to_R(X* x) { x->as[x->rp++] = (B*)x->ds[--x->sp]; }
V S_from_R(X* x) { x->ds[x->sp++] = (C)x->as[--x->rp]; }

V S_save_ip(X* x) { if (x->ip != 0 && *x->ip != 0 && *x->ip != 10) x->as[x->rp++] = x->ip; }
V S_jump(X* x) { x->ip = (B*)S_drop(x); }
V S_call(X* x) { S_save_ip(x); S_jump(x); }
V S_return(X* x) { if (x->rp > 0) { x->ip = R(x) - 1; x->rp--; } else { x->rp = 0; x->ip = (B*)-1; } }

V S_eq(X* x) { N(x) = (N(x) == T(x)) ? -1 : 0; x->sp--; }
V S_neq(X* x) { N(x) = (N(x) != T(x)) ? -1 : 0; x->sp--; }
V S_lt(X* x) { N(x) = (N(x) < T(x)) ? -1 : 0; x->sp--; }
V S_gt(X* x) { N(x) = (N(x) > T(x)) ? -1 : 0; x->sp--; }

V S_fetch(X* x) { T(x) = x->s->m[T(x)]; }
V S_store(X* x) { x->s->m[T(x)] = N(x); x->sp -= 2; }

V S_add(X* x) { N(x) = N(x) + T(x); x->sp -= 1; }
V S_sub(X* x) { N(x) = N(x) - T(x); x->sp -= 1; }
V S_mul(X* x) { N(x) = N(x) * T(x); x->sp -= 1; }
V S_divmod(X* x) { C a = T(x); C b = N(x); T(x) = b / a; N(x) = b % a; }

V S_and(X* x) { N(x) = N(x) & T(x); x->sp -= 1; }
V S_or(X* x) { N(x) = N(x) | T(x); x->sp -= 1; }
V S_xor(X* x) { N(x) = N(x) ^ T(x); x->sp -= 1; }
V S_not(X* x) { T(x) = !T(x); }
V S_invert(X* x) { T(x) = ~T(x); }

V S_shl(X* x) { N(x) = N(x) << T(x); x->sp -= 1; }
V S_shr(X* x) { N(x) = N(x) >> T(x); x->sp -= 1; }

V S_times(X* x) {	B* q = (B*)S_drop(x);	C n = S_drop(x); for (;n > 0; n--) { S_eval(x, q); } }

#define DC(a) { b += m = sprintf(b, "%ld ", a); n += m; }
C D_co(X* x, B* b, B* c) { return sprintf(b, "%.*s", (int)S_block(x, c), c); }
C D_ds(X* x, B* b) { C i = 0; C n = 0; C m = 0; for (;i < x->sp; i++) DC(x->ds[i]); return n; }
C D_ctx(X* x, B* b) {
	C i; C n = 0; C m = 0;
	b += m = D_ds(x, b); n += m;
	b += m = sprintf(b, "| "); n += m;
	b += m = D_co(x, b, x->ip); n += m;
	for (i = x->rp - 1; i >= 0; i--) { 
		b += m = sprintf(b, " : "); n += m;
		b += m = D_co(x, b, x->as[i]); n += m; 
	}
	return n;
}

V S_trace(X* x) {
	B buf[255];
	memset(buf, 0, 255);
	D_ctx(x, buf);
	printf("%s\n", buf);
}

V S_parse_name(X* x);

V S_create(X* x) {
  S_parse_name(x);
  C nlen = S_drop(x);
  B* name = (B*)S_drop(x);
  if (nlen) {
    W* w = malloc(sizeof(W) + nlen);
    if (!w) { return; }
    w->p = x->e->l;
    x->e->l = w;
    w->f = 0;
    w->c = 0;
    w->l = nlen;
    strncpy(w->n, name, nlen);
    w->n[nlen] = 0;
    S_push(x, w);
  }
}

V S_compile_quotation(X* x) {
  B* q = (B*)S_drop(x);
  C l = S_block(x, q);
  strncpy(&x->s->m[x->s->h], q, l);
  x->s->m[x->s->h + l] = 0;
  S_push(x, &x->s->m[x->s->h]);
  x->s->h += l + 1;
}

V S_step(X* x) {
  B token = *x->ip;
	S_trace(x);
	switch (token) {
		case '0': S_push(x, 0); break;
		case '1': S_push(x, 1); break;
		case '{': S_parse_block(x); break;
		case '#': S_parse_number(x); break;
		case '\'': S_char(x); break;
	  case 'd': S_dup(x); break;	
		case '_': S_drop(x); break;
		case 's': S_swap(x); break;
		case 'o': S_over(x); break;
		case '(': S_to_R(x); break;
		case ')': S_from_R(x); break;
		case 'j': S_jump(x); break;
		case 'x': S_call(x); break;
		case 0: case 10: case '}': case ']': S_return(x); break;
		case '=': S_eq(x); break;
		case '.': S_fetch(x); break;
		case ',': S_store(x); break;
		case '+': S_add(x); break;
		case '-': S_sub(x); break;
		case '*': S_mul(x); break;
		case '%': S_divmod(x); break;
		case '&': S_and(x); break;
		case '|': S_or(x); break;
		case '^': S_xor(x); break;
		case '!': S_not(x); break;
		case '~': S_invert(x); break;
		case 'l': S_shl(x); break;
		case 'r': S_shr(x); break;
		case 't': S_times(x); break;
    case 'c': S_create(x); break;
    case 'q': S_compile_quotation(x); break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
		case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			EXT(x, token)(x);
			break;
		default:
			if (token > 127) { S_lit(x); }
			else { }
			break;
	}
}

V S_inner(X* x) { C rp = x->rp; while(!x->err && x->rp >= rp && x->ip) { S_step(x); x->ip += 1; } }

V S_eval(X* x, B* s) { S_push(x, s); S_call(x); S_inner(x); }

#define L2(x, t1, v1, t2, v2) t1 v1 = (t1)S_drop(x); t2 v2 = (t2)S_drop(x)

#define TOKEN(cond) (x->ibuf && *x->ibuf && cond)
V S_spaces(X* x) { while (TOKEN(isspace(*x->ibuf))) { x->ibuf++; } }
V S_non_spaces(X* x) { while (TOKEN(!isspace(*x->ibuf))) { x->ibuf++; } }
V S_parse_name(X* x) { S_spaces(x); P1(x, x->ibuf); S_non_spaces(x); P1(x, (x->ibuf - ((B*)T(x)))); }

#define EQ_STR(w, n_, l_) (w->l == l_ && !strncmp(w->n, n_, l_))
V S_find_name(X* x) { L2(x, C, l, B*, n); W* w; for (w = x->e->l; w && !(EQ_STR(w, n, l)); w = w->p) {} P3(x, n, l, w); }

V S_asm(X* x) { L2(x, C, l, B*, c); B t = c[l]; c[l] = 0; S_eval(x, c + 1); c[l] = t; }
V S_num(X* x) { L2(x, C, _, B*, s); B* e; C n = strtol(s, &e, 10); (!n && s == e) ? x->err = NNF : S_push(x, n); }

V S_interpret(X* x) {
  W* w = (W*)S_drop(x);
  printf("Interpreting %.*s\n", w->l, w->n);
}
               
V S_evaluate(X* x, B* s) {
	x->ibuf = s;
	while (TOKEN(1)) {
    S_parse_name(x);
		if (T(x) == 0) { S_drop(x); S_drop(x); break; }
    S_find_name(x);
    if (T(x)) {
      S_interpret(x);
    } else {
      S_drop(x);
      if (T(x) && *((B*)N(x)) == '\\') {
        S_spaces(x);
        S_asm(x);
      } else {
        S_num(x);
      }
    }
	}
}

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(sizeof(S));
  x->e = malloc(sizeof(E));
	return x;
}
*/

#endif
