#include "sloth.h"

/* -- Initialization of Sloth context ------------------ */

void init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;

	x->jmpbuf_idx = -1;
}

/* -- Data stack --------------------------------------- */

void push(X* x, CELL v) { 
	x->s[x->sp] = v; x->sp++; 
}

CELL pop(X* x) { 
	x->sp--; 
	return x->s[x->sp]; 
}

CELL pick(X* x, CELL a) { 
	return x->s[x->sp - a - 1]; 
}

/* -- Return stack -------------------------------------- */

void rpush(X* x, CELL v) { 
	x->r[x->rp] = v; x->rp++; 
}

CELL rpop(X* x) { 
	x->rp--; 
	return x->r[x->rp]; 
}

CELL rpick(X* x, CELL a) { 
	return x->r[x->rp - a - 1]; 
}

/* -- Memory ------------------------------------------- */

/* 
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void store(X* x, CELL a, CELL v) {
	*((CELL*)a) = v; 
}

CELL fetch(X* x, CELL a) { 
	return *((CELL*)a); 
}

void cstore(X* x, CELL a, CHAR v) { 
	*((CHAR*)a) = v; 
}

CHAR cfetch(X* x, CELL a) { 
	return *((CHAR*)a); 
}

/*
The next two functions allow transforming from relative to
absolute addresses.
*/
CELL to_abs(X* x, CELL a) { 
	return (CELL)(x->d + a); 
}

CELL to_rel(X* x, CELL a) { 
	return a - x->d; 
}

/* -- Inner interpreter -------------------------------- */

CELL op(X* x) { 
	CELL o = fetch(x, to_abs(x, x->ip));
	x->ip += sCELL;
	return o; 
}

void do_prim(X* x, CELL p) { 
	(x->p->p[-1 - p])(x); 
}

void call(X* x, CELL q) { 
	if (x->ip >= 0) rpush(x, x->ip); 
	x->ip = q; 
}

void execute(X* x, CELL q) { 
	if (q < 0) do_prim(x, q); 
	else call(x, q); 
}

void inner(X* x) { 
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) { 
		execute(x, op(x));
	} 
}

void eval(X* x, CELL q) { 
	execute(x, q); 
	if (q > 0) inner(x); 
}

/* -- Exceptions --------------------------------------- */

void catch(X* x, CELL q) {
	int tsp = x->sp;
	int trp = x->rp;
	int tip = x->ip;
	int e;

	if (!(e = setjmp(x->jmpbuf[x->jmpbuf_idx++]))) {
		eval(x, q);
		push(x, 0);
	} else {
		x->sp = tsp;
		x->rp = trp;
		x->ip = tip;
		push(x, (CELL)e);
	}

	x->jmpbuf_idx--;
}

void throw(X* x, CELL e) {
	longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
}

/* -- Loading and saving already bootstrapped images --- */

void load_image(X* x, char* filename) {
	FILE *f = fopen(filename, "rb");
	CELL sz;
	if (f) {
		fseek(f, 0L, SEEK_END);
		sz = ftell(f);
		rewind(f);
		if (x->sz < sz) { return; /* TODO: Error */ }
		fread((void*)x->d, 1, sz, f);
		fclose(f);
	} else {
		return; /* TODO: Error */
	}
}

void save_image(X* x, char* filename) {
	FILE *f = fopen(filename, "wb");
	if (f) {
		fwrite((void*)x->d, 1, x->sz, f);
		fclose(f);
	} else {
		return; /* TODO: Error */
	}
}


