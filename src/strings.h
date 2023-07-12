#ifndef SLOTH_STRINGS_H
#define SLOTH_STRINGS_H

#include "sloth.h"

void SS_accept(X* x) { 
	C i = 0;
  C l1 = S_drop(x);
  B* s1 = (B*)S_drop(x);
	do { 
		x->key(x); 
		if (TS(x) == 10) {
			S_drop(x);
			break;
		} else if (TS(x) == 127) {
      S_drop(x);
      if (i > 0) {
        S_lit(x, '\b');
        x->emit(x);
        S_lit(x, ' ');
        x->emit(x);
        S_lit(x, '\b');
        x->emit(x);
        i--;
      }
    } else {
			s1[i++] = TS(x);
			x->emit(x);
		}
	} while(i < l1);
	S_lit(x, i);
}

void SS_compare(X* x) { 
  C l1 = S_drop(x);
  B* s1 = (B*)S_drop(x);
  C l2 = S_drop(x);
  B* s2 = (B*)S_drop(x);
  S_lit(x, l2 == l1 && !strncmp(s2, s1, l2)); 
}

void SS_fill(X* x) {
  B c = (B)S_drop(x);
  C n = S_drop(x);
  B* s = (B*)S_drop(x);
  memset(s, c, n);
}

void SS_type(X* x) {
	C i = 0;
	C n = S_drop(x);
	B* s = (B*)S_drop(x);
	while (n-- > 0) { S_lit(x, s[i++]); x->emit(x); }
}

void SS_ext(X* x) {
  switch(S_token(x)) {
    case 'a': SS_accept(x); break;
    case 'c': SS_compare(x); break;
    case 'f': SS_fill(x); break;
    case 't': SS_type(x); break;
  }
}

#endif