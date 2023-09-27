/* TODOS:

	- Correct eval/return/inner problem. Check with 1111+1+1+{so+}ts_ 

  - Find the relation between a packed memory and directly executing strings (maybe don't used packed memory? But, in high level languages everything is stored as objects, not pointers, or packing its used or a table holding references)
  The main problem is the index that points to there (here). Is it byte based or int based?
	- Test shoud be added to ensure call/return/eval/inner works correctly
	- More basic functions can be added, there are enough ASCII characters left and there's no reason to not do it
*/

/*
	Bytecode
	--------
	END OF CODE -> 0, 10 (return), '}', '\'
*/

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */
#include<fcntl.h>  /* open, close, read, write, O_RDONLY, O_WRONLY */

#define V void /* inline void ? */

typedef char B;
typedef int32_t H;
typedef intptr_t C;

#ifndef DICT_SIZE
#define DICT_SIZE 32768
#endif

typedef struct _System {
	B* r;
	B* b;
  H m[DICT_SIZE];
} S;

S* S_init_system() { return malloc(sizeof(S)); }

#define STACK_SIZE 64
#define RSTACK_SIZE 64

typedef struct _Context {
  C ds[STACK_SIZE]; C sp;
  B* as[RSTACK_SIZE]; C rp;
	B* ip;
  C err;
	S* s;
  void (*ext[26])(struct _Context*);
  void* st[26];
  C i, t, n;
} X;

#define EXT(x, l) (x->ext[l - 'A'])
#define ST(x, l) (x->st[l - 'A'])

#define S_push(x, v) (x->ds[x->sp++] = (C)(v))

#define T(x) (x->ds[x->sp - 1])
#define N(x) (x->ds[x->sp - 2])
#define NN(x) (x->ds[x->sp - 3])
#define R(x) (x->as[x->rp - 1])

#define NEOC(tk) (tk) && *(tk) && *(tk) != 10

C S_block(X* x, B* tk) { 
	C t = 1; 
	B* s = tk;
	for (;t && NEOC(tk); tk++) { 
		if (*tk == '{') { t++; }
		if (*tk == '}' || *tk == ']') { t--; }
	}
	return tk - s;
}

/* Literals */

V S_parse_block(X* x) { S_push(x, x->ip + 1); x->ip += S_block(x, x->ip + 1); }
V S_char(X* x) { x->ip += 1; S_push(x, *x->ip); }
V S_lit(X* x) { S_push(x, ((*x->ip) << 8 + *(x->ip + 1)) & 0xEFFF); x->ip++; }

V S_quit(X* x) { exit(0); }

V S_dup(X* x) { S_push(x, T(x)); }
C S_drop(X* x) { return x->ds[--x->sp]; }
V S_swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V S_over(X* x) { S_push(x, N(x)); }

V S_to_R(X* x) { x->as[x->rp++] = (B*)x->ds[--x->sp]; }
V S_from_R(X* x) { x->ds[x->sp++] = (C)x->as[--x->rp]; }

V S_save_ip(X* x) { if (x->ip != 0 && *x->ip != 0 && *x->ip != 10) x->as[x->rp++] = x->ip; }
V S_jump(X* x) { x->ip = (B*)S_drop(x); }
V S_call(X* x) { S_save_ip(x); S_jump(x); }
V S_ccall(X* x) { S_swap(x); if (S_drop(x)) S_call(x); else S_drop(x); }
V S_cjump(X* x) { S_swap(x); if (S_drop(x)) S_jump(x); else S_drop(x); }
V S_return(X* x) { if (x->rp > 0) { x->ip = R(x) - 1; x->rp--; } else { x->rp = 0; x->ip = 0; } }

V S_eq(X* x) { N(x) = (N(x) == T(x)) ? -1 : 0; x->sp--; }
V S_neq(X* x) { N(x) = (N(x) != T(x)) ? -1 : 0; x->sp--; }
V S_lt(X* x) { N(x) = (N(x) < T(x)) ? -1 : 0; x->sp--; }
V S_gt(X* x) { N(x) = (N(x) > T(x)) ? -1 : 0; x->sp--; }

V S_fetch(X* x) { T(x) = x->s->m[T(x)]; }
V S_store(X* x) { x->s->m[T(x)] = N(x); x->sp -= 2; }

