#ifndef SLOTH
#define SLOTH

#include"vm.h"

#define IMMEDIATE 1
#define HIDDEN 2
#define VARIABLE 4
#define CONSTANT 8
#define ENVIRONMENT 16

typedef struct _Word {
  struct _Word* previous;
	C flags;
  B* code; 
  C nlen;
	B name[1];
} W;

typedef struct _Environment {
	struct _Environment* parent;
	W* latest;
} E;

#define BLOCK_SIZE 65536

typedef struct _System {
	B* ibuf;
	E* environment;
	C state;
	C here;
	B block[BLOCK_SIZE];
} S;

#define TOK(cond) (s->ibuf && *s->ibuf && cond)
V S_spaces(X* x) { S* s = ST(x, 'S'); while (TOK(isspace(*s->ibuf))) { s->ibuf++; } }
V S_non_spaces(X* x) { S* s = ST(x, 'S'); while (TOK(!isspace(*s->ibuf))) { s->ibuf++; } }

V S_parse_name(X* x) { 
	S* s = ST(x, 'S');
	S_spaces(x); 
	S_lit(x, s->ibuf); 
	S_non_spaces(x); 
	S_lit(x, (s->ibuf - ((B*)TS(x)))); 
}

V S_find_name(X* x) {
	S* s = ST(x, 'S');
	C nlen = TS(x);
  B* name = (B*)NS(x);
	W* w = s->environment->latest;
	while (w) {
	  if (w->nlen == nlen && !strncmp(w->name, name, nlen)) break;
		w = w->previous;
	}
	S_lit(x, w);
}

V S_bcompile(X* x) {
	S* s = ST(x, 'S');
	s->block[s->here] = (B)S_drop(x);
	s->here++;
}

V S_ccompile(X* x) { 
	S* s = ST(x, 'S'); 
	*((C*)(&s->block[s->here])) = S_drop(x); 
	s->here += sizeof(C); 
}

V S_literal(X* x) {	S* s = ST(x, 'S'); S_lit(x, '#'); S_bcompile(x); S_ccompile(x); }

V S_compile(X* x) {
  S* s = ST(x, 'S');
	W* w = (W*)S_drop(x);
  S_lit(x, w->code);
  S_literal(x);
	s->block[s->here] = '$';
	s->here++;
}

V S_evaluate(X* x, B* str) {
  S* s = ST(x, 'S');
	B* p;
	W* w;
	C n;
	s->ibuf = str;
  while (s->ibuf && *s->ibuf) {
		S_parse_space(x);
		if (s->ibuf && *s->ibuf) {
			if (*s->ibuf == '\\') {
				s->ibuf++;
				if (!s->state) {
				  S_eval(x, s->ibuf);
				  S_parse_non_space(x);
				} else {
				  while (s->ibuf && *s->ibuf && !isspace(*s->ibuf)) {
				  	s->block[s->here] = *s->ibuf;
				  	s->here++;
				  	s->ibuf++;
          }
				}
			} else {
				S_parse_name(x);
				S_find_name(x);
				w = (W*)S_drop(x);
				n = S_drop(x);
				p = (B*)S_drop(x);
				if (w) {
					if (!s->state || w->flags & IMMEDIATE) {
            if (w->flags & VARIABLE) {
              S_lit(x, &w->code);
            } else if (w->flags & CONSTANT) {
              S_lit(x, w->code);
            } else {
              S_eval(x, w->code);
            }
          } else {
            if (w->flags & VARIABLE) {
              S_lit(x, &w->code);
              S_literal(x);
            } else if (w->flags & CONSTANT) {
              S_lit(x, w->code);
              S_literal(x);
            } else {
              S_lit(x, w);
              S_compile(x);
            }
          }
				} else {
					n = strtol(p, (B**)(&w), 10);
					if (n == 0 && (B*)w == p) {
					} else {
						S_lit(x, n);
            if (s->state) S_literal(x); 
					}
				}
			}
		}
		S_trace(x);
	}
}

V S_create_word(X* x, B* n, C l, B* c, C f) {
	S* s = ST(x, 'S');
  W* w = malloc(sizeof(W) + l);
  w->previous = s->environment->latest;
  s->environment->latest = w;
  w->code = c;
  w->flags = f;
  w->nlen = l;
  strcpy(w->name, n);
}

V S_primitive(X* x, B* n, B* c, C f) { S_create_word(x, n, strlen(n), c, f); }

V S_header(X* x) {
	S* s = ST(x, 'S');
	B* name;
	C nlen;
	S_parse_name(x);
	nlen = S_drop(x);
	name = (B*)S_drop(x);
	S_create_word(x, name, nlen, &s->block[s->here], 0);
}

V S_sloth_ext(X* x) {
	S* s = ST(x, 'S');
	switch (S_token(x)) {
		case 'b': S_bcompile(x); break;
		case 'h': S_header(x); break;
		case ']': s->state = 1; break;
		case '[': s->state = 0; break;
	}
}

X* S_sloth() {
  X* x = S_init();
	S* s = malloc(sizeof(S));
	EXT(x, 'S') = &S_sloth_ext;
	ST(x, 'S') = s;
	s->environment = malloc(sizeof(E));
  
  S_primitive(x, ":", "ShS]", 0);
  S_primitive(x, ";", "0SbS[", IMMEDIATE);

	return x;
}

#endif
