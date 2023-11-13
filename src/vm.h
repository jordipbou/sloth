#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

typedef void V;
typedef char B;
typedef int16_t W;
/*
typedef int32_t L;
typedef int64_t X;
*/
typedef intptr_t C;

#define ALIGNED(a) (a + (sizeof(C) - 1)) & ~(sizeof(C) - 1)

#define MSIZE 1024

typedef struct _State {
	C s[256];
	C sp;
	C r[256];
	C rp;
	C ip;
	C err;
	C tr;
  C st;	
	B m[MSIZE];
	B* d;
	V (**x)(struct _State*);
} S;

#define ABUF -MSIZE
#define IBUF ABUF + 64
#define IPOS IBUF + 256
#define ILEN IPOS + 1

#define IBUF_STR(s) ((B*)(s->m + 1024 - IBUF))

V inner(S*);

#define ABS_TO_REL(s, a) (((C)a) - ((C)s->d))
#define REL_TO_ABS(s, a) (((C)a) + ((C)s->d))

#define VARIABLE 128
#define HIDDEN 64 
#define IMMEDIATE 32

typedef struct _Name {
	W p;
	W c;
	B f;
	B n[1];
} N;

B get_at(S* s, C a) { return (a < 0) ? s->m[MSIZE + a] : *((B*)a); }
V put_at(S* s, C a, B v) { if (a < 0) s->m[MSIZE + a] = v; else *((B*)a) = v; }

W wget_at(S* s, C a) { return (a < 0) ? *((W*)&s->m[MSIZE + a]) : *((W*)a); }
V wput_at(S* s, C a, W v) { if (a < 0) *((W*)&s->m[MSIZE + a]) = v; else *((W*)a) = v; }

C cget_at(S* s, C a) { return (a < 0) ? *((C*)&s->m[MSIZE + a]) : *((W*)a); }
V cput_at(S* s, C a, C v) { if (a < 0) *((C*)&s->m[MSIZE + a]) = v; else *((W*)a) = v; }

#define SIZE(s) (*((C*)(s->d)))
#define HERE(s) (*((C*)(s->d + sizeof(C))))
#define LATEST(s) (*((W*)(s->d + sizeof(C))))

V put(S* s, B v) { put_at(s, HERE(s), v); HERE(s) += 1; }
V wput(S* s, W v) { wput_at(s, HERE(s), v); HERE(s) += 2; }
V cput(S* s, C v) { cput_at(s, HERE(s), v); HERE(s) += sizeof(C); }

#define T(s) (s->s[s->sp - 1])
#define N(s) (s->s[s->sp - 2])
#define NN(s) (s->s[s->sp - 3])

#define PUSH(s, v) (s->s[s->sp++] = (C)(v))
#define POP(s) (s->s[--s->sp])
#define DROP(s) (--s->sp)

#define L1(s, t, v) t v = (t)POP(s)
#define L2(s, t1, v1, t2, v2) L1(s, t1, v1); L1(s, t2, v2)
#define L3(s, t1, v1, t2, v2, t3, v3) L2(s, t1, v1, t2, v2); L1(s, t3, v3)
#define L4(s, t1, v1, t2, v2, t3, v3, t4, v4) L3(s, t1, v1, t2, v2, t3, v3); L1(s, t4, v4)

V duplicate(S* s) { s->sp++; T(s) = N(s); }
V over(S* s) { s->sp++; T(s) = NN(s); }
V swap(S* s) { C t = T(s); T(s) = N(s); N(s) = t; }
V rot(S* s) { C t = NN(s); NN(s) = N(s); N(s) = T(s); T(s) = t; }
V nip(S* s) { N(s) = T(s); s->sp--; }
V pick(S* s) { L1(s, C, v); PUSH(s, v); }

V to_r(S* s) { s->r[s->rp++] = s->s[--s->sp]; }
V from_r(S* s) { s->s[s->sp++] = s->r[--s->rp]; }
V fetch_r(S* s) { s->s[s->sp++] = s->r[s->rp - 1]; }

V add(S* s) { N(s) = N(s) + T(s); s->sp--; }
V sub(S* s) { N(s) = N(s) - T(s); s->sp--; }
V mul(S* s) { N(s) = N(s) * T(s); s->sp--; }
V division(S* s) { N(s) = N(s) / T(s); s->sp--; }
V mod(S* s) { N(s) = N(s) % T(s); s->sp--; }

V lt(S* s) { N(s) = (N(s) < T(s)) ? -1 : 0; s->sp--; }
V eq(S* s) { N(s) = (N(s) == T(s)) ? -1 : 0; s->sp--; }
V gt(S* s) { N(s) = (N(s) > T(s)) ? -1 : 0; s->sp--; }
V zeq(S* s) { T(s) = (T(s) == 0) ? -1 : 0; s->sp--; }

V and(S* s) { N(s) = N(s) & T(s); s->sp--; }
V or(S* s) { N(s) = N(s) | T(s); s->sp--; }
V xor(S* s) { N(s) = N(s) ^ T(s); s->sp--; }
V invert(S* s) { T(s) = ~T(s); }

C valid(S* s, C a) {
	if (!s->d) return a >= -MSIZE && a < 0;
	else return a >= -MSIZE && a < SIZE(s);
}

C tail(S* s) { return !valid(s, s->ip) || get_at(s, s->ip) == ']' || get_at(s, s->ip) == '}'; }
V execute(S* s) { L1(s, C, q); if (!tail(s)) s->r[s->rp++] = s->ip; s->ip = q; }
V ret(S* s) { if (s->rp > 0) s->ip = s->r[--s->rp]; else s->ip = (!s->d ? 0 : SIZE(s)); }
V eval(S* s, C q) { PUSH(s, q); execute(s); inner(s); }
V jump(S* s) { L1(s, C, d); s->ip += d; }
V zjump(S* s) { L2(s, C, d, C, b); if (!b) s->ip += d; }
V quotation(S* s) { L1(s, C, d); PUSH(s, s->ip); s->ip += d; }

V quit(S* s) { s->err = -256; }

V literal(S* s, C n) {
	if (n == 1) { put(s, '1'); }
	else if (n >= -128 && n <= 127) { put(s, '\''); put(s, n); }
	else if (n >= -32768 && n <= 32767) { put(s, '2'); wput(s, n); }
	else { put(s, '\\'); cput(s, n); }
}

B peek(S* s) { return get_at(s, s->ip); }
B token(S* s) { return get_at(s, s->ip++); }

V number(S* s) {
	C n = 0;
	B k;
	if (peek(s) == '-') {
		s->ip++;
		while (valid(s, s->ip) && (k = token(s)) >= 48 && k <= 57) { n = n*10 - (k - 48); }
	} else {
		while (valid(s, s->ip) && (k = token(s)) >= 48 && k <= 57) { n = n*10 + (k - 48); }
	}
	PUSH(s, n);
	s->ip--;
}

