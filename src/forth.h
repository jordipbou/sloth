/* TODO: Word flags (immediate/hidden) and use it */
/* TODO: Forth outer interpreter */
/* TODO: What about adding b0 b1 b2 as fixed addresses on the 
   dictionary */

#ifndef FORTH_SLOTH
#define FORTH_SLOTH

#include"sloth.h"

#define FORTH_DICT_SIZE 65536
#define FORTH_TIB_SIZE 255

/* Minimum required variables to implement a Forth will be 
   address-hardcoded in dictionary */

/* SLOTH VERSION: HERE:[b.b..+] HERE is modified thru allot , C, */
#define FORTH_HERE(x) (((C*)x->b)[0])
/* SLOTH VERSION: GET LATEST [b.c+.] SET LATEST [b.c+,] */
#define FORTH_LATEST(x) (((C*)x->b)[1])
/* SLOTH VERSION; GET STATE [b.cc++.] SET STATE [b.cc++,] */
#define FORTH_STATE(x) (((C*)x->b)[2])
/* SLOTH VERSION: GET IN [b.3c*+.] SET IN [b.3c*+,] */
#define FORTH_IN(x) (((C*)x->b)[3])
/* SLOTH VERSION: TIB ADDRESS [b.4c*+] */
#define FORTH_TIB(x) (x->b + 4*sizeof(C))

/* SLOTH VERSION: NAME ADDRESS [c2++] PUSH NAME STRING [c2++d1-:] */
#define NFA(w) (w + sizeof(C) + 1 + 1)
/* SLOTH VERSION: [dc1++;+c2+] */
#define CFA(w) (w + sizeof(C) + 1 + 1 + *((B*)w + sizeof(C) + 1))

/* SLOTH VERSION: [65536mb,c4*255+b.,] */
void FORTH_init(X* x) {
  x->b = malloc(FORTH_DICT_SIZE);
  FORTH_HERE(x) = 4*sizeof(C) + FORTH_TIB_SIZE;
  FORTH_LATEST(x) = 0;
  FORTH_STATE(x) = 0;
  FORTH_IN(x) = 0;
  memset(FORTH_TIB(x), 0, FORTH_TIB_SIZE);
}

/* SLOTH VERSION: [b.b..+,b..c+b.,] */
void FORTH_compile_cell(X* x) {
  S_lit(x, (C)(x->b + FORTH_HERE(x)));
  S_cstore(x);
  FORTH_HERE(x) += sizeof(C);
}

/* SLOTH VERSION: [b.b..+;b..1+b.,] */
void FORTH_compile_byte(X* x) {
  S_lit(x, (C)(x->b + FORTH_HERE(x)));
  S_bstore(x);
  FORTH_HERE(x) += sizeof(B);
}

/* SLOTH VERSION; [ TODO ] */
void FORTH_header(X* x) {
  C i, l = S_drop(x);
  B* n = (B*)S_drop(x);
  C h = FORTH_HERE(x);
  S_lit(x, FORTH_LATEST(x));
  FORTH_compile_cell(x);
  FORTH_LATEST(x) = x->b + h;
  S_lit(x, 0);
  FORTH_compile_byte(x);
  S_lit(x, l);
  FORTH_compile_byte(x);
  /* TODO: CMOVE would be a good helper? */
  for (i = 0; i < l; i++) {
    S_lit(x, n[i]);
    FORTH_compile_byte(x);
  }
}

void FORTH_find(X* x) {
  C l = S_drop(x);
  B* n = (B*)S_drop(x);
  B* w = FORTH_LATEST(x);
  while (w != 0) {
    if (!strncmp(NFA(w), n, l)) {
      S_lit(x, (C)w);
      return;
    } else {
      w = *w;
    }
  }
  S_lit(x, (C)n);
  S_lit(x, l);
  S_lit(x, 0);
}

void FORTH_compile_quotation(X* x) {
  C t = 1;
  B* q = (B*)S_drop(x);
  while (t) {
    if (*q == '[') t++;
    if (*q == ']') t--;
    S_lit(x, (C)*q);
    FORTH_compile_byte(x);
    q++;
  }
}

void FORTH_execute(X* x) {
  B* w = (B*)S_drop(x);
  S_lit(x, CFA(w));
  S_call(x);
}

void FORTH_extension(X* x) {
  switch (S_token(x)) {
  case 'i': FORTH_init(x); break;
  case 'h': S_lit(x, &(FORTH_HERE(x))); break;
  case 'l': S_lit(x, &(FORTH_LATEST(x))); break;
  case ',': FORTH_compile_cell(x); break;
  case ';': FORTH_compile_byte(x); break;
  case ':': FORTH_header(x); break;
  case 'f': FORTH_find(x); break;
  case 'q': FORTH_compile_quotation(x); break;
  case 'e': FORTH_execute(x); break;
  }
}

#endif
