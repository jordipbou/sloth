#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

typedef void V;
typedef char B;
typedef int16_t W;
typedef intptr_t C;

#define PAD_OFFSET 176
#define PAD_SIZE 84

typedef struct _Context X;

typedef struct _Dictionary { 
	C size; 
	C here; 
	C there; 
	B* data; 
	V (**x)(struct _Context*);
} D;

#define HERE_TO_THERE(d) (d->there = d->here + PAD_OFFSET + PAD_SIZE)

D* init_dict(C s) {
	D* d = malloc(sizeof(D));
	if (d == 0) return 0;
	d->data = malloc(s);
	if (d->data == 0) { free(d); return 0; }
	d->size = s;
	d->here = 0;
	HERE_TO_THERE(d);
	d->x = malloc(26 * sizeof(C));
	if (d->x == 0) { free(d->data); free(d); return 0; }
	return d;
}

C aligned(D* d) { return (d->here + (sizeof(C) - 1)) & ~(sizeof(C) - 1); }
V align(D* d) { d->here = aligned(d); }
V allot(D* d, C n) { d->here += n; }

V bcompile(D* d, B v) { *((B*)(d->data + d->here)) = v; d->here += 1; }
V wcompile(D* d, W v) { *((W*)(d->data + d->here)) = v; d->here += 2; }
V ccompile(D* d, C v) { *((C*)(d->data + d->here)) = v; d->here += sizeof(C); }

V btransient(D* d, B v) { *((B*)(d->data + d->there)) = v; d->there += 1; }
V wtransient(D* d, W v) { *((W*)(d->data + d->there)) = v; d->there += 2; }
V ctransient(D* d, C v) { *((C*)(d->data + d->there)) = v; d->there += sizeof(C); }

C unused(D* d) { return d->size - d->here; }
C tunused(D* d) { return d->size - d->there; }

B getB(D* d, C p) { return *((B*)(d->data + p)); }
W getW(D* d, C p) { return *((W*)(d->data + p)); }
C getC(D* d, C p) { return *((C*)(d->data + p)); }

V putB(D* d, C p, B v) { *((B*)(d->data + p)) = v; }
V putW(D* d, C p, W v) { *((W*)(d->data + p)) = v; }
V putC(D* d, C p, C v) { *((C*)(d->data + p)) = v; }

C str_to_transient(D* d, B* s, C l) {
	C t, i;
	if (tunused(d) < l) HERE_TO_THERE(d);
	if (tunused(d) < l) return -1;
	t = d->there;
	for (i = 0; i < l; i++) btransient(d, *(s + i));
	return t;
}

#define SSTACK_SIZE 256
#define RSTACK_SIZE 256

struct _Context { 
	D* d; 
	C s[SSTACK_SIZE]; 
	C sp; 
	C r[RSTACK_SIZE]; 
	C rp; 
	C ip; 
	C err; 
	C tr;
};

#define EXT(x, l) (x->d->x[l - 'A'])

X* init_context(D* d) {
	X* x = malloc(sizeof(X));
	x->d = d;
	x->sp = x->rp = x->err = x->tr = 0;
	x->ip = 0;
	return x;
}

V inner(X*);

#define T(x) (((X*)(x))->s[((X*)(x))->sp - 1])
#define N(x) (((X*)(x))->s[((X*)(x))->sp - 2])
#define NN(x) (((X*)(x))->s[((X*)(x))->sp - 3])

#define PUSH(x, v) (((X*)(x))->s[((X*)(x))->sp++] = ((C)(v)))
#define POP(x) (((X*)(x))->s[--((X*)(x))->sp])
#define DROP(x) (((X*)(x))->sp--)
#define SWAP(x) { C a = T(x); T(x) = N(x); N(x) = a; }
#define OVER(x) (PUSH(x, N(x)))

#define L1(x, t1, v1) t1 v1 = (t1)POP((X*)(x))
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)

#define PEEK(x) (getB(x->d, x->ip))
#define TOKEN(x) (getB(x->d, x->ip++))
#define IN(x, a) ((a) >= 0 && (a) <= x->d->size)

