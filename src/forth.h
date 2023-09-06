#ifndef SLOTH_FORTH
#define SLOTH_FORTH

#include"sloth.h"

#define SF_INPUT_BUFFER_SIZE 255
#define SF_DEFAULT_DICT_SIZE 65536

typedef struct _Word {
  struct _Word* previous;
  C flags;
  B* i;
  B* c;
  C nlen;
  B name[sizeof(C)];
} W;

typedef struct _Forth {
  C state;
  B ibuf[SF_INPUT_BUFFER_SIZE];
  C in;
  W* latest;
  C here;
  B dict[SF_DEFAULT_DICT_SIZE];
} F;

V SF_find_name(X* x, F* f) {
  LOCALS2(x, C, l, B*, s);
  W* w = f->latest;
  while (w) {
    if (w->nlen == l && !strncmp(w->name, s, l)) {
      break;  
    } else {
      w = w->previous;
    }
  }
  S_lit(x, (C)s);
  S_lit(x, l);
  S_lit(x, w);
}

V SF_parse_name(X* x, F* f) {
  while (f->in < SF_INPUT_BUFFER_SIZE && 
         f->ibuf[f->in] != 0 &&
         isspace(f->ibuf[f->in])) {
    f->in++;
  }
  S_lit(x, &f->ibuf[f->in]);
  while (f->in < SF_INPUT_BUFFER_SIZE &&
         f->ibuf[f->in] != 0 &&
         !isspace(f->ibuf[f->in])) {
    f->in++;
  }
  S_lit(x, &f->ibuf[f->in] - TS(x));
}
  
V SF_add_primitive(F* f, B* n, B* i, B* c) {
  W* w = (W*)(&f->dict[f->here]);
  C l = strlen(n);
  f->here += sizeof(W) + l - sizeof(C) + 1;
  w->previous = f->latest;
  f->latest = w;
  w->flags = 0;
  w->i = i;
  w->c = c;
  w->nlen = l;
  strncpy(w->name, n, l);
  w->name[l] = 0;
}

V SF_refill(X* x) {
  F* f = ST(x, 'F');
  B* s = fgets(f->ibuf, SF_INPUT_BUFFER_SIZE, stdin);
  S_lit(x, s != 0);
  f->in = 0;
}


V SF_type(X* x) {
  LOCALS2(x, C, l, B*, s);
  printf("%.*s", l, s);
}

V SF_ext(X* x, void* s) {
  F* f = (F*)s;
  switch (S_token(x)) {
    case 'c': f->state = 1; break; 
    case 'i': f->state = 0; break;
  }
}

X* SF_init() {
  X* x = S_init();
  F* f = malloc(sizeof(F));
  EXT(x, 'F') = &SF_ext; 
  ST(x, 'F') = f;
  SF_add_primitive(f, "drop", "_", 0);
  SF_add_primitive(f, "swap", "s", 0);
  SF_add_primitive(f, "dup", "d", 0);
  SF_add_primitive(f, "over", "o", 0);
  SF_add_primitive(f, "rot", "r", 0);
  SF_add_primitive(f, "r>", "(", 0);
  SF_add_primitive(f, ">r", ")", 0);
  
  SF_add_primitive(f, "+", "+", 0);
  SF_add_primitive(f, "-", "-", 0);
  SF_add_primitive(f, "*", "*", 0);
  SF_add_primitive(f, "/", "/", 0);
  SF_add_primitive(f, "mod", "%", 0);

  SF_add_primitive(f, "bye", "q", 0);

  SF_add_primitive(f, "]", 0, "Fi");
  SF_add_primitive(f, "[", "Fc", 0);
  
  SF_add_primitive(f, "1", "1", 0);
  return x;
}

V SF_repl(X* x) {
  F* f = ST(x, 'F');
  W* w;
  C nlen;
  B* name;
  char* t;
  C n;
  SF_refill(x);
  if (S_drop(x)) {
    do {
      SF_parse_name(x, f);
      if (TS(x) == 0) {
        S_drop(x);
        S_drop(x);
        break;
      } else {
        SF_find_name(x, f);
        w = (W*)S_drop(x);
        nlen = S_drop(x);
        name = (B*)S_drop(x);      
        if (w) {
          if (w->c) {
            S_eval(x, w->c); 
          } else if (!f->state) {
            S_eval(x, w->i); 
          } else {
            /* Compile */
            printf("I don't know how to compile!\n");
          }
        } else {
          n = strtol(name, &t, 0);
          if (n != 0 && name != t) {
            S_lit(x, n);
          } else {
            /* ERROR ! */ 
          }
        }
      }
    } while(1);
  }
}

#endif
