#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

/* --------------------------------------------------------------------------------------------- */
/* -- Virtual machine -------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

typedef int8_t CHAR;
typedef int64_t CELL;

#define SSIZE 64
#define RSIZE 64
#define MSIZE 65536
#define PSIZE 256

struct VM;
typedef void (*F)(struct VM*);

typedef struct VM { CELL s[SSIZE], sp, r[RSIZE], rp, ip, mp, lp; CHAR m[MSIZE]; F p[PSIZE]; } X;

void init(X* x) { x->sp = 0; x->rp = 0; x->ip = -1; x->mp = 0; x->lp = 0; }

/* Data stack */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }

void place(X* x, CELL a, CELL v) { x->s[x->sp - a - 1] = v; }
int pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

void drop(X* x) { x->sp--; }
void swap(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); }
void _pick(X* x) { push(x, pick(x, pop(x))); }

/* Return stack */

void rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL rpop(X* x) { x->rp--; return x->r[x->rp]; }

CELL rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

void to_r(X* x) { rpush(x, pop(x)); }
void r_fetch(X* x) { push(x, rpick(x, 0)); }
void from_r(X* x) { push(x, rpop(x)); }

/* Memory */

void store(X* x, CELL a, CELL v) { *((CELL*)(x->m + a)) = v; }
CELL fetch(X* x, CELL a) { return *((CELL*)(x->m + a)); }

void _store(X* x) { CELL a = pop(x); store(x, a, pop(x)); }
void _fetch(X* x) { push(x, fetch(x, pop(x))); }

void cstore(X* x, CELL a, CHAR v) { x->m[a] = v; }
CHAR cfetch(X* x, CELL a) { return x->m[a]; }

void _cstore(X* x) { CELL a = pop(x); cstore(x, a, (CHAR)pop(x)); }
void _cfetch(X* x) { push(x, (CELL)cfetch(x, pop(x))); }

CELL here(X* x) { return x->mp; }
void _here(X* x) { push(x, x->mp); }

void allot(X* x, CELL v) { x->mp += v; }
void _allot(X* x) { allot(x, pop(x)); }

void align(X* x) { x->mp = (x->mp + (sizeof(CELL) - 1)) & ~(sizeof(CELL) - 1); }

/* Inner interpreter */

CELL opcode(X* x) { CELL o = fetch(x, x->ip); x->ip += sizeof(CELL); return o; }

CELL noname(X* x, F f) { x->p[x->lp++] = f; return x->lp - 1; }

void do_prim(X* x, CELL p) { x->p[p](x); }
void call(X* x, CELL q) { if (x->ip >= 0) rpush(x, x->ip); x->ip = q; }

void execute(X* x, CELL q) { if (q < 0) do_prim(x, q); else call(x, q); }
void _execute(X* x) { execute(x, pop(x)); }

void inner(X* x) { CELL t = x->rp; while (t <= x->rp && x->ip >= 0) { execute(x, opcode(x)); } }

void eval(X* x, CELL q) { execute(x, q); inner(x); }

/* --------------------------------------------------------------------------------------------- */
/* -- Bootstrapping ---------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

void colon(X* x, char* n, CELL xt) { }

void bootstrap(X* x) {
	colon(x, "DROP", noname(x, &drop));
	colon(x, "SWAP", noname(x, &swap));
	colon(x, "PICK", noname(x, &_pick));

	colon(x, ">R", noname(x, &to_r));
	colon(x, "R@", noname(x, &r_fetch));
	colon(x, "R>", noname(x, &from_r));

	colon(x, "@", noname(x, &_fetch));
	colon(x, "!", noname(x, &_store));
	colon(x, "C@", noname(x, &_cfetch));
	colon(x, "C!", noname(x, &_cstore));

	colon(x, "HERE", noname(x, &_here));
	colon(x, "ALLOT", noname(x, &_allot));
	colon(x, "ALIGN", noname(x, &align));

	colon(x, "EXECUTE", noname(x, &_execute));
}

int main() {
	X vm;

	init(&vm);
}