#define TAIL(x) (!IN(x, x->ip) || PEEK(x) == ']' || PEEK(x) == '}')
V execute(X* x) { L1(x, C, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V jump(X* x) { L1(x, C, d); x->ip += d; }
V zjump(X* x) { L2(x, C, d, C, c); if (!c) x->ip += d; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = x->d->size; }
V eval(X* x, C q) { PUSH(x, q); execute(x); inner(x); }
V quotation(X* x) { L1(x, C, d); PUSH(x, x->ip); x->ip += d; }

V quit(X* x) { x->err = -256; }

V literal(X* x, C n) {
	if (n == 1) { bcompile(x->d, '1'); }
	else if (n >= -128 && n <= 127) { bcompile(x->d, '\''); bcompile(x->d, (B)n); }
	else if (n >= -32768 && n <= 32767) { bcompile(x->d, '2'); wcompile(x->d, (W)n); }
	/* TODO: Manage 32/64 bits */
	else { bcompile(x->d, '8'); ccompile(x->d, n); }
}

V dump(X* x) {
	L2(x, C, n, B*, a);
	C i;
	while (n > 0) {
		printf("\n%010ld  ", (C)a);
		for (i = 0; i < 8; i++) printf("%02X ", (*(a + i)) & 0xFF);
		printf(" ");
		for (i = 0; i < 8; i++) printf("%02X ", (*(a + 8 + i)) & 0xFF);
		printf("   ");
		for (i = 0; i < 16; i++) {
			B k = *(a + i);
			if (k >= 32 && k <= 127) printf("%c", k);
			else printf(".");
		}
		a += 16;
		n -= 16;
	}
	printf("\n");
}

V string(X* x) { 
	PUSH(x, x->d->data + x->ip); 
	while(TOKEN(x) != '"') { } 
	PUSH(x, x->d->data + x->ip - T(x) - 1); 
}
V number(X* x) { 
	C n = 0; 
	B k; 
	for (k = TOKEN(x); k >= 48 && k <= 57; k = TOKEN(x)) n = 10*n + (k - 48);
	x->ip--;
	PUSH(x, n);
}
V block(X* x) { 
	C t = 1; 
	PUSH(x, x->ip); 
	while (t > 0) { 
		switch (TOKEN(x)) { 
			case '{': t++; break;
			case '}': t--; break;
		}
	}
}

V times(X* x) { L2(x, C, q, C, n); for (;n > 0; n--) { eval(x, q); } }
V choice(X* x) { L2(x, C, f, C, t); if (POP(x)) eval(x, t); else eval(x, f); }

V type(X* x) { 
	L2(x, C, n, B*, s); 
	C i; 
	for (i = 0; i < n; i++) { 
		PUSH(x, *(s + i)); 
		EXT(x, 'E')(x); 
	} 
}

V step(X* x) {
	C a, b, c;
	switch (PEEK(x)) {
	case 'A': case 'B':
	case 'C': case 'D':
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
		case '"': string(x); break;
		case '#': number(x); break;
		case '?': choice(x); break;
		case 't': times(x); break;
		case 'y': type(x); break;

		case '$': bcompile(x->d, TOKEN(x)); break;

		case '1': PUSH(x, 1); break;
		case '\'': PUSH(x, TOKEN(x)); break;
		case '2': PUSH(x, *((W*)x->ip)); x->ip += 2; break;
		case '8': PUSH(x, *((C*)x->ip)); x->ip += sizeof(C); break;

		case '_': DROP(x); break;
		case 'd': a = T(x); PUSH(x, a); break;
		case 'o': b = N(x); PUSH(x, b); break;
		case 's': a = T(x); T(x) = N(x); N(x) = a; break;
		case 'r': a = T(x); T(x) = NN(x); NN(x) = N(x); N(x) = a; break;

		case '(': x->r[x->rp++] = x->s[--x->sp]; break;
		case ')': x->s[x->sp++] = x->r[--x->rp]; break;
		case 'f': PUSH(x, x->r[x->rp - 1]); break;

		case '+': N(x) = N(x) + T(x); x->sp--; break;
		case '-': N(x) = N(x) - T(x); x->sp--; break;
		case '*': N(x) = N(x) * T(x); x->sp--; break;
		case '/': N(x) = N(x) / T(x); x->sp--; break;
		case '%': N(x) = N(x) % T(x); x->sp--; break;

		case '<': N(x) = N(x) < T(x) ? -1 : 0; x->sp--; break;
		case '=': N(x) = N(x) == T(x) ? -1 : 0; x->sp--; break;
		case '>': N(x) = N(x) > T(x) ? -1 : 0; x->sp--; break;
		case '0': T(x) = T(x) == 0 ? -1 : 0; break;

		case '&': N(x) = N(x) & T(x); x->sp--; break;
		case '|': N(x) = N(x) | T(x); x->sp--; break;
		case '^': N(x) = N(x) ^ T(x); x->sp--; break;
		case '~': T(x) = ~T(x); break;
	
		/* Memory access must be absolute to allow the full power of the Forth !!! */
		case '!': { L2(x, C*, a, C, v); *a = v; } break;
		case '@': { L1(x, C*, a); PUSH(x, *a); } break;
		case ';': { L2(x, W*, a, W, v); *a = v; } break;
		case ':': { L1(x, W*, a); PUSH(x, *a); } break;
		case ',': { L2(x, B*, a, B, v); *a = v; } break;
		case '.': { L1(x, B*, a); PUSH(x, *a); } break;

		case '[': quotation(x); break;
		case '}': case ']': ret(x); break;
		case '{': block(x); break;

		case 'i': execute(x); break;
		case 'j': jump(x); break;
		case 'z': zjump(x); break;
	
		case 'c': PUSH(x, sizeof(C)); break;
		case 'b': PUSH(x, x->d->data); break;
		case 'e': x->err = POP(x); break;
		case 'q': quit(x); break;

		case '`': dump(x); break;

		case 'h': PUSH(x, x->d->data + x->d->here); break;
		case 'a': { L1(x, C, n); allot(x->d, n); } break;
		case 'g': align(x->d); break;

		case 'v': { L1(x, C, n); x->tr = n != 0; } break;
		}
		break;
	}
}

V dump_code(X* x, C c) {
	C t = 1;
	B k;
	while (t > 0 && IN(x, c)) {
		switch (k = getB(x->d, c++)) {
		case '#': printf("#"); while (k = getB(x->d, c++) >= 48 && k <= 57) printf("%c", k); c--; break;
		case '{': case '[': t++; printf("%c", k); break;
		case '}': case ']': t--; printf("%c", k); break;
		case '\'': printf("#%d", getB(x->d, c++)); break;
		case '2': printf("#%d", getW(x->d, c)); c += 2; break;
		case '8': printf("#%ld", getC(x->d, c)); c += sizeof(C); break;
		default: if (k >= 32 && k < 127) printf("%c", k); break;
		}
	}
}

V trace(X* x) {
	C i;
	for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); }
	if (IN(x, x->ip)) { printf(" : "); dump_code(x, x->ip); }
	for (i = x->rp - 1; i >= 0; i--) { printf(" : "); dump_code(x, x->r[i]); }
}