V string(S* s) {
	B k;
	PUSH(s, s->ip);
	while (valid(s, s->ip) && (k = token(s)) != '"') { }
	PUSH(s, s->ip - T(s) - 1);
}

V block(S* s) {
	int t = 1;
	B k;
	PUSH(s, s->ip);
	while (t > 0 && valid(s, s->ip)) {
	  switch (k = token(s)) {
		case '{': t++; break;
		case '}': t--; break;
		}
	}
}

V choice(S* s) { L3(s, C, f, C, t, C, b); if (b) eval(s, t); else eval(s, f); }
V times(S* s) { L2(s, C, q, C, n); while (n-- > 0) eval(s, q); }

V step(S* s) {
	switch (peek(s)) {
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
	case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
	case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'Y': 
	case 'Z':
		s->x[token(s) - 'A'](s);
		break;
	default:
		switch (token(s)) {
		case '$': put(s, token(s)); break;

		case '"': string(s); break;

		case 'l': { L1(s, C, n); literal(s, n); } break;
		case '#': number(s); break;

		case '1': PUSH(s, 1); break;
		case '\'': PUSH(s, get_at(s, s->ip)); s->ip += 1; break;
		case '2': PUSH(s, wget_at(s, s->ip)); s->ip += 2; break;
		case '\\': PUSH(s, cget_at(s, s->ip)); s->ip += sizeof(C); break;

		case '_': DROP(s); break;
		case 'd': duplicate(s); break;
		case 'o': over(s); break;
		case 's': swap(s); break;
		case 'r': rot(s); break;
		case 'n': nip(s); break;
		case 'p': pick(s); break;

		case '(': to_r(s); break;
		case ')': from_r(s); break;
		case 'f': fetch_r(s); break;

		case '+': add(s); break;
		case '-': sub(s); break;
		case '*': mul(s); break;
		case '/': division(s); break;
		case '%': mod(s); break;

		case '<': lt(s); break;
		case '=': eq(s); break;
		case '>': gt(s); break;
		case '0': zeq(s); break;

		case '&': and(s); break;
		case '|': or(s); break;
		case '^': xor(s); break;
		case '~': invert(s); break;

		case '!': { L2(s, C, a, C, v); cput_at(s, a, v); } break;
		case '@': { L1(s, C, a); PUSH(s, cget_at(s, a)); } break;
		case ';': { L2(s, C, a, W, v); wput_at(s, a, v); } break;
		case ':': { L1(s, C, a); PUSH(s, wget_at(s, a)); } break;
		case ',': { L2(s, C, a, B, v); put_at(s, a, v); } break;
		case '.': { L1(s, C, a); PUSH(s, get_at(s, a)); } break;

		case '[': quotation(s); break;
		case ']': ret(s); break;
		case '{': block(s); break;
		case '}': ret(s); break;
		case 'i': execute(s); break;
		case 'j': jump(s); break;
		case 'z': zjump(s); break;

		case '?': choice(s); break;
		case 't': times(s); break;

		case 'c': PUSH(s, sizeof(C)); break;
		case 'q': quit(s); break;
		case 'u': s->tr = 0; break;
		case 'v': s->tr = 1; break;
		}
		break;
	}
}

V dump_code(S* s, C c) {
	int t = 1;
	B k;
	while (t > 0 && valid(s, c)) {
		switch (k = get_at(s, c++)) {
		case '{': case '[': t++; printf("%c", k); break;
		case '}': case ']': t--; printf("%c", k); break;
		case '\'': printf("#%d", get_at(s, c++)); break;
		case '2': printf("#%d", wget_at(s, c)); c += 2; break;
		case '\\': printf("#%ld", cget_at(s, c)); c += sizeof(C); break;
		default: printf("%c", k); break;
		}
	}
}

V trace(S* s) {
	int i;
	printf("<%ld> ", s->sp);
	for (i = 0; i < s->sp; i++) { printf("%ld ", s->s[i]); }
	if (valid(s, s->ip)) { printf(" : "); dump_code(s, s->ip); }
	for (i = s->rp - 1; i >= 0; i--) { printf(" : "); dump_code(s, s->r[i]); }
}

V inner(S* s) {
	C t = s->rp;
	while (t <= s->rp && valid(s, s->ip)) {
		if (s->tr) { trace(s); printf("\n"); }
		step(s);
	}
}

V assembler(S* s, char* q) {
	C l = strlen(q);
	int i;
	for (i = 0; i < l; i++) { put_at(s, ABUF + i, *(q + i)); }
	put_at(s, ABUF + l - 1, ']');
	s->ip = ABUF;
	inner(s);
}

/* OUTER INTERPRETER */

#define WORD(s, w) ((N*)(s->d + (w << 2)))

V parse_name(S* s) {
	B i = get_at(s, IPOS);
	B l = get_at(s, ILEN);
	while (i < l && isspace(get_at(s, IBUF + i))) i++;
	PUSH(s, i);
	while (i < l && !isspace(get_at(s, IBUF + i))) i++;
	PUSH(s, i - T(s));
	put_at(s, IPOS, i);
}

B compare_without_case(S* s, N* w, C t, C l) {
	if ((w->f && 31) != l) return 0;
	else return strncmp(w->n, IBUF_STR(s) + t, l) == 0;
}

V find_name(S* s) {
	C l = POP(s);
	C t = POP(s);
	W w = LATEST(s);
	printf("FIND-NAME\n");
	while (w != -1) {
		printf("FIND-NAME:CURRENT WORD %d\n", w);
		if (compare_without_case(s, WORD(s, w), t, l)) break;
		w = WORD(s, w)->p;
	}
	PUSH(s, t);
	PUSH(s, l);
	PUSH(s, w);
}

B do_asm(S* s) {
	L2(s, C, l, C, t);
	printf("DO-ASM\n");
	if (l > 1 && get_at(s, IBUF + t) == '\\') {
		int i;
		printf("DO-ASM:CORRECT ASSEMBLER\n");
		l = l - 1;
		t = t + 1;
		for (i = 0; i < l; i++) { put_at(s, ABUF + i, get_at(s, IBUF + t + i)); }
		put_at(s, ABUF + l, ']');
		s->ip = ABUF;
		inner(s);
		return 1;
	} else {
		printf("DO-ASM:NO ASSEMBLER\n");
		PUSH(s, t);
		PUSH(s, l);
		return 0;
	}
}

B do_casm(S* s) {
	L2(s, C, l, C, t);
	printf("DO-CASM\n");
	if (l > 1 && get_at(s, IBUF + t) == '$') {
		int i;
		printf("DO-CASM:COMPILING ASSMEBLER\n");
		l = l - 1;
		t = t + 1;
		for (i = 0; i < l; i++) { put(s, get_at(s, IBUF + t + i)); }
		return 1;
	} else {
		printf("DO-CASM:NO COMPILATION\n");
		PUSH(s, t);
		PUSH(s, l);
		return 0;
	}
}

