#ifndef SLOTH_FORTH
#define SLOTH_FORTH

#include"sloth.h"

#define SF_INPUT_BUFFER_SIZE 255
#define SF_DEFAULT_DICT_SIZE 65536

typedef struct _Symbol {
  struct _Symbol* previous;
  B* name;
  C nlen;
  C flags;
  B* interpretation;
  B* compilation;
} S;

V F_create_symbol(S* prev, B* name, C nlen) {
  B* n = malloc(nlen + 1);
  if (!n) return 0;
  strncpy(n, name, nlen);
  n[nlen] = 0;
  S* s = malloc(sizeof(S));
  if (!s) { free(n); return 0; }
  s->previous = prev;
  s->name = n;
  s->nlen = nlen;
  s->flags = 0;
  s->interpretation = 0;
  s->compilation = 0;
  return s;
}

V F_find_symbol(B* name, C nlen, S* latest) {
  while (latest) {
    if (nlen == latest->nlen && !strncmp(latest->name, name, nlen)) {
      return latest;
    } else {
      latest = latest->previous;
    }
  } 
  return 0;
}

V F_print_quotation(B* q) {
  C t = 1;
  if (q == 0) { printf("[]"); } 
  else {
    while (t > 0) {
      if (*q == ']') t--;
      if (*q == '[') t++;
      printf("%c", *q);
    }
  }
}

V F_print_symbol(S* s) {
  printf("[%p] %.*s\n", s, s->nlen, s->name);
  printf("I:");
  F_print_quotation(s->interpretation);
  printf("\nC:");
  F_print_quotation(s->compilation);
  printf("\n");
}

V F_print_all_symbols(S* latest) {
  if (!latest) {
    printf("Empty symbol table\n");
  } else {
    while (latest) {
      F_print_symbol(latest);
      latest = latest->previous;
    }
  }
}

V SS_ext(X* x, S* l) {
  B* n;
  C nl;
  switch (S_token(x)) {
  case 'a': F_print_all_symbols(l); break;
  case 'c': nl = S_drop(x); n = (B*)S_drop(x); F_create_symbol(l, n, nl); break;
  }
}

X* SS_init() {
  X* x = S_init();
  ST(x, 'S') = 0;
  EXT(x, 'S') = &SS_ext;
  return x;
}

typedef struct _Forth {
  C state;
  B ibuf[SF_INPUT_BUFFER_SIZE];
  C in;
  S* latest;
  C here;
  B dict[SF_DEFAULT_DICT_SIZE];
} F;

/*
W* SF_create_word(F* f, B* n, C l, B* i, B* c) {
  W* w = (W*)(&f->dict[f->here]);
  f->here += sizeof(W) + l - sizeof(C) + 1;
  w->previous = f->latest;
  f->latest = w;
  w->flags = 0;
  w->i = i;
  w->c = c;
  w->nlen = l;
  strncpy(w->name, n, l);
  w->name[l] = 0; 
  return w;
}

W* SF_find_word(F* f, B* n, C l) {
  W* w = f->latest;
  while (w) {
    if (w->nlen == l && !strncmp(w->name, n, l)) {
      break;  
    } else {
      w = w->previous;
    }
  }  
  return w;
}

V SF_find_name(X* x, F* f) {
  LOCALS2(x, C, l, B*, n);
  W* w = SF_find_word(f, n, l);
  S_lit(x, (C)n);
  S_lit(x, l);
  S_lit(x, w);
}

V print_stack(X* x) {
  C i;
 	printf("Ok "); for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } printf("\n");   
}
  
V SF_parse_name(X* x, F* f) {
  printf("Parsing name...\n");
  print_stack(x);
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
  printf("Name parsed: %.*s\n", TS(x), NS(x));
  print_stack(x);
}
  
V SF_create(X* x) {
  B* n;
  C l;
  W* w;
  F* f = ST(x, 'F');
  printf("On SF_create\n");
  print_stack(x);
  SF_parse_name(x, f);
  l = S_drop(x);
  n = (B*)S_drop(x);
  w = SF_create_word(f, n, l, 0, 0);
  w->i = &f->dict[f->here];
  S_lit(x, w);
  printf("Word created\n");
  print_stack(x);
}
  
V SF_add_primitive(F* f, B* n, B* i, B* c) {
  C l = strlen(n);
  SF_create_word(f, n, l, i, c);
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
    case 'h': SF_create(x); break;
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

  SF_add_primitive(f, "create", "Fh", 0);
  SF_add_primitive(f, "]", 0, "Fi");
  SF_add_primitive(f, "[", "Fc", 0);
  SF_add_primitive(f, ":", "FhFc", 0);
  SF_add_primitive(f, ";", 0, "Fi");

  SF_add_primitive(f, "1", "1", 0);
  return x;
}

V SF_compile(X* x) {
  F* f = ST(x, 'F');
  W* w = (W*)S_drop(x);
  f->dict[f->here++] = '#';
  *((C*)&f->dict[f->here]) = (C)w;
  f->here += sizeof(C);
  f->dict[f->here++] = 'i';
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
            S_lit(x, w);
            SF_compile(x);
          }
        } else {
          n = strtol(name, &t, 0);
          if (n != 0 && name != t) {
            S_lit(x, n);
          } else {
          }
        }
      }
    } while(1);
  }
}
*/

#endif
