/* TODO: Condition system !!! */
/* TODO: Variables, constants, create >does */
/* TODO: Quotations outside colon definitions */
/* TODO: Maybe dual words? I don't need if I need them */
/* TODO: Clean every line not needed */

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

#define ERR_OK 0
#define ERR_UNDEFINED_WORD -13
#define ERR_ZERO_LEN_NAME -16
#define ERR_SYMBOL_ALLOCATION -256

#define DO(x, f) f(x); if (x->err) return
#define ERR(x, c, e) if (c) { x->err = e; return; }

typedef void V;
typedef char B;
typedef intptr_t C;

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Symbol {
  struct _Symbol* p;
  B f;
	C cl;
  C c;
  B nl;
  B n[1];
} S;

#define MEM_SIZE 65536

typedef struct _Memory {
	S* l;
	C hp;
	B h[MEM_SIZE];
} M;

#define DSTACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context { 
  C d[DSTACK_SIZE]; 
  C dp; 
  C r[RSTACK_SIZE]; 
  C rp; 
  C ip;
  C err;
  C s;
  C n; /* What's this? */
  M* m;
	/* TODO: Take it out of context to memory? */
  V (**x)(struct _Context*);
	/* TODO: Should ibuf be shared between contexts? Move to memory in that case */
	B* ibuf;
	C ilen;
	C ipos;
} X;

V inner(X* x);

#define EXT(x, l) (x->x[l - 'A'])

#define DPUSH(x, u) (x->d[x->dp++] = (C)(u))
#define DPOP(x) (x->d[--x->dp])
#define DDROP(x) (x->dp--)

#define L1(x, t, v) t v = (t)DPOP(x)
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)

#define T(x) (x->d[x->dp - 1])
#define N(x) (x->d[x->dp - 2])
#define NN(x) (x->d[x->dp - 3])

#define GETB(x, a) (x->m->h[(a)])
#define GETS(x, a) (*((int16_t*)&x->m->h[(a)]))
#define GETI(x, a) (*((int32_t*)&x->m->h[(a)]))
#define GETL(x, a) (*((int64_t*)&x->m->h[(a)]))
#define GETC(x, a) (*((C*)&x->m->h[(a)]))

#define PUTB(x, a, b) (x->m->h[(a)] = (b))
#define PUTS(x, a, s) (*((int16_t*)&x->m->h[(a)]) = (int16_t)(s))
#define PUTI(x, a, w) (*((int32_t*)&x->m->h[(a)]) = (int32_t)(w))
#define PUTL(x, a, l) (*((int64_t*)&x->m->h[(a)]) = (int64_t)(l))
#define PUTC(x, a, i) (*((C*)&x->m->h[(a)]) = (i))

#define COMMAB(x, b) { PUTB(x, x->m->hp, b); x->m->hp += 1; }
#define COMMAS(x, w) { PUTS(x, x->m->hp, w); x->m->hp += 2; }
#define COMMAI(x, i) { PUTI(x, x->m->hp, i); x->m->hp += 4; }
#define COMMAL(x, l) { PUTL(x, x->m->hp, l); x->m->hp += 8; }

V literal(X* x) {
	L1(x, C, n);
	if (n == 0) { COMMAB(x, '0'); }
	else if (n == 1) { COMMAB(x, '1'); }
  else if (n > INT8_MIN && n < INT8_MAX) { COMMAB(x, '#'); COMMAB(x, n); }
  else if (n > INT16_MIN && n < INT16_MAX) { COMMAB(x, '2'); COMMAS(x, n); }
  else if (n > INT32_MIN && n < INT32_MAX) { COMMAB(x, '4'); COMMAI(x, n); }
  else { COMMAB(x, '8'); COMMAI(x, n); }
}

/* s->cl can be used to choose between inline or not inline...let's inline everything now */
V compile(X* x) { L1(x, S*, s); int i; while (i < s->cl) { COMMAB(x, GETB(x, s->c + i++)); } }