V inner(X* x) {
	C t = x->rp;
	while (t <= x->rp && IN(x, x->ip)) {
		if (x->tr) { trace(x); printf("\n"); }
		step(x);
	}
}

V assembler(X* x, B* s) {
	x->ip = str_to_transient(x->d, s, strlen(s));
	btransient(x->d, ']');
	inner(x);
}

typedef struct _Sloth {
	X x;
	B* ibuf;
	C ipos;
	C ilen;
	C latest;
	C state;
} S;

S* init_sloth(D* d) {
	S* s = malloc(sizeof(S));
	X* x = (X*)s;
	x->d = d;
	x->sp = x->rp = x->err = x->tr = 0;
	x->ip = x->d->size;
	s->ibuf = 0;
	s->ipos = s->ilen = 0;
	s->latest = -1;
	s->state = 0;
	return s;
}

#define NO_FLAGS 0
#define HIDDEN 1
#define EXECUTABLE 2
#define IMMEDIATE 4

#define wPREVIOUS 0
#define wXT (wPREVIOUS + sizeof(C))
#define wFLAGS (wXT + sizeof(C))
#define wNAMELEN (wFLAGS + 1)
#define wNAME (wNAMELEN + 1)

V parse_name(S* s) {
	X* x = (X*)s;
	while (s->ipos < s->ilen && isspace(*(s->ibuf + s->ipos))) { s->ipos++; }
	PUSH(x, s->ibuf + s->ipos);
	while (s->ipos < s->ilen && !isspace(*(s->ibuf + s->ipos))) { s->ipos++; }
	PUSH(x, s->ibuf + s->ipos - T(x));
}

B compare_without_case(S* s, C w, C l, B* t) {
	X* x = (X*)s;
	if (getB(x->d, w + wNAMELEN) != l) return 0;
	else {
		int i;
		for (i = 0; i < l; i++) {
			B a = getB(x->d, w + wNAME + i);
			B b = *(t + i);
			if (a >= 97 && a <= 122) a -= 32;
			if (b >= 97 && b <= 122) b -= 32;
			if (a != b) return 0;
		}
		return 1;
	}
}

V find_name(S* s) {
	X* x = (X*)s;
	L2(x, C, l, B*, t);
	C w = s->latest;
	while (w != -1) {
		if ((getB(x->d, w + wFLAGS)	& HIDDEN) != HIDDEN && compare_without_case(s, w, l, t)) break;
		w = getC(x->d, w + wPREVIOUS);
	}
	PUSH(x, w == -1 ? 0 : w);
}

