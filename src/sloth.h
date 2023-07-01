#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<string.h>

typedef char B;
typedef intptr_t C;

#define STACK_SIZE 32 
#define RSTACK_SIZE 32 

typedef struct _X { 
  C* s; C sp; C ss;
  B** r; C rp; C rs;
	B* ip;
/* I don't really like vars */
/* I would prefer 4 stacks (1 more array) and use letters on bytecode, it's easier to write */
  C var[26];
  void (*ext[26])(struct _X*);
  C err;
  C tr;
} X;

#define EXT(x, l) (x->ext[l - 'A'])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(C));
  x->r = malloc(RSTACK_SIZE*sizeof(C));
	x->sp = x->rp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->err = 0;
  x->tr = 0;
	return x;
}

#define PR(f, a) s += t = sprintf(s, f, a); n += t
#define PW(c, f, a) while (c) { PR(f, a); } return n

#define DUMP(nm, b) C S_dump_##nm(B* s, X* x) { C i = 0, t = 1, n = 0; b; }

DUMP(S, { PW(i < x->sp, "%ld ", x->s[i++]); })
  
C S_dump_CODE(B* s, B* a) {
	C i = 0, t = 1;
	if (a == 0) return 0;
	while (t && a[i] && a[i] != 10) {
		switch (a[i++]) {
		case '[': t++; break;
		case ']': t--; break;
		}
	}
	return sprintf(s, "%.*s", (unsigned int)i, a);
}

C S_dump_R(B* s, X* x) {
	C i, t = 1, n = 0;
	s += t = S_dump_CODE(s, x->ip); n += t;
	for (i = x->rp - 1; i >= 0; i--) {
		*s++ = ' '; *s++ = ':'; *s++ = ' '; n += 3;
		s += t = S_dump_CODE(s, x->r[i]); n += t;
	}
	return n;
}

C S_dump_X(B* s, X* x, unsigned int w) {
	B buf[255];
	C t, n = 0;
	memset(buf, 0, 255);
	t = S_dump_S(buf, x);
	if (t > w) {
		s += t = sprintf(s, "...%-.*s", w - 3, buf); n += t;
	} else {
		s += t = sprintf(s, "%*s", w, buf); n += t;
	}
	*s++ = ':'; *s++ = ' '; n += 2;
  memset(buf, 0, 255);
  t = S_dump_R(buf, x);
  if (t > w) {
    s += t = sprintf(s, "%.*s...", w - 3, buf); n += t;
  } else {
    s += t = sprintf(s, "%s", buf); n += t;
  }
	return n;
}

void S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 || *x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])

void S_lit(X* x, C v) { x->s[x->sp++] = v; }
void S_dup(X* x) { S_lit(x, TS(x)); }
void S_over(X* x) { S_lit(x, NS(x)); }
void S_rot(X* x) { C t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
void S_swap(X* x) { C t = TS(x); TS(x) = NS(x); NS(x) = t; }
C S_drop(X* x) { return x->s[--x->sp]; }

void S_add(X* x) { NS(x) = NS(x) + TS(x); --x->sp; }
void S_sub(X* x) { NS(x) = NS(x) - TS(x); --x->sp; }
void S_mul(X* x) { NS(x) = NS(x) * TS(x); --x->sp; }
void S_divmod(X* x) { C a = TS(x); C b = NS(x); NS(x) = b / a; TS(x) = b % a; }

void S_and(X* x) { NS(x) = NS(x) & TS(x); --x->sp; }
void S_or(X* x) { NS(x) = NS(x) | TS(x); --x->sp; }
void S_not(X* x) { TS(x) = !TS(x); }

void S_lt(X* x) { NS(x) = NS(x) < TS(x); --x->sp; }
void S_eq(X* x) { NS(x) = NS(x) == TS(x); --x->sp; }
void S_gt(X* x) { NS(x) = NS(x) > TS(x); --x->sp; }

void S_push(X* x) { x->r[x->rp++] = x->ip; }
void S_pop(X* x) { x->ip = x->r[--x->rp]; }
void S_call(X* x) { B t = S_peek(x); if (t && t != ']') S_push(x); x->ip = (B*)S_drop(x); }
void S_eval(X* x, B* q) { S_lit(x, (C)q); S_call(x); S_inner(x); }
void S_if(X* x) { S_rot(x); if (!S_drop(x)) { S_swap(x); } S_drop(x); S_call(x); }
void S_times(X* x) { B* q = (B*)S_drop(x); C n = S_drop(x); while (n-- > 0) { S_eval(x, q); } }

void S_bstore(X* x) { B* a = (B*)S_drop(x); *a = (B)S_drop(x); }
void S_cstore(X* x) { C* a = (C*)S_drop(x); *a = S_drop(x); }

void S_bfetch(X* x) { S_lit(x, *((B*)S_drop(x))); }
void S_cfetch(X* x) { S_lit(x, *((C*)S_drop(x))); }

void S_parse_literal(X* x) { 
	C n = 0; 
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); } 
	S_lit(x, n); 
}

void S_parse_quotation(X* x) { 
	C t = 1; 
	B c; 
	S_lit(x, (C)(++x->ip)); 
	while (t) { switch (S_token(x)) { case '[': t++; break; case ']': t--; break; } } 
}

void S_inner(X* x) {
	B buf[255];
	C frame = x->rp;
	do {
		memset(buf, 0, 255);
		S_dump_X(buf, x, 20);
		printf("%s\n", buf);
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
		case '[': 
      S_parse_quotation(x); break;
		case 0: case ']': 
      if (x->rp > frame && x->rp > 0) S_pop(x);
			else return;
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
      S_lit(x, (C)&x->var[S_token(x) - 'a']);
      break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, S_token(x))(x);
      break;
		default:
			switch (S_token(x)) {
      case '\'': S_lit(x, (C)S_token(x)); break;
			case '$': S_swap(x); break;
			case '{': S_dup(x); break;
			case '^': S_over(x); break;
			case '@': S_rot(x); break;
			case '_': S_drop(x); break;
			case '+': S_add(x); break;
			case '-': S_sub(x); break;
			case '*': S_mul(x); break;
			case '/': S_divmod(x); break;
			case '&': S_and(x); break;
			case '|': S_or(x); break;
			case '!': S_not(x); break;
			case '<': S_lt(x); break;
			case '=': S_eq(x); break;
			case '>': S_gt(x); break;
			case '%': S_call(x); break;
			case '?': S_if(x); break;
			case '#': S_times(x); break;
      case ':': S_bfetch(x); break;
      case ';': S_bstore(x); break;
			case '.': S_cfetch(x); break;
			case ',': S_cstore(x); break;
      case '~': S_lit(x, sizeof(C)); break;
			case '`': exit(0); break;
			}
		}
	} while(1);
}

#endif
