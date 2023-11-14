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

#define IBUF_STR(s) ((B*)(s->m + (1024 + IBUF)))

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
#define LATEST(s) (*((W*)(s->d + 2*sizeof(C))))

V put(S* s, B v) { put_at(s, REL_TO_ABS(s, HERE(s)), v); HERE(s) += 1; }
V wput(S* s, W v) { printf("WPUT:COMPILING %d AT %d\n", v, REL_TO_ABS(s, HERE(s))); wput_at(s, REL_TO_ABS(s, HERE(s)), v); HERE(s) += 2; printf("/// WPUT\n"); }
V cput(S* s, C v) { cput_at(s, REL_TO_ABS(s, HERE(s)), v); HERE(s) += sizeof(C); }

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
	else return (a >= -MSIZE && a < 0) || (a > s->d && a < (s->d + SIZE(s)));
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
	printf("STEP:IP %d\n", s->ip);
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

		case 'b': PUSH(s, s->d); break;
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

B is_immediate(S* s, N* w) { return (w->f & IMMEDIATE) == IMMEDIATE; }

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
	int i;
	printf("COMPARE-WITHOUT-CASE:TOKEN-LENGTH %d WORD-LENGTH %d WORD-FLAGS %d\n", l, (w->f & 31), w->f);
	printf("COMPARING TOKEN [%.*s] WITH WORD [%.*s]\n", l, IBUF_STR(s) + t, (w->f & 31), w->n);
	if ((w->f & 31) != l) return 0;
	else return strncmp(w->n, IBUF_STR(s) + t, l) == 0;
}

V find_name(S* s) {
	int i;
	C l = POP(s);
	C t = POP(s);
	W w = LATEST(s);
	printf("FIND-NAME\n");
	printf("FIND-NAME:TOKEN [%.*s]\n", l, IBUF_STR(s) + t);
	printf("FIND-NAME:TOKEN WITHOUT IBUF_STR [");
	for (i = 0; i < l; i++) printf("%c", get_at(s, IBUF + t + i));
	printf("]\n");
	while (w != -1) {
		printf("FIND-NAME:CURRENT WORD %d\n", w);
		if (compare_without_case(s, WORD(s, w), t, l)) { printf("FIND-NAME:NAME FOUND!\n"); break; }
		w = WORD(s, w)->p;
	}
	PUSH(s, t);
	PUSH(s, l);
	PUSH(s, w);
}

V do_interpret(S* s, N* w) {
	printf("DO-INTERPRET:WORD CODE %d IS %d\n", w->c, w->c << 2);
	eval(s, REL_TO_ABS(s, w->c << 2));
	printf("/// DO-INTERPRET\n");
}

V do_compile(S* s, N* w) {
	int t = 1;
	int i = 0;
	B k;
	printf("DO-COMPILE\n");
	while (t > 0) {
		switch ((k = get_at(s, REL_TO_ABS(s, w->c + i)))) {
		case ']': case '}': t--; if (t > 0) put(s, k); break;
		case '[': case '{': t++; put(s, k); break;
		default: put(s, k); break;
		}
		i++;
	}
	printf("/// DO-COMPILE\n");
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
		printf("DO-COLON:PARSING\n");
	  parse_name(s);
		if (T(s) == 0) { DROP(s); DROP(s); s->err = -16; printf("/// DO-COLON::ZERO LENGTH WORD\n"); return 1; }
		else {
			int i;
			W w;
		  l = POP(s);
			t = POP(s);
			printf("DO-COLON:CREATING WORD WITH NAME [");
			for (i = 0; i < l; i++) { printf("%c", get_at(s, IBUF + t)); }
			printf("]\nDO-COLON:HERE PRE ALIGN %ld\n", HERE(s));
			align(s);
			printf("DO-COLON:HERE POST ALIGN %ld\n", HERE(s));
			w = HERE(s) >> 2;
			printf("DO-COLON:WORD'S AT %ld\n", w);
			printf("DO-COLON:COMPILING WORD:STORING LATEST AS PREVIOUS %d\n", LATEST(s));
			wput(s, LATEST(s));
			printf("DO-COLON:COMPILING WORD:SETTING LATEST AS WORD %d\n", w);
			LATEST(s) = w;
			printf("DO-COLON:COMPILING WORD:SETTING CODE TO 0\n");
			wput(s, 0);
			printf("DO-COLON:COMPILING WORD:SETTING FLAGS (HIDDEN) %d AND NAMELENGTH %d TO %d\n", HIDDEN, l, HIDDEN & (B)l);
			put(s, HIDDEN | ((B)(l)) & 31);
			printf("DO-COLON:SAVING WORD'S NAME\n");
			for (i = 0; i < l; i++) put(s, get_at(s, IBUF + t + i));
			put(s, 0);
			printf("DO-COLON:HERE AFTER WORD'S HEADER %ld\n", HERE(s));
			align(s);
			printf("DO-COLON:HERE AFTER WORD'S HEADER AND ALIGNMENT %ld\n", HERE(s));
			s->st = 1;
			WORD(s, w)->c = HERE(s) >> 2;
			printf("/// DO-COLON\n");
			return 1;
		}
	} else {
		printf("DO-COLON:NO COLON\n");
		PUSH(s, t);
		PUSH(s, l);
		printf("/// DO-COLON\n");
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
	printf("/// DO-NUMBER\n");
}

V refill(S* s, B* str) {
	int i;
	printf("REFILL\n");
	put_at(s, IPOS, 0);
	put_at(s, ILEN, (B)strlen(str));
	for (i = 0; i < strlen(str); i++) put_at(s, IBUF + i, str[i]);
	printf("/// REFILL\n");
}

V evaluate(S* s, B* str) {
	printf("EVALUATE\n");
	printf("EVALUATE:HERE %d\n", HERE(s));
	refill(s, str);
	while (get_at(s, IPOS) < get_at(s, ILEN)) {
		parse_name(s);
		if (!T(s)) { DROP(s); DROP(s); return; }
		find_name(s);
		if (T(s) != -1) {
			N* w = (N*)REL_TO_ABS(s, (POP(s) << 2));
			DROP(s); DROP(s);
			if (is_immediate(s, w) || !s->st) do_interpret(s, w);
			else do_compile(s, w);
		} else {
			DROP(s);
			if (!do_asm(s))
				if (!do_casm(s))
					if (!do_colon(s))
						if (!do_semicolon(s))
							do_number(s);
		}
	}
	printf("/// EVALUATE\n");
}

S* init() {
	S* s = malloc(sizeof(S));
	printf("INIT\n");
	s->x = malloc(26*sizeof(C));
	s->ip = 0;
	printf("/// INIT\n");
	return s;
}

S* init_dict(S* s) {
	printf("INIT-DICT\n");
	s->d = malloc(64 * 1024 * sizeof(C));
	SIZE(s) = 64 * 1024 * sizeof(C);
	HERE(s) = 3*sizeof(C);
	LATEST(s) = -1;
	s->ip = SIZE(s);
	printf("/// INIT-DICT:HERE %d\n", HERE(s));
	return s;
}

#endif