C do_interpret(S* s, C w) {
	X* x = (X*)s;
	B f = getB(x->d, w + wFLAGS);
	if ((f & EXECUTABLE) == EXECUTABLE) {
		eval(x, getC(x->d, w + wXT));
	} else {
		PUSH(x, x->d->data + getC(x->d, w + wXT));
	}
}

/* TODO: do_compile should check if state is less than 0 to compile on transient memory */
C do_compile(S* s, C w) {
	X* x = (X*)s;
	C t = 1;
	C i = 0;
	C xt = getC(x->d, w + wXT);
	B k;
	while (t > 0) {
		switch (k = getB(x->d, xt + i)) {
		case ']': case '}': t--; if (t > 0) bcompile(x->d, k); break;
		case '[': case '{': t++; bcompile(x->d, k); break;
		default: bcompile(x->d, k); break;
		}
		i++;
	}
}

C do_asm(S* s, C l, B* t) {
	X* x = (X*)s;
	if (l <= 1 || *t != '\\') return 0;
	else {
		C q = str_to_transient(x->d, t + 1, l - 1);
		btransient(x->d, ']');
		eval(x, q);
		return 1;
	}
}

C do_casm(S* s, C l, B* t) {
	X* x = (X*)s;
	if (l <= 1 || *t != '$') return 0;
	else {
		C i; for (i = 1; i < l; i++) bcompile(x->d, *(t + i));
		return 1;
	}
}

V header(S* s) {
	X* x = (X*)s;
	parse_name(s);
	if (!T(s)) { DROP(s); DROP(s); x->err = -16; return; }
	else {
		L2(s, C, l, B*, t);
		C w, i;
		align(x->d);
		w = x->d->here;
		ccompile(x->d, s->latest);
		s->latest = w;
		ccompile(x->d, 0);
		bcompile(x->d, NO_FLAGS);
		bcompile(x->d, l);
		for (i = 0; i < l; i++) bcompile(x->d, *(t + i));
		align(x->d);
		putC(x->d, w + wXT, x->d->here);
	}
}

C do_colon(S* s, C l, B* t) {
	X* x = (X*)s;
	if (l != 1 || *t != ':') return 0;
	else {
		C w;
		header(s); if (x->err) return;
		w = s->latest;	
		putB(x->d, w + wFLAGS, HIDDEN | EXECUTABLE);
		s->state = 1;
		return 1;
	}
}

C do_semicolon(S* s, C l, B* t) {
	X* x = (X*)s;
	if (l != 1 || *t != ';') return 0;
	else {
		C w = s->latest;
		bcompile(x->d, ']');
		s->state = 0;
		putB(x->d, w + wFLAGS, EXECUTABLE);
		return 1;
	}
}

V do_number(S* s, C l, B* t) {
	X* x = (X*)s;
	B k;
	int n = 0;
	int i;
	if (*t == '-') {
		for (i = 1; i < l; i++) {
			k = *(t + i);
			if (k >= 48 && k <= 57) { n = n* 10 - (k - 48); }
			else { x->err = -13; return; }
		}
	} else {
		for (i = 0; i < l; i++) {
			k = *(t + i);
			if (k >= 48 && k <= 57) { n = n* 10 + (k - 48); }
			else { x->err = -13; return; }
		}
	}
	if (!s->state) PUSH(x, n);
	else literal(x, n);
}

V evaluate(S* s, B* str) {
	X* x = (X*)s;
	s->ibuf = x->d->data + str_to_transient(x->d, str, strlen(str));
	s->ilen = strlen(str);
	s->ipos = 0;
	while (!x->err) {
		parse_name(s);
		if (!T(x)) { DROP(x); DROP(x); return; }
		OVER(x); OVER(x);
		find_name(s);
		if (T(x)) {
			L3(x, C, w, C, l, B*, t);
			if (s->state == 0 || (getB(x->d, w + wFLAGS) & IMMEDIATE) == IMMEDIATE) do_interpret(s, w);
			else do_compile(s, w);
		} else {
			L3(x, C, w, C, l, B*, t);
			if (!do_asm(s, l, t))
				if (!do_casm(s, l, t))
					if (!do_colon(s, l, t))
						if (!do_semicolon(s, l, t))
							do_number(s, l, t);	
		}
	}
}

V sloth_ext(X* x) {
	S* s = (S*)x;
	switch (TOKEN(x)) {
	case '>': PUSH(x, &s->ipos); break;
	case 'c': header(s); break;
	case 'f': find_name(s); break;
	case 'l': PUSH(x, s->latest); break;
	case 'n': parse_name(s); break;
	case 's': PUSH(x, s->ibuf); PUSH(x, s->ilen); break;
	case 't': PUSH(x, x->d->data + x->d->there); break;
	}
}
	
#endif
