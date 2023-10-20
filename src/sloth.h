#ifndef SLOTH_EXT
#define SLOTH_EXT

#include<stdlib.h>
#include"vm.h"

#define ERR_OK 0
#define ERR_DICT_OVERFLOW -8
#define ERR_UNDEFINED_WORD -13
#define ERR_ZERO_LEN_NAME -16
#define ERR_CONTROL_STRUCTURE_MISMATCH -22
#define ERR_SYMBOL_ALLOCATION -256

typedef struct _Header {
	P sz;
	P l;
  P h;
	C* ibuf;
	P ipos;
	P ilen;
	B s;
} H;

#define SIZE(x) (((H*)(x->b))->sz)
#define LATEST(x) (((H*)(x->b))->l)
#define HERE(x) (((H*)(x->b))->h)
#define IBUF(x) (((H*)(x->b))->ibuf)
#define IPOS(x) (((H*)(x->b))->ipos)
#define ILEN(x) (((H*)(x->b))->ilen)
#define STATE(x) (((H*)(x->b))->s)

#define REL_TO_ABS(x, a) ((V*)((P)x->b + (P)a))
#define ABS_TO_REL(x, a) ((P)a - (P)x->b)

typedef struct _Word {
	P p;
	P c;
	B f;
	B l;
	C n[1];
} W;

#define VARIABLE 1
#define HIDDEN 2
#define IMMEDIATE 4
#define CONSTANT 8

V bcompile(X* x) { L1(x, C, c); x->b[HERE(x)++] = c; }

V parse_spaces(X* x) { while (IPOS(x) < ILEN(x) && isspace(IBUF(x)[IPOS(x)])) IPOS(x)++; }
V parse(X* x) { L1(x, C, c); while (IPOS(x) < ILEN(x) && IBUF(x)[IPOS(x)] != c) IPOS(x)++; IPOS(x)++; }
V parse_non_spaces(X* x) { while (IPOS(x) < ILEN(x) && !isspace(IBUF(x)[IPOS(x)])) IPOS(x)++; }
V parse_name(X* x) {
	parse_spaces(x);
	PUSH(x, &IBUF(x)[IPOS(x)]);
	parse_non_spaces(x);
	PUSH(x, ((P)(&IBUF(x)[IPOS(x)])) - T(x));
}

V colon(X* x) {
	printf("On colon...\n");
	parse_name(x);
	ERR(x, T(x) == 0, ERR_ZERO_LEN_NAME);
	{
		L2(x, P, l, C*, n);
		W* w = REL_TO_ABS(x, HERE(x));
		w->p = LATEST(x);
		LATEST(x) = HERE(x);
		HERE(x) += sizeof(W) + l;
		printf("Created word [%.*s] at [%ld] with code at [%ld]\n", (int)l, n, ABS_TO_REL(x, w), HERE(x));
		w->c = HERE(x);
		w->f = HIDDEN;
		w->l = l;
		strncpy(w->n, n, l);
		w->n[l] = 0;
    STATE(x) = 1;
	}
}

V semicolon(X* x) {
	W* l;
  ERR(x, LATEST(x) == 0, ERR_CONTROL_STRUCTURE_MISMATCH);
	l = REL_TO_ABS(x, LATEST(x));
	l->f &= ~HIDDEN;
  x->b[HERE(x)++] = ']';
  STATE(x) = 0;
}

V immediate(X* x) { W* l = REL_TO_ABS(x, LATEST(x)); l->f |= IMMEDIATE; }

V literal(X* x) {
	L1(x, P, n);
	if (n == 0) { x->b[HERE(x)++] = '0'; }
	else if (n == 1) { x->b[HERE(x)++] = '1'; }
	else if (n >= INT8_MIN && n <= INT8_MAX) { 
		x->b[HERE(x)++] = '#'; *((B*)&x->b[HERE(x)]) = n; HERE(x) += 1; }
	else if (n >= INT16_MIN && n <= INT16_MAX) {
		x->b[HERE(x)++] = '2'; *((S*)&x->b[HERE(x)]) = n; HERE(x) += 2; }
	else if (n >= INT32_MIN && n <= INT32_MAX) { 
		x->b[HERE(x)++] = '4'; *((I*)&x->b[HERE(x)]) = n; HERE(x) += 4; }
	else { 
		x->b[HERE(x)++] = '8'; *((L*)&x->b[HERE(x)]) = n; HERE(x) += 8; }
}