V S_add(X* x) { N(x) = N(x) + T(x); x->sp -= 1; }
V S_sub(X* x) { N(x) = N(x) - T(x); x->sp -= 1; }
V S_mul(X* x) { N(x) = N(x) * T(x); x->sp -= 1; }
/* On konilo, crc does something called symmetric to change sign... */
V S_divmod(X* x) { C a = T(x); C b = N(x); T(x) = b / a; N(x) = b % a; }

V S_and(X* x) { N(x) = N(x) & T(x); x->sp -= 1; }
V S_or(X* x) { N(x) = N(x) | T(x); x->sp -= 1; }
V S_xor(X* x) { N(x) = N(x) ^ T(x); x->sp -= 1; }
V S_not(X* x) { T(x) = !T(x); }
V S_invert(X* x) { T(x) = ~T(x); }

V S_shl(X* x) { N(x) = N(x) << T(x); x->sp -= 1; }
V S_shr(X* x) { N(x) = N(x) >> T(x); x->sp -= 1; }

V S_eval(X* x, B* s);
V S_trace(X* x);

V S_times(X* x) {	B* q = (B*)S_drop(x);	C n = S_drop(x); for (;n > 0; n--) { S_eval(x, q); } }

#define DC(a) { b += m = sprintf(b, "%ld ", a); n += m; }
C D_co(X* x, B* b, B* c) { return !c || *c == 0 || *c == 10 ? sprintf(b, "0") : sprintf(b, "%.*s", (int)S_block(x, c), c); }
C D_ds(X* x, B* b) { C i = 0; C n = 0; C m = 0; for (;i < x->sp; i++) DC(x->ds[i]); return n; }
C D_ctx(X* x, B* b) {
	C i; C n = 0; C m = 0;
	b += m = D_ds(x, b); n += m;
	b += m = sprintf(b, "| "); n += m;
	b += m = D_co(x, b, x->ip); n += m;
	for (i = x->rp - 1; i >= 0; i--) { 
		b += m = sprintf(b, " : "); n += m;
		b += m = D_co(x, b, x->as[i]); n += m; 
	}
	return n;
}

V S_trace(X* x) {
	B buf[255];
	memset(buf, 0, 255);
	D_ctx(x, buf);
	printf("%s\n", buf);
}

V S_step(X* x) {
  B token = *x->ip;
	S_trace(x);
	switch (token) {
		case 'q': S_quit(x); break;
		case '0': S_push(x, 0); break;
		case '1': S_push(x, 1); break;
		case '{': S_parse_block(x); break;
		case '\'': S_char(x); break;
	  case 'd': S_dup(x); break;	
		case '_': S_drop(x); break;
		case 's': S_swap(x); break;
		case 'o': S_over(x); break;
		case '(': S_to_R(x); break;
		case ')': S_from_R(x); break;
		case 'j': S_jump(x); break;
		case 'x': S_call(x); break;
		case 'c': S_ccall(x); break; /* Where is this used? */
		case 'i': S_cjump(x); break; /* Where is this used? */
		case 0: case 10: case '}': case ']': S_return(x); break;
		case '=': S_eq(x); break;
		case '.': S_fetch(x); break;
		case ',': S_store(x); break;
		case '+': S_add(x); break;
		case '-': S_sub(x); break;
		case '*': S_mul(x); break;
		case '%': S_divmod(x); break;
		case '&': S_and(x); break;
		case '|': S_or(x); break;
		case '^': S_xor(x); break;
		case '!': S_not(x); break;
		case '~': S_invert(x); break;
		case 'l': S_shl(x); break;
		case 'r': S_shr(x); break;
		case 't': S_times(x); break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
		case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			EXT(x, token)(x);
			break;
		default:
			if (token > 127) { S_lit(x); }
			else { /* NOOP */ }
			break;
	}
}

V S_inner(X* x) { C rp = x->rp; while(!x->err && x->rp >= rp && NEOC(x->ip)) { S_step(x); x->ip += 1; } }

V S_eval(X* x, B* s) { printf("EVAL: RP: %ld\n", x->rp); S_push(x, s); S_call(x); S_inner(x); /* S_return(x); */ printf("EVAL OUT: RP: %ld\n", x->rp); }

X* S_init() {
	S* s = malloc(sizeof(S));
	X* x = malloc(sizeof(X));
	return x;
}

#endif
