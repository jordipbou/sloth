#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

typedef void V;
typedef char B;
typedef intptr_t C;

#define HIDDEN 1
#define IMMEDIATE 2

typedef struct _Symbol {
  struct _Symbol* p;
  B f;
  C c;
  B l;
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
  C s;
  C n;
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

#define GETB(x, a) (x->m->h[a])
#define GETS(x, a) (*((int16_t*)&x->m->h[a]))
#define GETI(x, a) (*((int32_t*)&x->m->h[a]))
#define GETL(x, a) (*((int64_t*)&x->m->h[a]))
#define GETC(x, a) (*((C*)&x->m->h[a]))

#define PUTB(x, a, b) (x->m->h[a] = b)
#define PUTS(x, a, s) (*((int16_t*)&x->m->h[a]) = (int16_t)s)
#define PUTI(x, a, w) (*((int32_t*)&x->m->h[a]) = (int32_t)w)
#define PUTL(x, a, l) (*((int64_t*)&x->m->h[a]) = (int64_t)l)
#define PUTC(x, a, i) (*((C*)&x->m->h[a]) = i)

#define TAIL(x) (x->ip >= MEM_SIZE || GETB(x, x->ip + 1) == ']' || GETB(x, x->ip + 1) == '}')
V call(X* x) { L1(x, C, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = MEM_SIZE; }
V eval(X* x, C q) { DPUSH(x, q); call(x); inner(x); }
V quotation(X* x) { L1(x, C, n); DPUSH(x, x->ip); x->ip += n - 1; }

V dup(X* x) { DPUSH(x, T(x)); }
V over(X* x) { DPUSH(x, N(x)); }
V swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { C t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }

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

/*
#define S_pr(s, n, f, a) { C t; s += t = sprintf(s, f, a); n += t; }
              
V dump_code(X* x, I c) {
  I t = 1;
  while (t) {
    if (GET(x, c) == '[') t++;
    if (GET(x, c) == ']') t--;
    printf("%c", GET(x, c));
    c++;
  }
}

V dump_context(X* x) {
  I i;
  B* t;
  for (i = 0; i < x->dp; i++) printf("%ld ", x->ds[i]);
  printf(": ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" : ");
    dump_code(x, x->rs[i]); 
  }
  printf("\n");
}
*/

V parse_name(X* x) {
	while (x->ipos < x->ilen && isspace(x->ibuf[x->ipos])) x->ipos++;
	DPUSH(x, &x->ibuf[x->ipos]);
	while (x->ipos < x->ilen && !isspace(x->ibuf[x->ipos])) x->ipos++;
	DPUSH(x, &x->ibuf[x->ipos] - T(x));
}

V colon(X* x) {
	parse_name(x);
	if (T(x) == 0) { DDROP(x); DDROP(x); /* Error */ return; }
	else {
		L2(x, C, l, B*, n);
		S* s = malloc(sizeof(S));
		s->p = x->m->l;
		x->m->l = s;
		s->f = HIDDEN;
		s->c = x->m->hp;
		s->l = l;
		strncpy(s->n, n, l);
		s->n[l] = 0;
		x->s = 1;
	}
}

V semicolon(X* x) {
	PUTB(x, x->m->hp++, ']');
	x->s = 0;
	x->m->l->f &= ~HIDDEN;
}

V immediate(X* x) {
	x->m->l->f |= IMMEDIATE;
}

#define PEEK(x) (GETB(x, x->ip))
#define TOKEN(x) (GETB(x, x->ip++))
                
V step(X* x) {
		/*dump_context(x);*/
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
				case 'e': call(x); break;
				case '[': quotation(x); break;
  	  	case ']': case '}': ret(x); break;
  		  case '0': DPUSH(x, 0); break;
  		  case '1': DPUSH(x, 1); break;
				case '#': /* TODO */ break;
 				case '2': /* TODO */ break;
 				case '4': /* TODO */ break;
 				case '8': /* TODO */ break;
			  case '_': DDROP(x); break;
  		  case 's': swap(x); break;
  		  case 'o': over(x); break;
  		  case 'd': dup(x); break;
  		  case 'r': rot(x); break;
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
				case ':': colon(x); break;
				case ';': semicolon(x); break;
				case 'i': immediate(x); break;
			}
  	}
}
              
V inner(X* x) { C rp = x->rp; while(x->rp >= rp && x->ip < MEM_SIZE) { step(x); } }

V literal(X* x) {
	L1(x, C, n);
	if (n == 0) PUTB(x, x->m->hp++, '0');
	else if (n == 1) PUTB(x, x->m->hp++, '1');
  else if (n > INT8_MIN && n < INT8_MAX) {
    PUTB(x, x->m->hp++, '#');
    PUTB(x, x->m->hp++, n);
  } else if (n > INT16_MIN && n < INT16_MAX) {
    PUTB(x, x->m->hp++, '2');
    PUTS(x, x->m->hp++, n);
  } else if (n > INT32_MIN && n < INT32_MAX) {
    PUTB(x, x->m->hp++, '4');
    PUTI(x, x->m->hp++, n);
  } else {
    PUTB(x, x->m->hp++, '8');
    PUTL(x, x->m->hp++, n);
  }
}

V find_name(X* x) {
	L2(x, C, l, B*, t);
	S* s = x->m->l;
	while (s) {
		if (s->l == l && !strncmp(s->n, t, l)) break;
		s = s->p;
	}
	DPUSH(x, t);
	DPUSH(x, l);
	DPUSH(x, s);
}

V evaluate(X* x, B* s) {
	x->ibuf = s;	
	x->ilen = strlen(s);
	x->ipos = 0;
	while (x->ipos < x->ilen) {
		parse_name(x);
		if (T(x) == 0) { DDROP(x); DDROP(x); return; }
		find_name(x);
		if (T(x)) {
			L3(x, S*, s, C, l, B*, t);
			if (!x->s || (s->f & IMMEDIATE) == IMMEDIATE) {
				eval(x, s->c);
			} else {
				/* TODO: Compile */
			}
		} else {
			L3(x, S*, _, C, l, B*, t);
			if (t[0] == '\\') {
				int i;
				for (i = 1; i < l; i++) {
					PUTB(x, MEM_SIZE - l + i, t[i]);
				}
				eval(x, MEM_SIZE - l);
			} else if (t[0] == '$') {
				int i;
				for (i = 1; i < l; i++) {
					PUTB(x, x->m->hp++, t[i]);
				}
			} else {
				char* end;
				int n = strtol(t, &end, 10);
				if (n == 0 && end == t) {
					printf("Word not found [%.*s]\n", (int)l, t);
				} else {
					DPUSH(x, n);
					if (x->s) {
						literal(x);
					}
				}
			}
		}
	}
}

X* init_VM(M* m) { X* x = malloc(sizeof(X)); x->m = m; }
X* init_EXT(X* x) { x->x = malloc(26*sizeof(C)); return x; }
M* init_MEM() { return malloc(sizeof(M)); }

#endif