V align(S* s) {	HERE(s) = (HERE(s) + sizeof(C) - 1) & ~(sizeof(C) - 1); }

B do_colon(S* s) {
	C l = POP(s);
	C t = POP(s);
	printf("DO-COLON\n");
	if (l == 1 && get_at(s, IBUF + t) == ':') {
		printf("DO-COLON:CREATING COLON WORD\n");
	  parse_name(s);
		if (T(s) == 0) { DROP(s); DROP(s); s->err = -16; return 1; }
		else {
			int i;
			W w;
		  l = POP(s);
			t = POP(s);
			align(s);
			w = HERE(s) >> 2;
			wput(s, LATEST(s));
			LATEST(s) = w;
			wput(s, 0);
			put(s, HIDDEN & (B)(l));
			for (i = 0; i < l; i++) put(s, get_at(s, IBUF + t + i));
			put(s, 0);
			align(s);
			s->st = 1;
			WORD(s, w)->c = HERE(s) >> 2;
			return 1;
		}
	} else {
		printf("DO-COLON:NO COLON\n");
		PUSH(s, t);
		PUSH(s, l);
		return 0;
	}
}

B do_semicolon(S* s) {
	C l = POP(s);
	C t = POP(s);
	printf("DO-SEMICOLON\n");
	if (l == 1 && get_at(s, IBUF + t) == ';') {
		printf("DO-SEMICOLON:ENDING COLON WORD\n");
		put(s, ']');
		s->st = 0;
		WORD(s, LATEST(s))->f &= ~HIDDEN;
	} else {
		printf("DO-SEMICOLON:NO SEMICOLON\n");
		PUSH(s, t);
		PUSH(s, l);
		return 0;
	}
}

V do_number(S* s) {
	L2(s, C, l, C, t);
	B k;
	int n = 0;
	int i;
	printf("DO-NUMBER\n");
	if (get_at(s, IBUF + t) == '-') {
		printf("DO-NUMBER:NEGATIVE NUMBER\n");
		for (i = 1; i < l; i++) {
			k = get_at(s, IBUF + t + i);
			if (k >= 48 && k <= 57) { n = n* 10 - (k - 48); }
			else { printf("DO-NUMBER:UNDEFINED WORD\n"); s->err = -13; return; }
		}
		PUSH(s, n);
	} else {
		printf("DO-NUMBER:POSITIVE NUMBER\n");
		for (i = 0; i < l; i++) {
			k = get_at(s, IBUF + t + i);
			if (k >= 48 && k <= 57) { n = n* 10 + (k - 48); }
			else { printf("DO-NUMBER:UNDEFINED WORD\n"); s->err = -13; return; }
		}
		PUSH(s, n);
	}
}

V evaluate(S* s, B* str) {
	int i;
	put_at(s, IPOS, 0);
  put_at(s, ILEN, (B)strlen(str));
	for (i = 0; i < strlen(str); i++) put_at(s, IBUF + i, str[i]);
	printf("EVALUATE\n");
	while (get_at(s, IPOS) < get_at(s, ILEN)) {
		parse_name(s);
		if (!T(s)) { DROP(s); DROP(s); return; }
		find_name(s);
		if (T(s) != -1) {
		} else {
			DROP(s);
			if (!do_asm(s))
				if (!do_casm(s))
					if (!do_colon(s))
						if (!do_semicolon(s))
							do_number(s);
		}
	}
	printf("END EVALUATE\n");
}

S* init() {
	S* s = malloc(sizeof(S));
	s->x = malloc(26*sizeof(C));
	s->ip = 0;

	return s;
}

S* init_dict(S* s) {
	s->d = malloc(64 * 1024 * sizeof(C));
	SIZE(s) = 64 * 1024 * sizeof(C);
	HERE(s) = 3*sizeof(C);
	LATEST(s) = -1;
	s->ip = SIZE(s);
	return s;
}

