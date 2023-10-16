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

V literal(X* x) {
	L1(x, C, n);
	if (n == 0) PUTB(x, x->m->hp++, '0');
	else if (n == 1) PUTB(x, x->m->hp++, '1');
  else if (n > INT8_MIN && n < INT8_MAX) {
    PUTB(x, x->m->hp++, '#');
    PUTB(x, x->m->hp++, n);
  } else if (n > INT16_MIN && n < INT16_MAX) {
    PUTB(x, x->m->hp++, '2');
    PUTS(x, x->m->hp, n);
		x->m->hp += 2;
  } else if (n > INT32_MIN && n < INT32_MAX) {
    PUTB(x, x->m->hp++, '4');
    PUTI(x, x->m->hp, n);
 		x->m->hp += 4;
 } else {
    PUTB(x, x->m->hp++, '8');
    PUTL(x, x->m->hp, n);
 		x->m->hp += 8;
  }
}

V word(X* x) {
	L1(x, S*, s);
	printf("Compiling %.*s\n", (int)s->l, s->n);
	DPUSH(x, s->c);
	literal(x);
	PUTB(x, x->m->hp++, 'e');
}

#define TAIL(x) (x->ip >= MEM_SIZE || GETB(x, x->ip + 1) == ']' || GETB(x, x->ip + 1) == '}')
V call(X* x) { L1(x, C, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = MEM_SIZE; }
V zjump(X* x) { L1(x, C, b); if (!b) x->ip = b; }
V eval(X* x, C q) { DPUSH(x, q); call(x); inner(x); }
V quotation(X* x) { L1(x, C, n); DPUSH(x, x->ip); x->ip += n - 1; }

V dup(X* x) { DPUSH(x, T(x)); }
V over(X* x) { DPUSH(x, N(x)); }
V swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { C t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }
V nip(X* x) { N(x) = T(x); DDROP(x); }

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
  C t = 1;
	B token;
  while (t && c < MEM_SIZE) {
		token = GETB(x, c);
    if (token == '[') t++;
    if (token == ']') t--;
		if (token == '#') {
			printf("#%d", GETB(x, ++c));
			c += 1;
		} else if (token == '2') {
		  printf("#%d", GETS(x, ++c));
			c += 2;
		} else if (token == '4') {
		  printf("#%d", GETI(x, ++c));
			c += 4;
		} else if (token == '8') {
		  printf("#%ld", GETL(x, ++c));
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
  printf(": ");
  dump_code(x, x->ip);
  for (i = x->rp - 1; i >= 0; i--) {
    printf(" : ");
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
		if (s->l == l && !strncmp(s->n, t, l)) break;
		s = s->p;
	}
	DPUSH(x, t);
	DPUSH(x, l);
	DPUSH(x, s);
}

/* TODO: Word could not be found! */
V compile(X* x) { 
	printf("Executing compile\n");
	parse_name(x); 
	find_name(x); 
	{ 
		L3(x, S*, s, C, l, B*, t); 
		printf("Word being compiled: %.*s\n", (int)l, t);
		DPUSH(x, s->c); 
		literal(x); 
		PUTB(x, x->m->hp++, 'e'); 
	} 
}

V tick(X* x) { parse_name(x); find_name(x); { L3(x, S*, s, C, _, B*, __); DPUSH(x, s->c); } }
V see(X* x) { 
	parse_name(x); 
	find_name(x); 
	{ 
		L3(x, S*, s, C, l, B*, t);
		printf("[%d] : %.*s ", s->c, (int)l, t);
		dump_code(x, s->c); 
		if ((s->f & IMMEDIATE) == IMMEDIATE) printf(" IMMEDIATE");
		printf("\n");
	} 
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

V immediate(X* x) {	x->m->l->f |= IMMEDIATE; }
/* Postpone must compile code to compile code, */
V postpone(X* x) { 
	parse_name(x); 
	find_name(x); /* Error? */ 
	nip(x); nip(x);
	/* Compile word address to be pushed on the stack */
	literal(x); 
	/* Compile literal to compile top of the stack as a literal */
	PUTB(x, x->m->hp++, 'l'); 
	/* Compile e as a literal to be later compiled */
	DPUSH(x, 'e');
	literal(x);
	PUTB(x, x->m->hp++, 'l');

	/* Compile a call to word to be compiled */
}

V recurse(X* x) { DPUSH(x, x->m->l->c); literal(x); PUTB(x, x->m->hp++, 'e'); }
V ahead(X* x) { 
	printf("AHEAD\n");
	printf("Pushing here+1: %d\n", x->m->hp + 1);
	DPUSH(x, x->m->hp + 1); 
	printf("Saving 1024 for using 2 bytes\n");
	DPUSH(x, 1024); 
	literal(x); 
	printf("Saved value: %d\n", GETS(x, T(x)));
	printf("Current here: %d\n", x->m->hp);
}
V resolve(X* x) { 
	L1(x, C, a); 
	printf("RESOLVE\n");
	printf("Address to set jump: %ld\n", a);
	C d = x->m->hp - a; 
	printf("Distance: %ld\n", d);
	PUTS(x, a, d); 
	printf("Value set to: %d\n", GETS(x, a));
}

#define PEEK(x) (GETB(x, x->ip))
#define TOKEN(x) (GETB(x, x->ip++))
                
V step(X* x) {
		dump_context(x);
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
				case 'z': zjump(x); break;
  		  case '0': DPUSH(x, 0); break;
  		  case '1': DPUSH(x, 1); break;
				case '#': DPUSH(x, GETB(x, x->ip)); x->ip += 1; break;
 				case '2': DPUSH(x, GETS(x, x->ip)); x->ip += 2; break;
 				case '4': DPUSH(x, GETI(x, x->ip)); x->ip += 4; break;
 				case '8': DPUSH(x, GETL(x, x->ip)); x->ip += 8; break;
			  case '_': DDROP(x); break;
  		  case 's': swap(x); break;
  		  case 'o': over(x); break;
  		  case 'd': dup(x); break;
  		  case '@': rot(x); break;
				case 'n': nip(x); break;
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
				case 'p': postpone(x); break;

				case 'l': literal(x); break;

				case '`': recurse(x); break;
				case 'a': ahead(x); break;
				case 'r': resolve(x); break;
				case 'c': compile(x); break;
				case 'w':
					switch (TOKEN(x)) {
						case 's': see(x); break;
					}
					break;
			}
  	}
}
              
V inner(X* x) { C rp = x->rp; while(x->rp >= rp && x->ip < MEM_SIZE) { step(x); } }

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
				printf("INTERPRETING %.*s\n", (int)l, t);
				eval(x, s->c);
			} else {
				printf("COMPILING %.*s\n", (int)l, t);
				DPUSH(x, s->c);
				literal(x);
				PUTB(x, x->m->hp++, 'e');
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
X* init_SLOTH(X* x) {
	evaluate(x, "\\: : $: \\;");
	evaluate(x, ": ; $; \\;i");

	evaluate(x, ": immediate $i ;");
	evaluate(x, ": postpone $p ; immediate");

	evaluate(x, ": drop $_ ;");
	evaluate(x, ": dup $d ;");
	evaluate(x, ": over $o ;");
	evaluate(x, ": swap $s ;");
	evaluate(x, ": rot $@ ;");
	evaluate(x, ": nip $n ;");

	evaluate(x, ": + $+ ;");
	evaluate(x, ": - $- ;");
	evaluate(x, ": * $* ;");
	evaluate(x, ": / $/ ;");
	evaluate(x, ": mod $% ;");

	evaluate(x, ": < $< ;");
	evaluate(x, ": = $= ;");
	evaluate(x, ": > $> ;");

	evaluate(x, ": and $& ;");
	evaluate(x, ": or $| ;");
	evaluate(x, ": invert $~ ;");

	/* a must be executed, z must be compiled ?! */
	/*
	evaluate(x, ": if $az ; immediate");
	evaluate(x, ": then $r ; immediate");
	*/
	evaluate(x, ": >mark $a ;");
	evaluate(x, ": >resolve $r ;");
	/*
	evaluate(x, ": compile $c ;");
	*/
	evaluate(x, ": 0branch $z ;");
	evaluate(x, ": if >mark postpone 0branch ; immediate");
	evaluate(x, ": then >resolve ; immediate");

	evaluate(x, ": see $ws ;");

	return x;
}

#endif