#define TAIL(x) (x->ip >= MEM_SIZE || GETB(x, x->ip) == ']' || GETB(x, x->ip) == '}')
V call(X* x) { L1(x, C, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = MEM_SIZE; }
V jump(X* x) { L1(x, C, d); x->ip += d - 1; }
V zjump(X* x) { L2(x, C, d, C, b); if (!b) x->ip += d - 1; }
V eval(X* x, C q) { DPUSH(x, q); call(x); inner(x); }
V quotation(X* x) { L1(x, C, d); DPUSH(x, x->ip); x->ip += d - 1; }
V recurse(X* x) { DPUSH(x, x->m->l->c); literal(x); PUTB(x, x->m->hp++, 'e'); }
V ahead(X* x) { DPUSH(x, x->m->hp + 1); DPUSH(x, 1024); literal(x); }
V resolve(X* x) { L1(x, C, a); C d = x->m->hp - a - 2; PUTS(x, a, d); }

V dup(X* x) { DPUSH(x, T(x)); }
V over(X* x) { DPUSH(x, N(x)); }
V swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { C t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }
V nip(X* x) { N(x) = T(x); DDROP(x); }

V to_r(X* x) { x->r[x->rp++] = x->d[--x->dp]; }
V from_r(X* x) { x->d[x->dp++] = x->r[--x->rp]; }

#define OP2(x, op) N(x) = N(x) op T(x); DDROP(x)
V add(X* x) { OP2(x, +); }
V sub(X* x) { OP2(x, -); }
V mul(X* x) { OP2(x, *); }
V division(X* x) { OP2(x, /); }
V mod(X* x) { OP2(x, %); }

V and(X* x) { OP2(x, &); }
V or(X* x) { OP2(x, |);}
V xor(X* x) { OP2(x, ^); }
V not(X* x) { T(x) = !T(x); }
V inverse(X* x) { T(x) = ~T(x); }

V lt(X* x) { OP2(x, <); }
V eq(X* x) { OP2(x, ==); }
V gt(X* x) { OP2(x, >); }

/*
V istore(X* x) { L2(x, I*, a, I, v); *a = v; }
V bstore(X* x) { L2(x, B*, a, B, v); *a = v; }
V ifetch(X* x) { L1(x, I*, a); DPUSH(x, *a); }
V bfetch(X* x) { L1(x, B*, a); DPUSH(x, *a); }
*/

V times(X* x) { L2(x, C, q, C, n); for(;n > 0; n--) eval(x, q); }
V branch(X* x) { L3(x, C, f, C, t, C, b); b ? eval(x, t) : eval(x, f); }

#define S_pr(s, n, f, a) { C t; s += t = sprintf(s, f, a); n += t; }
              
V dump_code(X* x, C c) {
	C n;
	S* s;
  C t = 1;
	B token;
  while (t && c < MEM_SIZE) {
		token = GETB(x, c);
    if (token == '[') t++;
    if (token == ']') t--;
		if (token == '#') {
			if (GETB(x, c + 2) == 'e') {
				C n = GETB(x, ++c);
				s = x->m->l;
				while (s) {
					if (s->c == n) {
						printf("%.*s ", (int)s->nl, s->n);
					}
					s = s->p;
				}
				c += 2;
			} else {
				printf("#%d ", GETB(x, ++c));
				c += 1;
			}
		} else if (token == '2') {
			if (GETB(x, c + 3) == 'e') {
				C n = GETS(x, ++c);
				s = x->m->l;
				while (s) {
					if (s->c == n) {
						printf("%.*s ", (int)s->nl, s->n);
					}
					s = s->p;
				}
				c += 3;
			} else {
				printf("#%d ", GETS(x, ++c));
				c += 2;
			}
		} else if (token == '4') {
		  printf("#%d ", GETI(x, ++c));
			c += 4;
		} else if (token == '8') {
		  printf("#%ld ", GETL(x, ++c));
			c += 8;
		} else {
	    printf("%c", token);
	    c++;
		}
  }
}

V dump_context(X* x) {
  C i;
  B* t;
  for (i = 0; i < x->dp; i++) printf("%ld ", x->d[i]);
  printf("▢ ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" ▢ ");
    dump_code(x, x->r[i]); 
  }
  printf("\n");
}

V parse_name(X* x) {
	while (x->ipos < x->ilen && isspace(x->ibuf[x->ipos])) x->ipos++;
	DPUSH(x, &x->ibuf[x->ipos]);
	while (x->ipos < x->ilen && !isspace(x->ibuf[x->ipos])) x->ipos++;
	DPUSH(x, &x->ibuf[x->ipos] - T(x));
}

V find_name(X* x) {
	L2(x, C, l, B*, t);
	S* s = x->m->l;
	while (s) {
		if (s->nl == l && !strncmp(s->n, t, l)) break;
		s = s->p;
	}
	DPUSH(x, t);
	DPUSH(x, l);
	DPUSH(x, s);
}

V see(X* x) { 
	parse_name(x); 
	find_name(x); 
	{ 
		L3(x, S*, s, C, l, B*, t);
		printf("[%ld] : %.*s ", s->c, (int)l, t);
		dump_code(x, s->c); 
		if ((s->f & IMMEDIATE) == IMMEDIATE) printf(" IMMEDIATE");
		printf("\n");
	} 
}

V create(X* x) {
  DO(x, parse_name);
  ERR(x, T(x) == 0, ERR_ZERO_LEN_NAME);
  {
    L2(x, C, l, B*, n);
    S* s = malloc(sizeof(S) + l);
    ERR(x, !s, ERR_SYMBOL_ALLOCATION);
    s->p = x->m->l;
    x->m->l = s;
    s->f = 0;
    s->cl = 0;
    s->c = x->m->hp;
    s->nl = l;
    strncpy(s->n, n, l);
    s->n[l] = 0;
  }
}
  
V colon(X* x) {
  DO(x, create);
  x->m->l->f = HIDDEN;
  x->s = 1;
}

V semicolon(X* x) { COMMAB(x, ']');	x->s = 0;	x->m->l->f &= ~HIDDEN; x->m->l->cl = x->m->hp - x->m->l->c - 1; }
V immediate(X* x) {	x->m->l->f |= IMMEDIATE; }

/* Postpone could not inline everything, but right now, it is. */
V postpone(X* x) { 
	parse_name(x); 
	find_name(x);
	{
		L3(x, S*, s, C, _, B*, __);
		int i;
		for (i = 0; i < s->cl; i++) {
			COMMAB(x, '$');
			COMMAB(x, GETB(x, s->c + i));
		}
	}
}

#define PEEK(x) (GETB(x, x->ip))
#define TOKEN(x) (GETB(x, x->ip++))
                
V step(X* x) {
		/*dump_context(x);*/
  if (!x->err) {
  	switch (PEEK(x)) {
  	  case 'A': case 'B': 
  	  case 'C':	case 'D': 
  	  case 'E': case 'F':
  	  case 'G': case 'H':
  	  case 'I': case 'J': 
  	  case 'K': case 'L':
  	  case 'M': case 'N': 
  	  case 'O': case 'P':
  	  case 'Q': case 'R': 
  	  case 'S': case 'T':
  	  case 'U': case 'V': 
  	  case 'W': case 'X':
  	  case 'Y': case 'Z':
  	    EXT(x, TOKEN(x))(x);
  	    break;
			default:
				switch (TOKEN(x)) {
				/*
				case 'l': literal(x); break;
				*/

  		  case '0': DPUSH(x, 0); break;
  		  case '1': DPUSH(x, 1); break;
				case '#': DPUSH(x, GETB(x, x->ip)); x->ip += 1; break;
 				case '2': DPUSH(x, GETS(x, x->ip)); x->ip += 2; break;
 				case '4': DPUSH(x, GETI(x, x->ip)); x->ip += 4; break;
 				case '8': DPUSH(x, GETL(x, x->ip)); x->ip += 8; break;

				case 'x': call(x); break;
				case '[': quotation(x); break;
  	  	case ']': case '\\': ret(x); break;
				case 'j': jump(x); break;
				case 'z': zjump(x); break;

				case '{': { L1(x, C, e); x->err = e; }; break;
				/*case '}': catch_error(x); break;*/

				case ':': colon(x); break;
				case ';': semicolon(x); break;
				case 'i': immediate(x); break;
				case '$': COMMAB(x, GETB(x, x->ip++)); break;
				case '`': recurse(x); break;
				case 'a': ahead(x); break;
				case '@': resolve(x); break;

			  case '_': DDROP(x); break;
  		  case 's': swap(x); break;
  		  case 'o': over(x); break;
  		  case 'd': dup(x); break;
  		  case 'r': rot(x); break;
				case 'n': nip(x); break;

				case '(': to_r(x); break;
				case ')': from_r(x); break;

  		  case '+': add(x); break;
  		  case '-': sub(x); break;
  		  case '*': mul(x); break;
  		  case '/': division(x); break;
  		  case '%': mod(x); break;

  		  case '&': and(x); break;
  		  case '|': or(x); break;
  		  case '!': not(x); break;
  		  case '^': xor(x); break;
  		  case '~': inverse(x); break;

  		  case '<': lt(x); break;
  		  case '=': eq(x); break;
  		  case '>': gt(x); break;
				/*
  		  case ',': istore(x); break;
  		  case ';': bstore(x); break;
  		  case '.': ifetch(x); break;
  		  case ':': bfetch(x); break;
				*/
  		  case 't': times(x); break;
  		  case '?': branch(x); break;

				case 'p': postpone(x); break;

				case 'w':
					switch (TOKEN(x)) {
						case 's': see(x); break;
					}
					break;
			}
  	}
  }
}
              
V inner(X* x) { C rp = x->rp; while(x->rp >= rp && x->ip < MEM_SIZE && !x->err) { step(x); } }

V evaluate(X* x, B* s) {
	x->ibuf = s;	
	x->ilen = strlen(s);
	x->ipos = 0;
	while (x->ipos < x->ilen && !x->err) {
		parse_name(x);
		if (T(x) == 0) { DDROP(x); DDROP(x); return; }
		find_name(x);
		if (T(x)) {
			L3(x, S*, s, C, l, B*, t);
			if (!x->s || (s->f & IMMEDIATE) == IMMEDIATE) {
				eval(x, s->c);
			} else {
				DPUSH(x, s);
				compile(x);
			}
		} else {
			L3(x, S*, _, C, l, B*, t);
			if (t[0] == '\\') {
				int i;
				for (i = 1; i < l; i++) { PUTB(x, MEM_SIZE - l + i, t[i]); }
				eval(x, MEM_SIZE - l + 1);
			} else {
				char* end;
				int n = strtol(t, &end, 10);
        ERR(x, n == 0 && end == t, ERR_UNDEFINED_WORD);
				DPUSH(x, n);
				if (x->s) literal(x);
			}
		}
	}
}

V reset_context(X* x) {
	x->err = 0;
	x->dp = 0;
	x->rp = 0;
}

X* init_VM(M* m) { 
	X* x = malloc(sizeof(X)); 
	x->m = m; 
	x->ip = MEM_SIZE;

	return x;
}

X* init_EXT(X* x) { x->x = malloc(26*sizeof(C)); return x; }

M* init_MEM() { 
	M* m = malloc(sizeof(M));
	m->l = 0;
	m->hp = 0;
	return m;
}

X* init_SLOTH(X* x) {
	evaluate(x, "\\: : \\$: \\;");
	evaluate(x, ": ; \\$; \\;i");

	evaluate(x, ": immediate \\$i ;");
	evaluate(x, ": execute \\$x ;");

	evaluate(x, ": recurse \\$` ; immediate");

	evaluate(x, ": >mark \\$a ;");
	evaluate(x, ": >resolve \\$@ ;");
	evaluate(x, ": 0branch \\$z ;");
	evaluate(x, ": jump \\$j ;");

	evaluate(x, ": postpone \\$p ; immediate");

	evaluate(x, ": drop \\$_ ;");
	evaluate(x, ": dup \\$d ;");
	evaluate(x, ": over \\$o ;");
	evaluate(x, ": swap \\$s ;");
	evaluate(x, ": rot \\$@ ;");
	evaluate(x, ": nip \\$n ;");

	evaluate(x, ": + \\$+ ;");
	evaluate(x, ": - \\$- ;");
	evaluate(x, ": * \\$* ;");
	evaluate(x, ": / \\$/ ;");
	evaluate(x, ": mod \\$% ;");

	evaluate(x, ": < \\$< ;");
	evaluate(x, ": = \\$= ;");
	evaluate(x, ": > \\$> ;");

	evaluate(x, ": and \\$& ;");
	evaluate(x, ": or \\$| ;");
	evaluate(x, ": invert \\$~ ;");
	
	evaluate(x, ": see \\$w$s ;");

	evaluate(x, ": if >mark postpone 0branch ; immediate");
	evaluate(x, ": else >mark postpone jump swap >resolve ; immediate");
	evaluate(x, ": then >resolve ; immediate");

	evaluate(x, ": fib dup 1 > if 1 - dup 1 - recurse swap recurse + then ;");

	return x;
}

#endif
