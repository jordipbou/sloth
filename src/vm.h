#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

#define V void /* inline void ? */

typedef char B;
typedef int16_t B2;
typedef intptr_t C;

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Word { struct _Word* p; C f; C c; C l;	B n[1]; } W;
typedef struct _Environment {	struct _Environment* p;	W* l; } E;

#define HEAP_SIZE 64*1024

typedef struct _System {
  B* ibuf;
  C hp;
  B h[HEAP_SIZE];
	E* e;
	C st;
} S;

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context {
  C ds[STACK_SIZE]; C dp; C cp;
  B* rs[RSTACK_SIZE]; C rp;
	B* ip;
	S* s;
} X;

#define S_pu(x, v) (x->ds[x->dp++] = (C)(v))
#define S_po(x) (x->ds[--x->dp])

#define S_tu(x, v) (x->ds[--x->cp] = (C)(v))
#define S_to(x) (x->ds[x->cp++])

#define P1(x, u) S_pu(x, u)
#define P2(x, u, v) S_pu(x, u); S_pu(x, v)
#define P3(x, u, v, w) S_pu(x, u); S_pu(x, v); S_pu(x, w)

#define L1(x, t, v) t v = (t)S_po(x)
#define L2(x, t1, v1, t2, v2) t1 v1 = (t1)S_po(x); t2 v2 = (t2)S_po(x)
#define L3(x, t1, v1, t2, v2, t3, v3) t1 v1 = (t1)S_po(x); t2 v2 = (t2)S_po(x); t3 v3 = (t3)S_po(x)

#define TS(x) (x->ds[x->dp - 1])
#define NS(x) (x->ds[x->dp - 2])
#define NNS(x) (x->ds[x->dp - 3])

V S_si(X* x, C i) { x->rs[x->rp++] = x->ip + i; }
#define TC(a) ((a) != 0 && *(a) != 0 && *(a) != 10 && *(a) != '}')
V S_ca(X* x, C i) { L1(x, B*, q); if (TC(x->ip + i)) S_si(x, i); x->ip = q; }
V S_ex(X* x) { if (x->rp == 0) x->ip = 0; else x->ip = x->rs[--x->rp]; }
V S_fj(X* x) { B2 d; x->ip++; S_pu(x, x->ip + 2); d = *((B2*)(x->ip)); x->ip += d; }
                   
V S_dr(X* x) { S_po(x); }
V S_sw(X* x) { C t = TS(x); TS(x) = NS(x); NS(x) = t; }
V S_ov(X* x) { S_pu(x, NS(x)); }
V S_du(X* x) { S_pu(x, TS(x)); }
V S_ro(X* x) { C t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; }

V S_ad(X* x) { NS(x) = NS(x) + TS(x); x->dp--; }
V S_su(X* x) { NS(x) = NS(x) - TS(x); x->dp--; }
V S_mu(X* x) { NS(x) = NS(x) * TS(x); x->dp--; }
V S_di(X* x) { NS(x) = NS(x) / TS(x); x->dp--; }
V S_mo(X* x) { NS(x) = NS(x) % TS(x); x->dp--; }

V S_an(X* x) { NS(x) = NS(x) & TS(x); x->dp--; }
V S_or(X* x) { NS(x) = NS(x) | TS(x); x->dp--; }
V S_xo(X* x) { NS(x) = NS(x) ^ TS(x); x->dp--; }
V S_no(X* x) { TS(x) = !TS(x); }
V S_iv(X* x) { TS(x) = ~TS(x); }

V S_lt(X* x) { NS(x) = NS(x) < TS(x); x->dp--; }
V S_eq(X* x) { NS(x) = NS(x) == TS(x); x->dp--; }
V S_gt(X* x) { NS(x) = NS(x) > TS(x); x->dp--; }

V S_ev(X* x, B* q);
V S_ti(X* x) { L2(x, B*, q, C, n); for(;n > 0; n--) { S_ev(x, q); } }
V S_br(X* x) { L3(x, B*, f, B*, t, C, b); if (b) S_ev(x, t); else S_ev(x, f); }