P code_length(X* x, P c) {
	int t = 1;
	P n = 0;
	while (t) {
		if (x->b[c] == '[') { t++; c++; n++; }
		else if (x->b[c] == ']') { t--; c++; n++; }
		else if (x->b[c] == '#') { c += 2; n += 2; }
		else if (x->b[c] == '2') { c += 3; n += 3; }
		else if (x->b[c] == '4') { c += 5; n += 5; }
		else if (x->b[c] == '8') { c += 9; n += 9; }
		else { c++;	n++; }
	}
	return n - 1;
}

V compile(X* x) {
	L1(x, P, c);
	P cl = code_length(x, c);
	printf("Compiling, code length %ld\n", cl);
	if (cl < 5) {
		C i;
		printf("Inlining\n");
		for (i = 0; i < cl; i++) x->b[HERE(x)++] = x->b[c + i];
	} else {
		PUSH(x, c);
		literal(x);
		x->b[HERE(x)++] = 'x';
	}
}

V find_name(X* x) {
	L2(x, P, l, B*, t);
	P wp = LATEST(x);
	while (wp) {
		W* w = REL_TO_ABS(x, wp);
		if (w->l == l) {
			P i;
			P f = 1;
			for (i = 0; i < l; i++) {
				if (t[i] >= 97 && t[i] <= 122) {
					if (t[i] != w->n[i] && t[i] != (w->n[i] + 32)) { f = 0; break; }
				} else if (t[i] >= 65 && t[i] <= 90) {
					if (t[i] != w->n[i] && t[i] != (w->n[i] - 32)) { f = 0; break; }
				} else {
					if (t[i] != w->n[i]) { f = 0; break; }
				}
			}
			if (f) break;
		}
		wp = w->p;
	}
	PUSH(x, t);
	PUSH(x, l);
	PUSH(x, wp);
}

V sloth_ext(X* x) {
	switch (TOKEN(x)) {
	case '$': PUSH(x, x->b[x->ip++]); bcompile(x); break;

	case ':': colon(x); break;
  case ';': semicolon(x); break;
	case 'i': immediate(x); break;

	case 't': x->err = POP(x); break;
	case 'p': parse(x); break;
	}
}

V evaluate(X* x, C* s) {
	IBUF(x) = s;
	IPOS(x) = 0;
	ILEN(x) = strlen(s);
	while (IPOS(x) < ILEN(x)) {
		parse_name(x);
		if (T(x) == 0) { DROP(x); DROP(x); return; }
		find_name(x);
		if (T(x)) {
			L3(x, P, wp, P, _, C*, __);
			W* w = (W*)REL_TO_ABS(x, wp);
			printf("Word found [%.*s] at [%ld]\n", (int)_, __, wp);
      if (!STATE(x) || (w->f & IMMEDIATE) == IMMEDIATE) {
				printf("Evaluating [%ld]\n", w->c);
        eval(x, w->c);
      } else {
				printf("Compiling\n");
				PUSH(x, w->c);
				compile(x);
      }
		} else {
			L3(x, P, _, P, l, C*, t);
			printf("Word NOT found [%.*s]\n", (int)l, t);
			if (t[0] == '\\') {
				int i;
				printf("Evaluating assembler [%.*s]\n", (int)(l - 1), t + 1);
				for (i = 1; i < l; i++) { x->b[SIZE(x) - l + i] = t[i]; }
				eval(x, SIZE(x) - l + 1);
			} else if (t[0] == '$') {
				int i;
				for (i = 1; i < l; i++) { x->b[HERE(x)++] = t[i]; }
			} else {
				char * end;
				int n = strtol(t, &end, 10);
				if ((n == 0 && end == t) || end < (t + l)) {
					PUSH(x, t);
					PUSH(x, l);
					x->err = ERR_UNDEFINED_WORD;
					return;
				}
				PUSH(x, n);
				if (STATE(x)) literal(x);
			}
		}
	}
}

X* init_SLOTH() {
	X* x = init();
	x->b = malloc(65536);
	x->s = 65536;
	SIZE(x) = 65536;
	LATEST(x) = 0;
	HERE(x) = sizeof(H);
	STATE(x) = 0;
	EXT(x, 'S') = &sloth_ext;

	return x;
}

#endif