/*
#define ABS_TO_REL(m, a) (((C)a) - ((C)m->d->b))
#define REL_TO_ABS(m, a) (((C)a) + ((C)m->d->b))

#define VARIABLE 1
#define HIDDEN 2
#define IMMEDIATE 4

typedef struct _Machine M;

typedef struct _Word {
	C p;
	C c;
	B f;
	B l;
	B n[1];
} W;

typedef struct _Dictionary {
	B* b;
	C h;
	C s;
	C l;
	C st;
	V (*x[26])(M*);
  B* ibuf;
  C ipos;
  C ilen;
} D;

#define EXT(m, l) (m->d->x[l - 'A'])

#define GET_AT(m, t, p) (*((t*)(m->d->b + p)))
#define PUT_AT(m, t, p, v) (*((t*)(m->d->b + p)) = (t)(v))
#define PUT(m, t, v) { *((t*)(m->d->b + m->d->h)) = v; m->d->h += sizeof(t); }

#define IBUF(m) (m->d->ibuf)
#define IPOS(m) (m->d->ipos)
#define ILEN(m) (m->d->ilen)

#define DSTACK_SIZE 256
#define RSTACK_SIZE 256

struct _Machine {
	C sp;
	C rp;
	C ip;
	C err;
	C tr;
	D* d;
	C s[DSTACK_SIZE];
	C r[RSTACK_SIZE];
};

V inner(M*);

#define PUSH(m, v) (m->s[m->sp++] = (C)(v))
#define POP(m) (m->s[--m->sp])
#define DROP(m) (--m->sp)

#define T(m) (m->s[m->sp - 1])
#define N(m) (m->s[m->sp - 2])
#define NN(m) (m->s[m->sp - 3])

#define L1(m, t, v) t v = (t)POP(m)
#define L2(m, t1, v1, t2, v2) L1(m, t1, v1); L1(m, t2, v2)
#define L3(m, t1, v1, t2, v2, t3, v3) L2(m, t1, v1, t2, v2); L1(m, t3, v3)
#define L4(m, t1, v1, t2, v2, t3, v3, t4, v4) L3(m, t1, v1, t2, v2, t3, v3); L1(m, t4, v4)

#define DO(m, f) { f(m); if (m->err) return; }
#define ERR(m, c, e) if (c) { m->err = e; return; }

V duplicate(M* m) { PUSH(m, T(m)); }
V over(M* m) { PUSH(m, N(m)); }
V swap(M* m) { C t = T(m); T(m) = N(m); N(m) = t; }
V rot(M* m) { C t = NN(m); NN(m) = N(m); N(m) = T(m); T(m) = t; }
V nip(M* m) { N(m) = T(m); DROP(m); }

V to_r(M* m) { m->r[m->rp++] = m->s[--m->sp]; }
V from_r(M* m) { m->s[m->sp++] = m->r[--m->rp]; }

V add(M* m) { N(m) = N(m) + T(m); DROP(m); }
V sub(M* m) { N(m) = N(m) - T(m); DROP(m); }
V mul(M* m) { N(m) = N(m) * T(m); DROP(m); }
V division(M* m) { N(m) = N(m) / T(m); DROP(m); }
V mod(M* m) { N(m) = N(m) % T(m); DROP(m); }

V and(M* m) { N(m) = N(m) & T(m); DROP(m); }
V or(M* m) { N(m) = N(m) | T(m); DROP(m); }
V xor(M* m) { N(m) = N(m) ^ T(m); DROP(m); }
V invert(M* m) { T(m) = ~T(m); }

V lt(M* m) { N(m) = (N(m) < T(m)) ? -1 : 0; DROP(m); }
V eq(M* m) { N(m) = (N(m) == T(m)) ? -1 : 0; DROP(m); }
V gt(M* m) { N(m) = (N(m) > T(m)) ? -1 : 0; DROP(m); }

V cstore(M* m) { L2(m, C*, a, C, b); *a = b; }
V cfetch(M* m) { L1(m, C*, a); PUSH(m, *a); }
V bstore(M* m) { L2(m, B*, a, B, b); *a = b; }
V bfetch(M* m) { L1(m, B*, a); PUSH(m, *a); }
V sstore(M* m) { L2(m, S*, a, S, b); *a = b; }
V sfetch(M* m) { L1(m, S*, a); PUSH(m, *a); }
V istore(M* m) { L2(m, I*, a, I, b); *a = b; }
V ifetch(M* m) { L1(m, I*, a); PUSH(m, *a); }
                
V reset(M* m) { m->ip = m->d->s; m->rp = 0; m->sp = 0; m->err = 0; }

#define PEEK(m) (GET_AT(m, B, m->ip))
#define TOKEN(m) (GET_AT(m, B, m->ip++))
#define IN(m, p) (p >= 0 && p < m->d->s)
#define TAIL(m) (!(IN(m, m->ip)) || PEEK(m) == ']' || PEEK(m) =='}')

V execute(M* m) { L1(m, C, q); if (!TAIL(m)) { m->r[m->rp++] = m->ip; } m->ip = q; }
V ret(M* m) { m->ip = (m->rp > 0) ? m->r[--m->rp] : m->d->s; }

V eval(M* m, C q) { PUSH(m, q); execute(m); inner(m); }

V jump(M* m) { S d = *((S*)(m->d->b + m->ip)); m->ip += d + 2; }
V zjump(M* m) { L1(m, C, b); S d = *((S*)(m->d->b + m->ip)); m->ip += 2; if (!b) m->ip += d; }
V quotation(M* m) { S d; PUSH(m, m->ip + 2); d = *((S*)(m->d->b + m->ip)); m->ip += d + 2; }

#define BLOCK(m, ip) { \
	C t = 1; \
	while (t && IN(m, ip)) { \
		switch (GET_AT(m, B, ip++)) { \
		case '{': case '[': t++; break; \
		case '}': case ']': t--; break; \
		} \
	} \
}

V block(M* m) { C t = 1; PUSH(m, m->ip); BLOCK(m, m->ip); }

V dc(M* m, C ip) { 
	C t = ip;
	B c;
	BLOCK(m, ip);
	printf(" : ");
	for(;t <= ip; t++) { 
		c = GET_AT(m, B, t);
		if (c == '\'') { printf("#%d", GET_AT(m, B, t + 1)); t += 1; }
		else if (c == '2') { printf("#%d", GET_AT(m, S, t + 1)); t += 2; }
		else if (c == '4') { printf("#%d", GET_AT(m, I, t + 1)); t += 4; }
		else if (c == '8') { printf("#%ld", GET_AT(m, L, t + 1)); t += 8; }
		else if (c != 10) printf("%c", c); 
	}
}
V ds(M* m) { C i; for(i = 0; i < m->sp; i++) { printf("%ld ", m->s[i]); } }
V dr(M* m) { C i; dc(m, m->ip); for(i = 0; i < m->rp; i++) { dc(m, m->r[i]); } }
V trace(M* m) { ds(m); dr(m); }

V step(M* m) {
	switch (PEEK(m)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
  case 'E': case 'K':
		EXT(m, TOKEN(m))(m);
		break;
	default:
		switch (TOKEN(m)) {
		case '0': PUSH(m, 0); break;
		case '1': PUSH(m, 1); break;
		case '\'': PUSH(m, *((B*)(m->d->b + m->ip))); m->ip += sizeof(B); break;
		case '2': PUSH(m, *((S*)(m->d->b + m->ip))); m->ip += sizeof(S); break;
		case '4': PUSH(m, *((I*)(m->d->b + m->ip))); m->ip += sizeof(I); break;
		case '8': PUSH(m, *((L*)(m->d->b + m->ip))); m->ip += sizeof(L); break;

		case '_': DROP(m); break;
		case 'd': duplicate(m); break;
		case 'o': over(m); break;
		case 's': swap(m); break;
		case 'r': rot(m); break;
		case 'n': nip(m); break;

		case '(': to_r(m); break;
		case ')': from_r(m); break;

		case '+': add(m); break;
		case '-': sub(m); break;
		case '*': mul(m); break;
		case '/': division(m); break;
		case '%': mod(m); break;

		case '&': and(m); break;
		case '|': or(m); break;
		case '^': xor(m); break;
		case '~': invert(m); break;

		case '<': lt(m); break;
		case '=': eq(m); break;
		case '>': gt(m); break;

    case '!': cstore(m); break;
    case '@': cfetch(m); break;
		case ',': istore(m); break;
		case '.': ifetch(m); break;
		case ';': bstore(m); break;
		case ':': bfetch(m); break;
    case '\'': sstore(m); break;
    case '"': sfetch(m); break;

		case '[': quotation(m); break;
		case '{': block(m); break;
		case '}': case ']': ret(m); break;
		case 'x': execute(m); break;
		case 'j': jump(m); break;
		case 'z': zjump(m); break;

		case 'c': PUSH(m, sizeof(C)); break;

		case 'b': PUSH(m, m); break;

		case 'l': PUSH(m, REL_TO_ABS(m, m->d->l)); break;

		case 'u': m->tr = 0; break;
		case 'v': m->tr = 1; break;
		}
	}
}

V inner(M* m) {
	C t = m->rp;
	while (m->rp >= t && IN(m, m->ip)) {
		if (m->tr) {
			trace(m);
			printf("\n");
		}
		step(m);
	}
}

V isolated(M* m, char* s) {
	m->d->b = (B*)s;
	m->d->s = strlen(s);
	m->ip = 0;
	inner(m);
}

V align(M* m) { m->d->h = ALIGNED(m->d->h); }

V parse_name(M* m) {
	while (IPOS(m) < ILEN(m) && isspace(IBUF(m)[IPOS(m)])) IPOS(m)++; 
	PUSH(m, IBUF(m) + IPOS(m));
	while (IPOS(m) < ILEN(m) && !isspace(IBUF(m)[IPOS(m)])) IPOS(m)++; 
	PUSH(m, (IBUF(m) + IPOS(m)) - T(m));
}

V find_name(M* m) {
	L2(m, C, l, B*, t);
	C wp = m->d->l;
	while (wp != -1) {
		W* w = (W*)REL_TO_ABS(m, wp);
		if (w->l == l && !strncmp(w->n, t, l) && (w->f & HIDDEN) != HIDDEN) break;
		wp = w->p;
	}
	PUSH(m, t);
	PUSH(m, l);
	PUSH(m, wp);
}

V create(M* m) {
	parse_name(m);
	{
		L2(m, C, l, B*, t);
		W* w;
		align(m);
		w = (W*)REL_TO_ABS(m, m->d->h);
		w->p = m->d->l;
		m->d->l = ABS_TO_REL(m, w);
		w->f = HIDDEN;
		w->l = l;
		strncpy(w->n, t, l);
		m->d->h += sizeof(W) + l - 1;
		align(m);
		w->c = m->d->h;
	}
}

C literal_size(C n) {
	if (n >= INT8_MIN && n <= INT8_MAX) { return 1; }
	else if (n >= INT16_MIN && n <= INT16_MAX) { return 2; }
	else if (n >= INT32_MIN && n <= INT32_MAX) { return 4; }
	else { return 8; }
}

V literal(M* m) {
	L1(m, C, n);
	if (n >= INT8_MIN && n <= INT8_MAX) {
		PUT(m, B, '\'');
		PUT(m, B, n);
	} else if (n >= INT16_MIN && n <= INT16_MAX) {
		PUT(m, B, '2');
		PUT(m, S, n);
	} else if (n >= INT32_MIN && n <= INT32_MAX) {
		PUT(m, B, '4');
		PUT(m, I, n);
	} else {
		PUT(m, B, '8');
		PUT(m, L, n);
	}
}

V evaluate(M* m, char* s) {
  IBUF(m) = (B*)s;
  ILEN(m) = strlen(s);
  IPOS(m) = 0;
  while (IPOS(m) < ILEN(m)) {
    parse_name(m);
    if (!T(m)) { DROP(m); DROP(m); return; }
    find_name(m);
    {
      L3(m, C, wp, C, l, B*, t);
			if (wp != -1) {
				W* w = (W*)REL_TO_ABS(m, wp);
				if (!m->d->st || (w->f & IMMEDIATE) == IMMEDIATE) {
					eval(m, w->c);
				} else {
					C c = w->c;
					C l;
					BLOCK(m, c);
					l = c - w->c;
					if (l < (literal_size(w->c) + 2)) {
						for (c = 0; c < l - 1; c++) PUT(m, B, GET_AT(m, B, w->c + c));
					} else {
						PUSH(m, w->c);
						literal(m);
						PUT(m, B, 'x');
					}
				}
    	} else {
				if (l == 1 && *t == ':') {
					create(m);
					((W*)(REL_TO_ABS(m, m->d->l)))->f = HIDDEN;
					m->d->st = 1;
				} else if (l == 1 && *t == ';') {
					PUT(m, B, ']');
					align(m);
					((W*)(REL_TO_ABS(m, m->d->l)))->f = 0;
					m->d->st = 0;
				} else if (*t == '\\') {
					strncpy((char*)(m->d->b + m->d->s - l), (char*)(t + 1), l - 1);
					PUSH(m, m->d->s - l);
					execute(m);
					inner(m);
				} else if (*t == '$') {
					strncpy((char*)(m->d->b + m->d->h), (char*)(t + 1), l - 1);
					m->d->h += l - 1;
				} else {
					char * end;
					int n = strtol((char*)t, &end, 10);
					if ((n == 0 && end == (char*)t) || end < ((char*)t + l)) {
						PUSH(m, t);
						PUSH(m, l);
						m->err = -13;
						return;
					}
					PUSH(m, n);
					if (m->d->st) literal(m);
				}
    	}
		}
  }
}
                
D* init_DICT(int block_size) {
	D* d = malloc(sizeof(D));
	if (!d) return 0;
	if (block_size) {
		d->b = malloc(block_size);
    if (!d->b) { free(d); return 0; }
		d->l = -1;
		d->h = 0;
		d->s = block_size;
	} else {
		d->b = 0;
		d->s = 0;
		d->h = 0;
		d->l = 0;
	}

	return d;
}

M* init_VM(D* d) {
	M* m;

	if (!d) return 0;
	m = malloc(sizeof(M));
	if (!m) return 0;
	m->d = d;

	reset(m);

	return m;
}
*/
/*
#define VARIABLE 32
#define HIDDEN 64 
#define IMMEDIATE 128 

#define FLAGS(w) (*((B*)(w + 2)))
#define SET_VARIABLE(w)	(FLAGS(w) = FLAGS(w) | VARIABLE)
#define SET_HIDDEN(w) (FLAGS(w) = FLAGS(w) | HIDDEN)
#define SET_IMMEDIATE(w) (FLAGS(w) = FLAGS(w) | IMMEDIATE)
#define UNSET_VARIABLE(w) (FLAGS(w) = FLAGS(w) & ~VARIABLE)
#define UNSET_HIDDEN(w) (FLAGS(w) = FLAGS(w) & ~HIDDEN)
#define UNSET_IMMEDIATE(w) (FLAGS(w) = FLAGS(w) & ~IMMEDIATE)
#define IS_IMMEDIATE(w) ((FLAGS(w) & IMMEDIATE) == IMMEDIATE)
#define IS_HIDDEN(w) ((FLAGS(w) & HIDDEN) == HIDDEN)
#define IS_VARIABLE(w) ((FLAGS(w) & VARIABLE) == VARIABLE)
#define NAME_LENGTH(w) (FLAGS(w) & 0x1F)
#define NAME(w) (w + 3)

#define CODE(w) (ALIGN(w + NAME_LENGTH(w) + 1))

#define GET_AT(m, t, p) (*((t*)(m->s->b + p)))
#define PUT_AT(m, t, p, v) (*((t*)(m->s->b + p)) = (t)(v))

#define HERE(m) (*((C*)(m->s->b)))
#define SIZE(m) (*((C*)(m->s->b + sizeof(C))))
#define LATEST(m) (*((C*)(m->s->b + 2*sizeof(C))))

#define COMPILE(m, t, v) { *((t*)(m->s->b + HERE(m))) = (t)(v); HERE(m) = HERE(m) + sizeof(t); }

#define ABUF(m) (m->s->b + SIZE(m) - 64)
#define IBUF(m) (ABUF(m) - 256)

typedef struct _Machine M; 

typedef struct _Dictionary {
	V (*x[26])(M*);
	B* b;
	C s;
} SYS;

#define EXT(m, l) (m->s->x[l - 'A'])

#define DSTACK_SIZE 256
#define RSTACK_SIZE 256

struct _Machine {
  C d[DSTACK_SIZE];
	C dp;
	C r[RSTACK_SIZE];
	C rp;
	C ip;
	C err;
	struct _System* s;
};

V inner(M*);

#define PUSH(m, v) (m->d[m->dp++] = (C)(v))
#define POP(m) (m->d[--m->dp])
#define DROP(m) (--m->dp)

#define T(m) (m->d[m->dp - 1])
#define N(m) (m->d[m->dp - 2])
#define NN(m) (m->d[m->dp - 3])

#define L1(m, t, v) t v = (t)POP(m)
#define L2(m, t1, v1, t2, v2) L1(m, t1, v1); L1(m, t2, v2)
#define L3(m, t1, v1, t2, v2, t3, v3) L2(m, t1, v1, t2, v2); L1(m, t3, v3)
#define L4(m, t1, v1, t2, v2, t3, v3, t4, v4) L3(m, t1, v1, t2, v2, t3, v3); L1(m, t4, v4)

#define DO(m, f) { f(m); if (m->err) return; }
#define ERR(m, c, e) if (c) { m->err = e; return; }

V duplicate(M* m) { PUSH(m, T(m)); }
V over(M* m) { PUSH(m, N(m)); }
V swap(M* m) { C t = T(m); T(m) = N(m); N(m) = t; }
V rot(M* m) { C t = NN(m); NN(m) = N(m); N(m) = T(m); T(m) = t; }
V nip(M* m) { N(m) = T(m); DROP(m); }

V to_r(M* m) { m->r[m->rp++] = m->d[--m->dp]; }
V from_r(M* m) { m->d[m->dp++] = m->r[--m->rp]; }

V add(M* m) { N(m) = N(m) + T(m); DROP(m); }
V sub(M* m) { N(m) = N(m) - T(m); DROP(m); }
V mul(M* m) { N(m) = N(m) * T(m); DROP(m); }
V division(M* m) { N(m) = N(m) / T(m); DROP(m); }
V mod(M* m) { N(m) = N(m) % T(m); DROP(m); }

V and(M* m) { N(m) = N(m) & T(m); DROP(m); }
V or(M* m) { N(m) = N(m) | T(m); DROP(m); }
V xor(M* m) { N(m) = N(m) ^ T(m); DROP(m); }
V not(M* m) { T(m) = !T(m); }
V invert(M* m) { T(m) = ~T(m); }

V lt(M* m) { N(m) = (N(m) < T(m)) ? -1 : 0; DROP(m); }
V eq(M* m) { N(m) = (N(m) == T(m)) ? -1 : 0; DROP(m); }
V gt(M* m) { N(m) = (N(m) > T(m)) ? -1 : 0; DROP(m); }

V pstore(M* m) { L2(m, C*, a, C, b); *a = b; }
V pfetch(M* m) { L1(m, C*, a); PUSH(m, *a); }
V bstore(M* m) { L2(m, B*, a, B, b); *a = b; }
V bfetch(M* m) { L1(m, B*, a); PUSH(m, *a); }

#define TAIL(m) (m->ip >= (SIZE(m)) || m->s->b[m->ip] == ']')
V call(M* m) { L1(m, C, q); if (!TAIL(m)) { m->r[m->rp++] = m->ip; } m->ip = q; }
V ret(M* m) { m->ip = (m->rp > 0) ? m->r[--m->rp] : 0; }
V jump(M* m) { L1(m, C, d); m->ip += d - 1; }
V zjump(M* m) { L2(m, C, d, C, b); if (!b) m->ip += d - 1; }
V quot(M* m) { L1(m, C, d); PUSH(m, m->ip); m->ip += d; }
V eval(M* m, B* q) { PUSH(m, q); call(m); inner(m); }

#define PEEK(x) (m->s->b[m->ip])
#define TOKEN(x) (m->s->b[m->ip++])

V step(M* m) {
	switch (PEEK(m)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
  case 'E': case 'K':
		EXT(m, TOKEN(m))(m);
		break;
	default:
		switch (TOKEN(m)) {
		case '0': PUSH(m, 0); break;
		case '1': PUSH(m, 1); break;

		case '_': DROP(m); break;
		case 'd': duplicate(m); break;
		case 'o': over(m); break;
		case 's': swap(m); break;
		case 'r': rot(m); break;
		case 'n': nip(m); break;

		case '(': to_r(m); break;
		case ')': from_r(m); break;

		case '+': add(m); break;
		case '-': sub(m); break;
		case '*': mul(m); break;
		case '/': division(m); break;
		case '%': mod(m); break;

		case '&': and(m); break;
		case '|': or(m); break;
		case '^': xor(m); break;
		case '!': not(m); break;
		case '~': invert(m); break;

		case '<': lt(m); break;
		case '=': eq(m); break;
		case '>': gt(m); break;

		case ',': pstore(m); break;
		case '.': pfetch(m); break;
		case ';': bstore(m); break;
		case ':': bfetch(m); break;

		case 'x': call(m); break;
		case 'j': jump(m); break;
		case 'z': zjump(m); break;
		case '[': quot(m); break;
		case ']': ret(m); break;

		case 'c': PUSH(m, sizeof(C)); break;
		case '@': PUSH(m, m); break;
		}
	}
}

V inner(M* m) { 
	C rp = m->rp; 
	while(m->rp >= rp && !m->err && m->ip < SIZE(m)) { 
		step(m); 
	} 
)

V evaluate(M* m, B* s) {
  
}

V reset(M* m) {
  m->ip = SIZE(m);
  m->dp = 0;
  m->rp = 0;
}
                
M* init() {
  M* m = malloc(sizeof(M));
  m->s = malloc(sizeof(SYS));
  m->s->b = malloc(65536);
  SIZE(m) = 65536;
  LATEST(m) = 0;
  HERE(m) = 3*sizeof(C);
  reset(m);
  return m;
}
*/
/*

typedef struct _Machine M;

#define DICT_SIZE 65536
#define ASM_BUFFER_SIZE 80

typedef struct _System {
	C hs;
	C hp;
	N* l;
	V (*x[26])(M*);
	B h[DICT_SIZE];
} S;

#define EXT(m, l) (m->s->x[l - 'A'])

#define cbyte(s, b) (s)->h[(s)->hp++] = (b)
#define cword(s, w) *((W*)((s)->h + (s)->hp)) = (w); (s)->hp += 2
#define clong(s, l) *((L*)((s)->h + (s)->hp)) = (l); (s)->hp += 4
#if defined (BITS_64)
#define cextra(s, x) { *((X*)((s)->h + (s)->hp)) = (x); (s)->hp += 8; }
#define ccell(s, n) cextra(s, n)
#else
#define ccell(s, n) clong(s, n)
#endif

#define DSTACK_SIZE 64 
#define RSTACK_SIZE 64

struct _Machine {
	C dp;
	C rp;
	B* ip;
	C err;
	C st;
	S* s;
	char* ibuf;
	C ilen;
	C ipos;
  C d[DSTACK_SIZE];
	C r[RSTACK_SIZE];
};

V inner(M*);

#define PUSH(m, v) (m->d[m->dp++] = (C)(v))
#define POP(m) (m->d[--m->dp])
#define DROP(m) (--m->dp)

#define T(m) (m->d[m->dp - 1])
#define N(m) (m->d[m->dp - 2])
#define NN(m) (m->d[m->dp - 3])

#define L1(m, t, v) t v = (t)POP(m)
#define L2(m, t1, v1, t2, v2) L1(m, t1, v1); L1(m, t2, v2)
#define L3(m, t1, v1, t2, v2, t3, v3) L2(m, t1, v1, t2, v2); L1(m, t3, v3)
#define L4(m, t1, v1, t2, v2, t3, v3, t4, v4) L3(m, t1, v1, t2, v2, t3, v3); L1(m, t4, v4)

#define DO(m, f) { f(m); if (m->err) return; }
#define ERR(m, c, e) if (c) { m->err = e; return; }

V duplicate(M* m) { PUSH(m, T(m)); }
V over(M* m) { PUSH(m, N(m)); }
V swap(M* m) { C t = T(m); T(m) = N(m); N(m) = t; }
V rot(M* m) { C t = NN(m); NN(m) = N(m); N(m) = T(m); T(m) = t; }
V nip(M* m) { N(m) = T(m); DROP(m); }

V to_r(M* m) { m->r[m->rp++] = m->d[--m->dp]; }
V from_r(M* m) { m->d[m->dp++] = m->r[--m->rp]; }

V add(M* m) { N(m) = N(m) + T(m); DROP(m); }
V sub(M* m) { N(m) = N(m) - T(m); DROP(m); }
V mul(M* m) { N(m) = N(m) * T(m); DROP(m); }
V division(M* m) { N(m) = N(m) / T(m); DROP(m); }
V mod(M* m) { N(m) = N(m) % T(m); DROP(m); }

V and(M* m) { N(m) = N(m) & T(m); DROP(m); }
V or(M* m) { N(m) = N(m) | T(m); DROP(m); }
V xor(M* m) { N(m) = N(m) ^ T(m); DROP(m); }
V not(M* m) { T(m) = !T(m); }
V invert(M* m) { T(m) = ~T(m); }

V lt(M* m) { N(m) = (N(m) < T(m)) ? -1 : 0; DROP(m); }
V eq(M* m) { N(m) = (N(m) == T(m)) ? -1 : 0; DROP(m); }
V gt(M* m) { N(m) = (N(m) > T(m)) ? -1 : 0; DROP(m); }

V pstore(M* m) { L2(m, C*, a, C, b); *a = b; }
V pfetch(M* m) { L1(m, C*, a); PUSH(m, *a); }
V bstore(M* m) { L2(m, B*, a, B, b); *a = b; }
V bfetch(M* m) { L1(m, B*, a); PUSH(m, *a); }

V literal(M* m) {
	L1(m, C, n);
	if (n >= INT8_MIN && n <= INT8_MAX) { cbyte(m->s, 'b'); cbyte(m->s, (B)n); }
	else if (n >= INT16_MIN && n <= INT16_MAX) { cbyte(m->s, 'w'); cword(m->s, (W)n); }
	else if (n >= INT32_MIN && n <= INT32_MAX) { cbyte(m->s, 'l'); clong(m->s, (L)n); }
#if defined (BITS_64)
	else { cbyte(m->s, 'x'); cextra(m->s, (X)n); }
#endif
}

V compile(M* m) {
}

V interpret(M* m) {
}

V call(M* m) { L1(m, B*, q); if (*q != ']') m->r[m->rp++] = (C)m->ip; m->ip = q; }
V ret(M* m) { m->ip = (m->rp > 0) ? (B*)m->r[--m->rp] : 0; }
V jump(M* m) { L1(m, C, d); m->ip += d - 1; }
V zjump(M* m) { L2(m, C, d, C, b); if (!b) m->ip += d - 1; }
V quot(M* m) { L1(m, C, d); PUSH(m, m->ip); m->ip += d; }
V eval(M* m, B* q) { PUSH(m, q); call(m); inner(m); }

V align(M* m) {
#if defined (BITS_64)
	m->s->hp = (m->s->hp + 7) & 7;
#else
	m->s->hp = (m->s->hp + 3) & 3;
#endif
}

V parse_spaces(M* m) { while (m->ipos < m->ilen && isspace(m->ibuf[m->ipos]))	m->ipos++; }
V parse(M* m) { L1(m, C, c); while (m->ipos < m->ilen && m->ibuf[m->ipos] != c) m->ipos++; m->ipos++; }
V parse_non_spaces(M* m) { while (m->ipos < m->ilen && !isspace(m->ibuf[m->ipos])) m->ipos++; }
V parse_name(M* m) {
	parse_spaces(m);
	PUSH(m, m->ibuf + m->ipos);
	parse_non_spaces(m);
	PUSH(m, (m->ibuf + m->ipos) - T(m));
}

V colon(M* m) {
	parse_name(m);
	{
		L2(m, C, l, B*, t);
		N* n = (N*)(m->s->h + m->s->hp);
		printf("Creating new word at %ld\n", n);
		printf("m->s->l %ld\n", m->s->l);
		n->p = m->s->l;
		m->s->l = n;
		n->f = HIDDEN;
		n->l = l;
		strncpy((char*)n->n, (char*)t, l);
		n->n[l] = 0;
		align(m);
		n->c = m->s->h + m->s->hp;
		m->st = 1;
	}
}

V semicolon(M* m) {
	m->s->l->f &= ~HIDDEN;
	m->st = 0;
}

#define PEEK(x) (*m->ip)
#define TOKEN(x) (*m->ip++)

V step(M* m) {
	switch (PEEK(m)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
  case 'E': case 'K':
		EXT(m, TOKEN(m))(m);
		break;
	default:
		switch (TOKEN(m)) {
		case '0': PUSH(m, 0); break;
		case '1': PUSH(m, 1); break;
		case 'b': PUSH(m, TOKEN(x)); break;
		case 'w': PUSH(m, *((W*)m->ip)); m->ip += 2; break;
		case 'l': PUSH(m, *((L*)m->ip)); m->ip += 4; break;
#if defined (BITS_64)
		case 'x': PUSH(m, *((X*)m->ip)); m->ip += 8; break;
#endif

		case 'h': colon(m); break;
		case 'v': semicolon(m); break;

		case '_': DROP(m); break;
		case 'd': duplicate(m); break;
		case 'o': over(m); break;
		case 's': swap(m); break;
		case 'r': rot(m); break;
		case 'n': nip(m); break;

		case '(': to_r(m); break;
		case ')': from_r(m); break;

		case '+': add(m); break;
		case '-': sub(m); break;
		case '*': mul(m); break;
		case '/': division(m); break;
		case '%': mod(m); break;

		case '&': and(m); break;
		case '|': or(m); break;
		case '^': xor(m); break;
		case '!': not(m); break;
		case '~': invert(m); break;

		case '<': lt(m); break;
		case '=': eq(m); break;
		case '>': gt(m); break;

		case ',': pstore(m); break;
		case '.': pfetch(m); break;
		case ';': bstore(m); break;
		case ':': bfetch(m); break;

		case 'e': call(m); break;
		case 'j': jump(m); break;
		case 'z': zjump(m); break;
		case '[': quot(m); break;
		case ']': ret(m); break;

		case 'c': PUSH(m, sizeof(C)); break;
		case '@': PUSH(m, m); break;
		}
	}
}

V inner(M* m) { 
	C rp = m->rp; 
	while(m->rp >= rp && m->ip && !m->err) { 
		step(m); 
	} 
}

V reset_context(M* m) {
	m->dp = 0;
	m->rp = 0;
	m->ip = 0;
	m->err = 0;
}

M* init() {
	M* m = malloc(sizeof(M));
	m->s = malloc(sizeof(S));
	m->s->l = 0;
	reset_context(m);

	return m;
}

V find_name(M* m) { 
	L2(m, C, l, B*, t);
	N* n = m->s->l;
	while (n) {
		printf("Searching for name [%.*s] with name %ld [%.*s]\n", (int)l, t, n, (int)n->l, n->n);
		if (n->l == l && !strncmp((char*)n->n, (char*)t, l)) break;
		n = n->p;
		printf("n is now: %ld\n", n);
	}
	printf("After while...\n");
	PUSH(m, t);
	PUSH(m, l);
	PUSH(m, n);
}

V asm_comp(M* m, C l, char* t) { 
	C i;
	printf("Compiling assembler l: %ld\n", l);
	for (i = 0;i < l; i++) {
		cbyte(m->s, t[i]);
	}
}

V to_number(M* m, C l, char* t) {
	char * end;
	int n = strtol(t, &end, 10);
	if ((n == 0 && end == t) || end < (t + l)) {
		PUSH(m, t);
		PUSH(m, l);
		m->err = -13;
		return;
	}
	PUSH(m, n);
}

V evaluate(M* m, char* s) {
	m->ibuf = s;
	m->ilen = strlen(s);
	m->ipos = 0;
	while (!m->err && m->ipos < m->ilen) {
		printf("m->ipos %ld m->ilen %d\n", m->ipos, m->ilen);
		parse_name(m);
		printf("...m->ipos %ld m->ilen %d\n", m->ipos, m->ilen);
		printf("Stack depth: %ld\n", m->dp);
		printf("T(m) %ld N(m) %ld\n", T(m), N(m));
		if (!T(m)) { DROP(m); DROP(m); return; }
		printf("Pre find name\n");
		find_name(m);
		printf("....Stack depth: %ld\n", m->dp);
		printf("T(m) %ld N(m) %ld NN(m) %ld\n", T(m), N(m), NN(m));
		if (T(m)) {
			L3(m, N*, nt, C, _, char*, __);
			if (!m->st || (nt->f & IMMEDIATE) == IMMEDIATE) eval(m, nt->c);
			else { PUSH(m, nt->c); compile(m); }
		} else {
			L3(m, N*, _, C, l, char*, t);
			if (t[0] == '\\' && t[l - 1] == ']') eval(m, (B*)(t + 1)); 
			else if (t[0] == '$') asm_comp(m, l - 1, t + 1);
			else to_number(m, l, t);
		}
	}
}
*/
/*
V times(X*P* p) { L2(x, C, q, C, n); for (;n > 0; n--) eval(x, q); }
V choose(X*P* p) { L3(x, C, f, C, t, C, b); if (b) eval(x, t); else eval(x, f); }

V block(X*P* p) {
	C t = 1;
	PUSH(x,P* p->ip);
	while (x->ip < DICT_SIZE && t) {
		switch (TOKEN(x)) {
		case '{': t++; break;
		case '}': t--; break;
		}
	}
}

V string(X*P* p) {
	PUSH(x,P* p->m->h +P* p->ip);
	while (x->ip < DICT_SIZE && TOKEN(x) != '"') {}
	PUSH(x, (x->m->h +P* p->ip) - T(x) - 1);
}

V parse_spaces(X*P* p) { while (x->ipos <P* p->ilen && isspace(x->ibuf[x->ipos]))	x->ipos++; }
V parse(X*P* p) { L1(x, C, c); while (x->ipos <P* p->ilen &&P* p->ibuf[x->ipos] != c)P* p->ipos++;P* p->ipos++; }
V parse_non_spaces(X*P* p) { while (x->ipos <P* p->ilen && !isspace(x->ibuf[x->ipos]))P* p->ipos++; }
V parse_name(X*P* p) {
	parse_spaces(x);
	PUSH(x,P* p->ibuf +P* p->ipos);
	parse_non_spaces(x);
	PUSH(x, (x->ibuf +P* p->ipos) - T(x));
}

V find_name(X*P* p) { PUSH(x, 0); }

V asm_comp(X*P* p, C l, B* t) { }

V to_number(X*P* p, C l, B* t) {
	char * end;
	int n = strtol(t, &end, 10);
	if ((n == 0 && end == t) || end < (t + l)) {
		PUSH(x, t);
		PUSH(x, l);
		x->err = -13;
		return;
	}
	PUSH(x, n);
}

V evaluate(X*P* p, B* s) {
	x->ibuf = s;
	x->ilen = strlen(s);
	x->ipos = 0;
	while (!x->err &&P* p->ipos <P* p->ilen) {
		parse_name(x);
		if (!T(x)) { DROP(x); DROP(x); return; }
		find_name(x);
		if (T(x)) {
			L3(x, C, wp, C, _, B*, __);
		} else {
			L3(x, C, _, C, l, B*, t);			
			if (t[0] == '\\') asm_exec(x, l - 1, t + 1);
			else if (t[0] == '$') asm_comp(x, l - 1, t + 1);
			else to_number(x, l, t);
		}
	}
}
*/

#endif