#define S_bl(a, b) \
  { \
    C t = 1; \
    while (t) { \
      if ((a) == 0 || *(a) == 0 || *(a) == 10) break; \
      if (*(a) == '{') t++; \
      if (*(a) == '}') t--; \
      b; \
      (a)++; \
    } \
  }

V S_pq(X* x) { x->ip = x->ip + 1; S_pu(x, x->ip); S_bl(x->ip, {}); }
 
#define S_pr(s, n, f, a) { C t; s += t = sprintf(s, f, a); n += t; }
              
/* C S_co(X* x, B* c) { S_bl(c, printf("%c", *c)); } */
V S_co(X* x, B* c) {
  C t = 1;
  while (t) {
    if (c == 0 || *c == 0 || *c == 10) break;
    if (*c == '{') t++;
    if (*c == '}') t--;
    if (*c == '#') { printf("#%d", *((B2*)(c+1))); c += 3; }
    else {
      printf("%c", *c);
      c++;
    }
  }
}
V S_tr(X* x) {
  C i;
  B* t;
  for (i = 0; i < x->dp; i++) printf("%ld ", x->ds[i]);
  printf("| ");
  S_co(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" : ");
    S_co(x, x->rs[i]); 
  }
  printf("\n");
}
              
V S_st(X* x) {
  S_tr(x);
  switch (*x->ip) {
    case '[': S_fj(x); return;
    case '{': S_pq(x); return;
    case 0: 
    case 10: 
    case ']':
    case '}': S_ex(x); return;
    case 'e': S_ca(x, 1); return;
    case '0': S_pu(x, 0); break;
    case '1': S_pu(x, 1); break;
    case '#': S_pu(x, *((B2*)(x->ip + 1))); x->ip += 3; return;
    case '$': S_pu(x, x->s->h + *((B2*)(x->ip + 1))); x->ip += 3; return;
    case '_': S_dr(x); break;
    case 's': S_sw(x); break;
    case 'o': S_ov(x); break;
    case 'd': S_du(x); break;
    case 'r': S_ro(x); break;
    case '+': S_ad(x); break;
    case '-': S_su(x); break;
    case '*': S_mu(x); break;
    case '/': S_di(x); break;
    case '%': S_mo(x); break;
    case '&': S_an(x); break;
    case '|': S_or(x); break;
    case '!': S_no(x); break;
    case '^': S_xo(x); break;
    case '~': S_iv(x); break;
    case '<': S_lt(x); break;
    case '=': S_eq(x); break;
    case '>': S_gt(x); break;
    case 't': S_ti(x); break;
    case '?': S_br(x); break;
    case 'w': /* word/s inspection */ break;
    case 'x': /* context reflection */ break;
  }
  x->ip = x->ip + 1;
}
              
V S_in(X* x) { C rp = x->rp; while(x->rp >= rp && x->ip) { S_st(x); } }
V S_ev(X* x, B* q) { S_pu(x, q); S_ca(x, 0); S_in(x); }

X* S_init() { X* x = malloc(sizeof(X)); x->s = malloc(sizeof(S)); x->s->e = malloc(sizeof(E)); return x; }

#define TOKEN(cond) (x->s->ibuf && *x->s->ibuf && cond)
V S_spaces(X* x) { while (TOKEN(isspace(*x->s->ibuf))) { x->s->ibuf++; } }
V S_non_spaces(X* x) { while (TOKEN(!isspace(*x->s->ibuf))) { x->s->ibuf++; } }
V S_parse_name(X* x) { S_spaces(x); P1(x, x->s->ibuf); S_non_spaces(x); P1(x, (x->s->ibuf - ((B*)TS(x)))); }

V S_find_name(X* x) {
	L2(x, C, l, B*, n);
	E* e = x->s->e;
	while (e) {
		W* w = e->l;
		while (w) {
			if (w->l == l && !strncmp(w->n, n, l)) {
				S_pu(x, n);
				S_pu(x, l);
				S_pu(x, w);
        return;
			}
			w = w->p;
		}
		e = e->p;
	}
	S_pu(x, n);
	S_pu(x, l);
	S_pu(x, 0);
}

