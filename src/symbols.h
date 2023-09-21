#ifndef SLOTH_SYMBOLS_H
#define SLOTH_SYMBOLS_H

#include "vm.h"

#define IMMEDIATE 1
#define HIDDEN 2
#define VARIABLE 4
#define CONSTANT 8

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

V S_create(X* x) {
  E* e = ST(x, 'E');
	C nlen = S_drop(x);
	B* name = (B*)S_drop(x);
	W* w = malloc(sizeof(W) + nlen);
	if (w) {
		w->nlen = nlen;
		strncpy(w->name, name, nlen);
		w->name[nlen] = 0;
		w->previous = e->latest;
		e->latest = w;
	}
	S_lit(x, w);
}

V S_find_name(X* x) {
	E* e = ST(x, 'E');
	C nlen = TS(x);
  B* name = (B*)NS(x);
	W* w = e->latest;
	while (w) {
	  if (w->nlen == nlen && !strncmp(w->name, name, nlen)) break;
		w = w->previous;
	}
	S_lit(x, w);
}

V S_parse_symbol(X* x) {
	S_lit(x, x->ip);
	while (S_peek(x) && !isspace(S_token(x))) {}
	S_lit(x, (x->ip - (B*)TS(x)) - 1);
}

V S_symbol(X* x) {
  W* w;
	C nlen;
	B* name;
	S_parse_symbol(x);
	S_find_name(x);
	if (TS(x)) {
		S_nip(x); S_nip(x);
		w = (W*)S_drop(x);
		if (w->flags & CONSTANT) S_lit(x, w->code);
		else if (w->flags & VARIABLE) S_lit(x, &w->code);
		else S_eval(x, w->code);
	} else {
		S_drop(x);
		S_create(x);
	}
}

V S_latest(X* x) { E* e = ST(x, 'E'); S_lit(x, e->latest); }
V S_variable(X* x) { W* w = (W*)S_drop(x); w->flags |= VARIABLE; }
V S_constant(X* x) { W* w = (W*)S_drop(x); w->flags |= CONSTANT; }
V S_hidden(X* x) { W* w = (W*)S_drop(x); w->flags |= HIDDEN; }
V S_reveal(X* x) { W* w = (W*)S_drop(x); w->flags &= ~HIDDEN; }

V S_env_ext(X* x) {
	switch (S_token(x)) {
	case 'c': S_constant(x); break;
	case 'h': S_hidden(x); break;
	case 'l': S_latest(x); break;
	case 'r': S_reveal(x); break;
	case 'v': S_variable(x); break;
	case 'x': S_create(x); break;
	case '\'': S_symbol(x); break;
	}
}

X* S_env_init(X* x) {
	E* e = malloc(sizeof(E));
	e->parent = 0;
	e->latest = 0;
	EXT(x, 'E') = &S_env_ext;
	ST(x, 'E') = e;
	return x;
}

#endif