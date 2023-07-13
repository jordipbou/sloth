#ifndef SLOTH_DICTIONARY_H
#define SLOTH_DICTIONARY_H

#include "sloth.h"

#define SD_DICT_SIZE 65536

#define SD_BLOCK_SIZE(x) (((C*)x->b)[0])
#define SD_HERE(x) (((C*)x->b)[1])
#define SD_LATEST(x) (((C*)x->b)[2])

#define SD_IMMEDIATE 1

/* TODO: Use mask for flags/length? */
#define SD_FLAGS(s) (*((C*)(s + sizeof(C))))
#define SD_NL(s) (*((C*)(s + sizeof(C) + 1)))
#define SD_NFA(s) (s + sizeof(C) + 1 + 1)
#define SD_CFA(s) (SD_NFA(s) + SD_NL(s))

void SD_bootstrap(X* x) {
  x->b = malloc(SD_DICT_SIZE);
  SD_BLOCK_SIZE(x) = SD_DICT_SIZE;
  SD_HERE(x) = 3*sizeof(C);
  SD_LATEST(x) = 0;
}

void SD_create(X* x) {
  C l1 = S_drop(x);
  B* s1 = (B*)S_drop(x);
  B* w = x->b + SD_HERE(x);
	*((B**)w) = (B*)SD_LATEST(x);
	SD_LATEST(x) = (C)w;
	SD_FLAGS(w) = 0;
	SD_NL(w) = l1;
	strncpy(SD_NFA(w), s1, l1);
	S_lit(x, (C)SD_CFA(w));
	SD_HERE(x) += sizeof(B**) + 2 + l1; 
}

void SD_bcompile(X* x) { 
  B v = (B)S_drop(x);
  *(x->b + SD_HERE(x)) = v;
  SD_HERE(x)++;
}

void SD_ccompile(X* x) {
  C v = S_drop(x);
  *((C*)(x->b + SD_HERE(x))) = v;
  SD_HERE(x) += sizeof(C);
}

void SD_qcompile(X* x, C e) { 
  C l = 0, t = 1; 
  B* q = (B*)S_drop(x);
  while (t) {
    if (q[l] == '[') t++;
    if (q[l] == ']') t--;
    l++;
  }
  strncpy(x->b + SD_HERE(x), q, l + e);
  SD_HERE(x) += l + e; 
}

void SD_scompile(X* x) { 
  C l = S_drop(x); 
  B* s = (B*)S_drop(x); 
  *(x->b + SD_HERE(x)) = '"';
  strncpy(x->b + SD_HERE(x) + 1, s, l);
  *(x->b + SD_HERE(x) + 1 + l) = '"';
  SD_HERE(x) += l + 2;
}

void S_allot(X* x) { 
  SD_HERE(x) += S_drop(x); 
}

void SD_parse_name(X* x) {
  C i = S_drop(x);
  B* s = (B*)S_drop(x);
  while (s[i] != 0 && isspace(s[i])) { i++; }
  S_lit(x, (C)(s + i));
  while (s[i] != 0 && !isspace(s[i])) { i++; }
  S_lit(x, (C)(s + i - TS(x)));
	S_lit(x, i);
}

void SD_set_immediate(X* x) {
  SD_FLAGS(SD_LATEST(x)) |= SD_IMMEDIATE;
}

void SD_is_immediate(X* x) {
  B* w = (B*)S_drop(x);
  S_lit(x, (C)(SD_FLAGS(w) & SD_IMMEDIATE));
}

void SD_find(X* x) {
  C l = S_drop(x);
  B* s = (B*)S_drop(x);
  B* w = (B*)SD_LATEST(x);
  while (w) {
    if (SD_NL(w) == l && !strncmp(SD_NFA(w), s, l)) {
      S_lit(x, (C)SD_CFA(w));
      return;
    }
    w = *((B**)w);
  }
  S_lit(x, (C)s);
  S_lit(x, l);
  S_lit(x, 0);
}

void SD_symbol(X* x) {
  /*
  S_lit(x, (C)(x->ip));
  S_lit(x, 0);
  S_parse_name(x);
  S_find(x);
  if (TS(x) == 0) {
    S_drop(x);
    S_create(x);
  }
  */
  C l = 0;
	B* s = x->ip;
	B* w = (B*)SD_LATEST(x);
	while (!isspace(S_token(x))) { l++; }
	while (w) {
		if (SD_NL(w) == l && !strncmp(SD_NFA(w), s, l)) {
			S_lit(x, (C)SD_CFA(w));
			return;
		}
		w = *((B**)w);
	}
  w = x->b + SD_HERE(x);
	*((B**)w) = (B*)SD_LATEST(x);
	SD_LATEST(x) = (C)w;
	SD_FLAGS(w) = 0;
	SD_NL(w) = l;
	strncpy(SD_NFA(w), s, l);
	S_lit(x, (C)SD_CFA(w));
	SD_HERE(x) += sizeof(B**) + 2 + l;
}

void SD_ext(X* x) {
  switch (S_token(x)) {
    case 'a': SD_HERE(x) += S_drop(x); break;
    case 'b': SD_bootstrap(x); break;
    case 'h': S_lit(x, (C)(x->b + SD_HERE(x))); break;
    case 'c': SD_create(x); break;
    case 'f': SD_find(x); break;
    case 'i': SD_set_immediate(x); break;
    case 'I': SD_is_immediate(x); break;
    case ';': SD_bcompile(x); break;
    case ',': SD_ccompile(x); break;
    case 'p': SD_parse_name(x); break;
    case 'q': SD_qcompile(x, -1); break;
    case 'Q': SD_qcompile(x, 0); break;
    case 's': SD_scompile(x); break;
  }
}

#endif