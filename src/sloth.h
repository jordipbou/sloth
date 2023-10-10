#ifndef SLOTH_OUTER
#define SLOTH_OUTER

#include "vm.h"

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
  I vp;
  I* v;
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
  parse_name(x); 
  header(x); 
  D(x)->s = 1; 
}

V semicolon(X* x) { 
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

V dump_repl_context(X* x) {
  printf("[%d] ", D(x)->s);
  dump_context(x);
  printf("<%s>\n", D(x)->i);
}

V evaluate(X* x, B* s) {
	I l;
	B* t;
  I i;
  W* w;
	IBUF(x) = s;
	while (IBUF(x) && *IBUF(x) && *IBUF(x) != 10) {
    dump_repl_context(x);
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
			if (l == 1 && t[0] == ':') {
        DDROP(x); DDROP(x);
        create(x);
      }
      else if (l == 1 && t[0] == ';') {
        DDROP(x); DDROP(x);
        semicolon(x);
      }
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

B* forth = 
": drop \\_ ; "
": dup \\d ; "
": swap \\s ; "
": over \\o ; "
": rot \\r ; "
": + \\+ ; "
": - \\- ; "
": * \\* ; "
": / \\/ ; "
": mod \\% ; "
": < \\< ; "
": = \\= ; "
": > \\> ; ";

V create_variable(X* x) {
  printf("pre-create\n");
  dump_context(x);
  printf("%ld\n", x->dp);
  create(x);
  printf("post-create\n");
  dump_context(x);
  printf("%ld\n", x->dp);
  cnum(x, D(x)->vp);
  C(x, B, 'S');
  C(x, B, 'v');
  printf("post-compile var\n");
  dump_context(x);
  printf("%ld\n", x->dp);
  (D(x)->vp)++;
  semicolon(x);
  printf("post-semicolon\n");
  dump_context(x);
  printf("%ld\n", x->dp);
  printf("Name of latest: %s\n", ((W*)TO_PTR(x, D(x)->l))->n);
  printf("Compiled code of latest: %s\n", TO_PTR(x, ((W*)TO_PTR(x, D(x)->l))->c));
}

V ext_SLOTH(X* x) {
  switch (TOKEN(x)) {
  case 'c': create_variable(x); break;
  case 'v': DPUSH(x, &(D(x)->v[DPOP(x)])); break;
  }
}

X* init_ANS_FORTH() {
  X* x = init_EXT(init_VM());
  x->d = malloc(DICT_SIZE);
  D(x)->l = 0;
  HERE(x) += sizeof(DICT);
  D(x)->vp = 0;
  D(x)->v = malloc(64*1024*sizeof(I));

  EXT(x, 'S') = &ext_SLOTH;

  /* evaluate(x, forth); */

  return x;
}

#endif