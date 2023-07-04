#ifndef FORTH_SLOTH
#define FORTH_SLOTH

#define FORTH_DICT_SIZE 65536
#define FORTH_TIB_SIZE 1024

#define FORTH_HERE(x) (((C*)x->b)[0])
#define FORTH_LATEST(x) (((C*)x->b)[1])
#define FORTH_IN(x) (((C*)x->b)[2])
#define FORTH_TIB(x) (x->b + 3*sizeof(C))

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
	I i;
	FORTH_compile_cell(x, LATEST(x));
	LATEST(x) = here;	
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
	FORTH_HERE(x) = 3*sizeof(C) + FORTH_TIB_SIZE;
	FORTH_LATEST(x) = 0;
	FORTH_IN(x) = 0;

	FORTH_add_primitive(x, "DROP", 4, "d", 1);
	FORTH_add_primitive(x, "SWAP", 4, "s", 1);

	FORTH_add_primitive(x, ">IN", 3, "b.cc+b.+.+", 10);
	FORTH_add_primitive(x, "HERE", 4, "b.b..+", 6);
}

void FORTH_quit(X* x) {
	do {
		/* Erase stacks */
		/* Refill */
		/* Interpret or compile as needed */
	} while(1);
}

#endif
