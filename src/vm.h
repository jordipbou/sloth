#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

typedef void V;
typedef char C;
typedef int8_t B;
typedef int16_t S;
typedef int32_t I;
typedef int64_t L;
typedef intptr_t P;

#define DSTACK_SIZE 64 
#define RSTACK_SIZE 64

typedef struct _Context {
	P dp;
	P rp;
	P ip;
	P err;
	B* b;
	P s;
	V (**x)(struct _Context*);
  P d[DSTACK_SIZE];
	P r[RSTACK_SIZE];
} X;

V inner(X*);

#define BLOCK_SIZE(x) (x->s)

#define EXT(x, l) (x->x[l - 'A'])

#define PUSH(x, v) (x->d[x->dp++] = (P)(v))
#define POP(x) (x->d[--x->dp])
#define DROP(x) (--x->dp)

#define T(x) (x->d[x->dp - 1])
#define N(x) (x->d[x->dp - 2])
#define NN(x) (x->d[x->dp - 3])

#define L1(x, t, v) t v = (t)POP(x)
#define L2(x, t1, v1, t2, v2) L1(x, t1, v1); L1(x, t2, v2)
#define L3(x, t1, v1, t2, v2, t3, v3) L2(x, t1, v1, t2, v2); L1(x, t3, v3)
#define L4(x, t1, v1, t2, v2, t3, v3, t4, v4) L3(x, t1, v1, t2, v2, t3, v3); L1(x, t4, v4)

#define DO(x, f) { f(x); if (x->err) return; }
#define ERR(x, c, e) if (c) { x->err = e; return; }

V dup(X* x) { PUSH(x, T(x)); }
V over(X* x) { PUSH(x, N(x)); }
V swap(X* x) { P t = T(x); T(x) = N(x); N(x) = t; }
V rot(X* x) { P t = NN(x); NN(x) = N(x); N(x) = T(x); T(x) = t; }
V nip(X* x) { N(x) = T(x); DROP(x); }

V to_r(X* x) { x->r[x->rp++] = x->d[--x->dp]; }
V from_r(X* x) { x->d[x->dp++] = x->r[--x->rp]; }

V add(X* x) { N(x) = N(x) + T(x); DROP(x); }
V sub(X* x) { N(x) = N(x) - T(x); DROP(x); }
V mul(X* x) { N(x) = N(x) * T(x); DROP(x); }
V division(X* x) { N(x) = N(x) / T(x); DROP(x); }
V mod(X* x) { N(x) = N(x) % T(x); DROP(x); }

V and(X* x) { N(x) = N(x) & T(x); DROP(x); }
V or(X* x) { N(x) = N(x) | T(x); DROP(x); }
V xor(X* x) { N(x) = N(x) ^ T(x); DROP(x); }
V not(X* x) { T(x) = !T(x); }
V inverse(X* x) { T(x) = ~T(x); }

V lt(X* x) { N(x) = (N(x) < T(x)) ? -1 : 0; DROP(x); }
V eq(X* x) { N(x) = (N(x) == T(x)) ? -1 : 0; DROP(x); }
V gt(X* x) { N(x) = (N(x) > T(x)) ? -1 : 0; DROP(x); }

V pstore(X* x) { L2(x, P*, a, P, b); *a = b; }
V pfetch(X* x) { L1(x, P*, a); PUSH(x, *a); }
V bstore(X* x) { L2(x, B*, a, B, b); *a = b; }
V bfetch(X* x) { L1(x, B*, a); PUSH(x, *a); }

#define TAIL(x) (x->ip >= BLOCK_SIZE(x) || x->b[x->ip] == ']' || x->b[x->ip] == '}')
V call(X* x) { L1(x, P, q); if (!TAIL(x)) x->r[x->rp++] = x->ip; x->ip = q; }
V ret(X* x) { if (x->rp > 0) x->ip = x->r[--x->rp]; else x->ip = BLOCK_SIZE(x); }
V jump(X* x) { L1(x, P, d); x->ip += d - 1; }
V zjump(X* x) { L2(x, P, d, P, b); if (!b) x->ip += d - 1; }
V quot(X* x) { L1(x, P, d); PUSH(x, x->ip); x->ip += d; }
V eval(X* x, P q) { PUSH(x, q); call(x); inner(x); }

V times(X* x) { L2(x, P, q, P, n); for (;n > 0; n--) eval(x, q); }
V choose(X* x) { L3(x, P, f, P, t, P, b); if (b) eval(x, t); else eval(x, f); }

#define PEEK(x) (x->b[x->ip])
#define TOKEN(x) (x->b[x->ip++])

V step(X* x) {
	switch (PEEK(x)) {
	case 'A': case 'B': case 'C': case 'D':
	case 'F': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V':
	case 'W': case 'X': case 'Y': case 'Z':
		EXT(x, TOKEN(x))(x);
		break;
	default:
		switch (TOKEN(x)) {
		case 'e': EXT(x, 'E')(x); break;
		case 'k': EXT(x, 'K')(x); break;

		case '0': PUSH(x, 0); break;
		case '1': PUSH(x, 1); break;
		case '#': PUSH(x, *((B*)(x->b + x->ip))); x->ip += 1; break;
		case '2': PUSH(x, *((S*)(x->b + x->ip))); x->ip += 2; break;
		case '4': PUSH(x, *((I*)(x->b + x->ip))); x->ip += 4; break;
		case '8': PUSH(x, *((L*)(x->b + x->ip))); x->ip += 8; break;

		case '_': DROP(x); break;
		case 'd': dup(x); break;
		case 'o': over(x); break;
		case 's': swap(x); break;
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
		case '^': xor(x); break;
		case '!': not(x); break;
		case '~': inverse(x); break;

		case '<': lt(x); break;
		case '=': eq(x); break;
		case '>': gt(x); break;

		case ',': pstore(x); break;
		case '.': pfetch(x); break;
		case ';': bstore(x); break;
		case ':': bfetch(x); break;

		case 'x': call(x); break;
		case 'j': jump(x); break;
		case 'z': zjump(x); break;
		case '[': quot(x); break;
		case ']': ret(x); break;

		case '{': PUSH(x, x->ip); while (x->ip < BLOCK_SIZE(x) && TOKEN(x) != '}') {}; break;
		case '}': ret(x); break;

		case 'b': PUSH(x, &x->b); break;
		case 'c': PUSH(x, sizeof(P)); break;
		case '@': PUSH(x, x); break;

		case 't': times(x); break;
		case '?': choose(x); break;
		}
	}
}

V inner(X* x) { 
	P rp = x->rp; 
	while(x->rp >= rp && x->ip < BLOCK_SIZE(x) && !x->err) { 
		step(x); 
		/* Manage errors here? */
	} 
}

V execute(X* x, P l, C* s) {
	x->b = (B*)s;
	x->s = l;
	x->ip = 0;
	inner(x);
}

V reset_context(X* x) {
	x->dp = 0;
	x->rp = 0;
	x->ip = x->s;
	x->err = 0;
}

X* init() {
	X* x = malloc(sizeof(X));
	x->x = malloc(sizeof(P) * 26);
	reset_context(x);

	return x;
}

#endif
