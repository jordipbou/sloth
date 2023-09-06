#ifndef SLOTH_SYMBOLS_H
#define SLOTH_SYMBOLS_H

#include "sloth.h"

#define S_ENV 255
#define S_HIDDEN 2
#define S_DUAL 1

typedef struct _Symbol {
  B* i; 
  B* c;
  C f;
  struct _Symbol* p;
  C nl;
  B n[sizeof(C)];
} S;

#define HIDDEN(l) (l->f & S_HIDDEN)
#define DUAL(l) (l->f & S_DUAL)

typedef struct _Env { 
  struct _Env* p;
  S* l;
} E;

E* XS_newE(E* p) {
  E* e = malloc(sizeof(E));
  if (e) {
    e->p = p;
    e->l = 0;
  }
  return e;
}

S* XS_newS(E* e, B* n, C nl) {
  S* s = malloc(sizeof(S) + nl - sizeof(C) + 1);
  if (s) {
    s->p = e->l;
    e->l = s;
    s->f = 0;
    s->i = 0;
    s->c = 0;
    s->nl = nl;
    strncpy(s->n, n, nl);
    s->n[nl] = 0;
  }
  return s;
}

C XS_same(S* s, B* n, C nl) {
  return
    !HIDDEN(s) &&
    s->nl == nl &&
    !strncmp(s->n, n, nl);
}

S* XS_find(E* e, B* n, C nl) {
  while (e) {
    S* s = e->l;
    while (s) {
      if (XS_same(s, n, nl)) {
        return s;
      } else {
        s = s->p;
      }
    }
    e = e->p;
  }
  return 0;
}

void XS_ext(X* x) {
  switch (S_token(x)) {
    case 'c': SS_create(x); break;
    case 'f': SS_find(x); break;
  }
}

V XS_install(X* x) {
  EXT(x, 'S') = &XS_ext;  
  ST(x, 'S') = XS_newE(0);
}

#endif