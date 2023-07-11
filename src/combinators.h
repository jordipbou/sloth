#ifndef SLOTH_COMBINATORS_H
#define SLOTH_COMBINATORS_H

#include"sloth.h"

void SC_branch(X* x) { 
  S_rot(x); 
  if (!S_drop(x)) { S_swap(x); }
  S_drop(x);
  S_call(x);
}

void SC_1bi(X* x) {
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  C n = TS(x);
  S_eval(x, p);
  S_lit(x, n);
  S_eval(x, q);
}

void SC_2bi(X* x) {
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  C m = TS(x);
  C n = NS(x);
  S_eval(x, p);
  S_lit(x, n);
  S_lit(x, m);
  S_eval(x, q);
}

void SC_3bi(X* x) {
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  C o = TS(x);
  C m = NS(x);
  C n = NNS(x);
  S_eval(x, p);
  S_lit(x, n);
  S_lit(x, m);
  S_lit(x, o);
  S_eval(x, q);
}

void SC_1tri(X* x) {
  B* r = (B*)S_drop(x);
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  C n = TS(x);
  S_eval(x, p);
  S_lit(x, n);
  S_eval(x, q);
  S_lit(x, n);
  S_eval(x, r);
}

void SC_2tri(X* x) {
  B* r = (B*)S_drop(x);
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  C m = TS(x);
  C n = NS(x);
  S_eval(x, p);
  S_lit(x, n);
  S_lit(x, m);
  S_eval(x, q);
  S_lit(x, n);
  S_lit(x, m);
  S_eval(x, r);
}

void SC_3tri(X* x) {
  B* r = (B*)S_drop(x);
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  C o = TS(x);
  C m = NS(x);
  C n = NNS(x);
  S_eval(x, p);
  S_lit(x, n);
  S_lit(x, m);
  S_lit(x, o);
  S_eval(x, q);
  S_lit(x, n);
  S_lit(x, m);
  S_lit(x, o);
  S_eval(x, r);
}

void _binrec(X* x, B* c, B* t, B* p, B* q) {
  S_eval(x, c);
  if (S_drop(x)) {
    S_eval(x, t);
  } else {
    S_eval(x, p);
    _binrec(x, c, t, p, q);
    S_swap(x);
    _binrec(x, c, t, p, q);
    S_eval(x, q);
  }
}

void SC_binrec(X* x) {
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  B* t = (B*)S_drop(x);
  B* c = (B*)S_drop(x);
  _binrec(x, c, t, p, q);
}

void SC_dip(X* x) {
  B* p = (B*)S_drop(x);
  C n = S_drop(x);
  S_eval(x, p);
  S_lit(x, n);
}

void _linrec(X* x, B* c, B* t, B* p, B* q) {
  S_eval(x, c);
  if (S_drop(x)) {
    S_eval(x, t);
  } else {
    S_eval(x, p);
    _linrec(x, c, t, p, q);
    S_eval(x, q);
  }
}

void SC_linrec(X* x) {
  B* q = (B*)S_drop(x);
  B* p = (B*)S_drop(x);
  B* t = (B*)S_drop(x);
  B* c = (B*)S_drop(x);
  _linrec(x, c, t, p, q);
}

void SC_sip(X* x) {
  B* p = (B*)S_drop(x);
  C n = TS(x);
  S_eval(x, p);
  S_lit(x, n);
}

void SC_times(X* x) { 
  B* q = (B*)S_drop(x); 
  C n = S_drop(x); 
  while (n-- > 0) { S_eval(x, q); } 
}

void SC_until(X* x) { 
  B* q = (B*)S_drop(x);
  B* c = (B*)S_drop(x);
  do { 
    S_eval(x, c); 
    if (S_drop(x)) break; 
    S_eval(x, q); 
  } while(1);
}

void SC_while(X* x) { 
  B* q = (B*)S_drop(x);
  B* c = (B*)S_drop(x);
  do { 
    S_eval(x, c); 
    if (!S_drop(x)) break; 
    S_eval(x, q); 
  } while(1);
}

void SC_ext(X* x) {
  switch (S_token(x)) {
    case '?': SC_branch(x); break;
    case '1':
      switch (S_token(x)) {
        case 'b': SC_1bi(x); break;
        case 't': SC_1tri(x); break;
      }
    case '2':
      switch (S_token(x)) {
        case 'b': SC_2bi(x); break;
        case 't': SC_2tri(x); break;
      }
    case '3':
      switch (S_token(x)) {
        case 'b': SC_3bi(x); break;
        case 't': SC_3tri(x); break;
      }
    case 'b': SC_binrec(x); break;
    case 'd': SC_dip(x); break;
    /*case 'i': SC_ifte(x); break;*/
    case 'l': SC_linrec(x); break;
    case 's': SC_sip(x); break;
    case 't': SC_times(x); break;
    case 'u': SC_until(x); break;
    case 'w': SC_while(x); break;
  }
}

#endif