V S_asm(X* x) { L2(x, C, l, B*, c); B t = c[l]; c[l] = 0; S_ev(x, c + 1); c[l] = t; }
V S_num(X* x) { L2(x, C, _, B*, s); B* e; C n = strtol(s, &e, 10); (!n && s == e) ? 0 : S_pu(x, n); }

B* S_hr(X* x) { return &x->s->h[x->s->hp]; }
V S_bc(X* x, B b) { x->s->h[x->s->hp] = b; x->s->hp += 1; }
V S_lc(X* x, B2 l) { *((B2*)&x->s->h[x->s->hp]) = l; x->s->hp += 2; }
V S_wc(X* x, W* w) { S_bc(x, '$'); S_lc(x, x->s->h + w->c); }
V S_nc(X* x, C n) { S_bc(x, '#'); S_lc(x, n); }

V S_inline(X* x, W* w) { C* c = w->c; while (c) { S_bc(x, *c); c++; } }
V S_compile(X* x) { L3(x, W*, w, C, _, B*, __); if (strlen(w->c) < 3) S_inline(x, w); else S_wc(x, w); }
V S_interpret(X* x) { L3(x, W*, w, C, _, B*, __); S_ev(x, x->s->h + w->c); }

W* S_aword(X* x, C l) { W* w = (W*)S_hr(x); x->s->hp += sizeof(W) + l; return w; }
V S_slatest(X* x, W* w) { w->p = x->s->e->l; x->s->e->l = w; }
V S_sname(W* w, C l, B* n) { w->l = l; strncpy(w->n, n, l); w->n[l] = 0; }
V S_scode(X* x, W* w) { w->c = x->s->hp; }
V S_header(X* x) { L2(x, C, l, B*, n); W* w = S_aword(x, l); S_slatest(x, w); S_sname(w, l, n); S_scode(x, w); }
V S_create(X* x) { L2(x, C, _, B*, __); S_parse_name(x); S_header(x); x->s->st = 1; }

V S_reveal(X* x) { L2(x, C, _, B*, __); x->s->e->l->f &= ~HIDDEN; x->s->st = 0; S_bc(x, 0); }

V S_start_quotation(X* x) { 
	L2(x, C, _, B*, __);
  S_bc(x, '[');
  S_tu(x, x->s->hp);
  S_lc(x, 0);
  if (!x->s->st) S_pu(x, &x->s->h[x->s->hp]);
  x->s->st++;
}

V S_end_quotation(X* x) {
	C i;
	L2(x, C, _, B*, __);
  S_bc(x, ']'); 
  i = S_to(x);
  *((B2*)&x->s->h[i]) = (B2)(0 - (i - x->s->hp));
  x->s->st--;
}

V S_evaluate(X* x, B* s) {
	C l;
	B* t;
  C i;
  W* w;
	x->s->ibuf = s;
	while (x->s->ibuf && *x->s->ibuf && *x->s->ibuf != 10) {
		S_parse_name(x);
		if (!TS(x)) { S_dr(x); S_dr(x); return; }
		S_find_name(x);
		if (TS(x)) {
			if (x->s->st) S_compile(x);
			else S_interpret(x);
		} else {
			S_dr(x);
			l = TS(x);
			t = (B*)NS(x);
			if (l == 1 && t[0] == ':') S_create(x);
      else if (l == 1 && t[0] == ';') S_reveal(x);
      else if (l == 1 && t[0] == '[') S_start_quotation(x);
      else if (l == 1 && t[0] == ']') S_end_quotation(x);
			else if (t[0] == '\\') {
				if (x->s->st) { /* S_compile_asm(x); */
          S_po(x); S_po(x);
          for (i = 1; i < l; i++) {
            S_bc(x, t[i]);
          }
				} else {
					S_spaces(x);
					S_asm(x);
				}
			} else {
				S_num(x);
        if (x->s->st) {
          S_bc(x, '#');
          S_lc(x, S_po(x));
        }
			}
		}
    S_tr(x);
	}
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
  void (*ext[26])(struct _Context*);
  void* st[26];
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
