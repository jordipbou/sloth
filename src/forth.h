#ifndef FORTH_SLOTH
#define FORTH_SLOTH

#define FORTH_DICT_SIZE 65536
#define FORTH_TIB_SIZE 1024

#define FORTH_HERE(x) (((C*)x->b)[0])
#define FORTH_LATEST(x) (((C*)x->b)[1])
#define FORTH_STATE(x) (((C*)x->b)[2])
#define FORTH_IN(x) (((C*)x->b)[3])
#define FORTH_TIB(x) (x->b + 4*sizeof(C))

void FORTH_compile_cell(X* x, C v) {
	*((C*)(x->b + FORTH_HERE(x))) = v;
	FORTH_HERE(x) += sizeof(C);
}

void FORTH_compile_byte(X* x, B v) {
	*(x->b + FORTH_HERE(x)) = v;
	FORTH_HERE(x) += 1;
}

void FORTH_add_primitive(X* x, B* n, C nl, B* c, C cl) {
	B* here = x->b + FORTH_HERE(x);	
	C i;
	FORTH_compile_cell(x, FORTH_LATEST(x));
	FORTH_LATEST(x) = here;	
	FORTH_compile_byte(x, 0);
	FORTH_compile_byte(x, (B)nl);
	for (i = 0; i < nl; i++) {
		FORTH_compile_byte(x, n[i]);
	}
	for (i = 0; i < cl; i++) {
		FORTH_compile_byte(x, c[i]);
	}
	FORTH_compile_byte(x, ']');
}

void FORTH_init(X* x) {
	x->b = malloc(FORTH_DICT_SIZE);
	FORTH_HERE(x) = 4*sizeof(C) + FORTH_TIB_SIZE;
	FORTH_LATEST(x) = 0;
	FORTH_IN(x) = 0;

	FORTH_add_primitive(x, "DROP", 4, "d", 1);
	FORTH_add_primitive(x, "SWAP", 4, "s", 1);

	FORTH_add_primitive(x, ">IN", 3, "b.cc+b.+.+", 10);
	FORTH_add_primitive(x, "HERE", 4, "b.b..+", 6);
}

#define NFA(w) (w + sizeof(C) + 1 + 1 + *((B*)w + sizeof(C) + 1))

void FORTH_find_word(X* x) {
  C l = 0;
  C i;
  B* n;
  /* Parse word */
  for (i = FORTH_IN(x); i < FORTH_TIB_SIZE && *(FORTH_TIB(x) + i) != 0 && isspace(*(FORTH_TIB(x) + i)); i++) {}
  for (l = 0; (l + i) < FORTH_TIB_SIZE && *(FORTH_TIB(x) + i + l) != 0 && !isspace(*(FORTH_TIB(x) + i + l)); l++) {} 
  n = FORTH_TIB(x) + i; 
  B* w = FORTH_LATEST(x);
  while (w != 0) {
    if (!strncmp(NFA(w), n, l)) {
      S_lit(x, (C)w);
      S_lit(x, 1);
    } else {
      w = *w;
    }
  }

  S_lit(x, (C)n);
  S_lit(x, l);
  S_lit(x, 0);
}

void FORTH_reset(X* x) {
  x->sp = 0;
  x->rp = 0;
  x->yp = STACK_SIZE;
  x->err = 0;
  x->ip = 0;
}

void FORTH_quit(X* x) {
  B buf[255];
  C e;
  do {
    /* FORTH_reset(x); */
    FORTH_IN(x) = 0;
    S_lit(x, FORTH_TIB(x));
    S_lit(x, FORTH_TIB_SIZE);
    S_accept(x);
  	do {
  		/* Interpret or compile as needed */
      FORTH_find_word(x);
      e = S_drop(x);
      if (e) {
        /* Execute or compile */
        S_call(x); 
      } else {
         /* Convert to number */ 
      }
      memset(buf, 0, 255);
      S_dump_S(buf, x);
      printf("Ok %s\n", buf);
  	} while(1);
  } while(1);
}

#endif
