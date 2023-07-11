#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<string.h>

typedef char B;
typedef intptr_t C;

#define szC sizeof(C)

#define STACK_SIZE 64
#define RSTACK_SIZE 64

#define BLOCK_SIZE 65536

#define HERE(x) (((C*)x->b)[0])
#define LATEST(x) (((C*)x->b)[1])

/* TODO: Use mask for flags/length? */
#define FLAGS(s) (*(s + szC))
#define NL(s) (*(s + szC + 1))
#define NFA(s) (s + szC + 1 + 1)
#define CFA(s) (NFA(s) + NL(s))

#define VC(x, n) C n = S_drop(x)
#define VB(x, n) B* n = (B*)S_drop(x)
#define VS(x, n) VC(x, l##n); VB(x, s##n)

typedef struct _X { 
  C* s; C sp; C ss;
  B** r; C rp; C rs;
	B* ip;
  B* b;
	void (*key)(struct _X*);
	void (*emit)(struct _X*);
  void (**ext)(struct _X*);
  C err;
  C tr;
} X;

#define EXT(x, l) (x->ext[l - 'A'])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*szC);
  x->r = malloc(RSTACK_SIZE*szC);
	x->b = malloc(BLOCK_SIZE);
	HERE(x) = 2*szC;
	x->sp = x->rp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ext = malloc(26*szC);
  x->err = 0;
  x->tr = 0;
	return x;
}

C S_dump_S(B* s, X* x) {
	C i = 0, t, n = 0;
	while (i < x->sp) { s += t = sprintf(s, "%ld ", x->s[i]); n += t; i++; }
	return n;
}

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
	C t, n = 0;
	s += t = S_dump_S(s, x); n += t;
	*s++ = ':'; *s++ = ' '; n += 2;
  s += t = S_dump_R(s, x); n += t;
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
void S_div(X* x) { NS(x) = NS(x) / TS(x); --x->sp; }
void S_mod(X* x) { NS(x) = NS(x) % TS(x); --x->sp; }

void S_and(X* x) { NS(x) = NS(x) & TS(x); --x->sp; }
void S_or(X* x) { NS(x) = NS(x) | TS(x); --x->sp; }
void S_xor(X* x) { NS(x) = NS(x) ^ TS(x); --x->sp; }
void S_not(X* x) { TS(x) = !TS(x); }
void S_invert(X* x) { TS(x) = ~TS(x); }

void S_lt(X* x) { NS(x) = NS(x) < TS(x); --x->sp; }
void S_eq(X* x) { NS(x) = NS(x) == TS(x); --x->sp; }
void S_gt(X* x) { NS(x) = NS(x) > TS(x); --x->sp; }

void S_to_R(X* x) { x->r[x->rp++] = (B*)x->s[--x->sp]; }
void S_from_R(X* x) { x->s[x->sp++] = (C)x->r[--x->rp]; }
void S_peek_R(X* x) { x->s[x->sp++] = (C)x->r[x->rp - 1]; }

void S_push(X* x) { x->r[x->rp++] = x->ip; }
void S_pop(X* x) { x->ip = x->r[--x->rp]; }
void S_call(X* x) { B t = S_peek(x); if (t && t != ']' && t != '}') S_push(x); x->ip = (B*)S_drop(x); }
void S_eval(X* x, B* q) { S_lit(x, (C)q); S_call(x); S_inner(x); }


void S_bstore(X* x) { B* a = (B*)S_drop(x); *a = (B)S_drop(x); }
void S_cstore(X* x) { C* a = (C*)S_drop(x); *a = S_drop(x); }

void S_bfetch(X* x) { S_lit(x, *((B*)S_drop(x))); }
void S_cfetch(X* x) { S_lit(x, *((C*)S_drop(x))); }

void S_malloc(X* x) { S_lit(x, (C)malloc(S_drop(x))); }
void S_free(X* x) { free((void*)S_drop(x)); }
void S_inspect(X* x) {
  /* TODO: Show ASCII representation */
  /* TODO: Use vectored I/O */
  C i = 0, j;
  C n = S_drop(x);
  B* a = (B*)S_drop(x);
  while (i < n) {
    /* Do with type! */
    printf("\n%p: ", a + i);
    for (j = 0; j < 4 && i < n; j++, i++) {
      printf("%02X ", (unsigned char)a[i]);
    }
    if (i < n) {
      printf("- ");
      for (j = 0; j < 4 && i < n; j++, i++) {
        printf("%02X ", (unsigned char)a[i]);
      }
    }
  }
  printf("\n");
}

void S_compare(X* x) { VS(x, 1); VS(x, 2); S_lit(x, l2 == l1 && !strncmp(s2, s1, l2)); }

void S_accept(X* x) { 
	C i = 0;
  VS(x, 1);
	do { 
		x->key(x); 
		if (TS(x) == 10) {
			S_drop(x);
			break;
		} else if (TS(x) == 127) {
      S_drop(x);
      if (i > 0) {
        S_lit(x, '\b');
        x->emit(x);
        S_lit(x, ' ');
        x->emit(x);
        S_lit(x, '\b');
        x->emit(x);
        i--;
      }
    } else {
			s1[i++] = TS(x);
			x->emit(x);
		}
	} while(i < l1);
	S_lit(x, i);
}

void S_type(X* x) {
	C i = 0;
	C n = S_drop(x);
	B* s = (B*)S_drop(x);
	while (n-- > 0) { S_lit(x, s[i++]); x->emit(x); }
}

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

void S_parse_string(X* x) {
  S_lit(x, (C)(++x->ip));
  while (S_token(x) != '"') {}
  S_lit(x, (C)(x->ip - TS(x)) - 1);
}

#define BCOMP(x, v) *(x->b + HERE(x)) = v; HERE(x)++
#define CCOMP(x, v) *((C*)(x->b + HERE(x))) = v; HERE(x) += szC

void S_bcompile(X* x) { BCOMP(x, (B)S_drop(x)); }
void S_ccompile(X* x) { CCOMP(x, S_drop(x)); }
/* test */
void S_qcompile(X* x) { 
  C l = 0, t = 1; 
  B* q = (B*)S_drop(x);
  while (t) {
    if (q[l] == '[') t++;
    if (q[l] == ']') t--;
    l++;
  }
  strncpy(x->b + HERE(x), q, l - 1);
  HERE(x) += l - 1; 
}
void S_scompile(X* x) { C l = S_drop(x); B* s = (B*)S_drop(x); BCOMP(x, '"'); strncpy(x->b + HERE(x), s, l); BCOMP(x, '"'); HERE(x) += l + 2; }

void S_allot(X* x) { HERE(x) += S_drop(x); }

void S_parse_name(X* x) {
  VC(x, i);
  VB(x, s);
  while (s[i] != 0 && isspace(s[i])) { i++; }
  S_lit(x, (C)(s + i));
  while (s[i] != 0 && !isspace(s[i])) { i++; }
  S_lit(x, (C)(s + i - TS(x)));
	S_lit(x, i);
}

void S_find(X* x) {
  VC(x, l);
  VB(x, s);
  B* w = (B*)LATEST(x);
  while (w) {
    if (NL(w) == l && !strncmp(NFA(w), s, l)) {
      S_lit(x, (C)CFA(w));
      return;
    }
    w = *((B**)w);
  }
  S_lit(x, (C)s);
  S_lit(x, l);
  S_lit(x, 0);
}

void S_create(X* x) {
  VS(x, 1);
  B* w = x->b + HERE(x);
	*((B**)w) = (B*)LATEST(x);
	LATEST(x) = (C)w;
	FLAGS(w) = 0;
	NL(w) = l1;
	strncpy(NFA(w), s1, l1);
	S_lit(x, (C)CFA(w));
	HERE(x) += sizeof(B**) + 2 + l1;  
}

void S_symbol(X* x) {
  /*
  S_lit(x, (C)(x->ip));
  S_lit(x, 0);
  S_parse_name(x);
  S_find(x);
  if (TS(x) == 0) {
    S_drop(x);
    S_create(x);
  }
  */
  C l = 0;
	B* s = x->ip;
	B* w = (B*)LATEST(x);
	while (!isspace(S_token(x))) { l++; }
	while (w) {
		if (NL(w) == l && !strncmp(NFA(w), s, l)) {
			S_lit(x, (C)CFA(w));
			return;
		}
		w = *((B**)w);
	}
  w = x->b + HERE(x);
	*((B**)w) = (B*)LATEST(x);
	LATEST(x) = (C)w;
	FLAGS(w) = 0;
	NL(w) = l;
	strncpy(NFA(w), s, l);
	S_lit(x, (C)CFA(w));
	HERE(x) += sizeof(B**) + 2 + l;
}

void S_inner(X* x) {
	B buf[255];
	C frame = x->rp;
	do {
		memset(buf, 0, 255);
		S_dump_X(buf, x, 50);
		printf("%s <%ld>\n", buf, x->rp);
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
		case '[': 
      S_parse_quotation(x); break;
    case '"':
      S_parse_string(x); break;
		case 0: case ']': case '}':
      if (x->rp > frame && x->rp > 0) S_pop(x);
			else return;
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
      case '#': S_lit(x, *((C*)x->ip)); x->ip += sizeof(C); break;
      case '\\': S_symbol(x); break;
      /* Stack */
			case 's': S_swap(x); break;
			case 'd': S_dup(x); break;
			case 'o': S_over(x); break;
			case 'r': S_rot(x); break;
			case '_': S_drop(x); break;
      /* Arithmetics */
			case '+': S_add(x); break;
			case '-': S_sub(x); break;
			case '*': S_mul(x); break;
			case '/': S_div(x); break;
			case '%': S_mod(x); break;
      /* Bitwise */
			case '&': S_and(x); break;
			case '|': S_or(x); break;
			case '^': S_xor(x); break;
			case '!': S_not(x); break;
			case '~': S_invert(x); break;
      /* Comparison */
			case '<': S_lt(x); break;
			case '=': S_eq(x); break;
			case '>': S_gt(x); break;
      /* Execution */
      case ')': S_from_R(x); break;
      case '(': S_to_R(x); break;
      case 'p': S_peek_R(x); break;
			case '$': 
        switch(S_peek(x)) {
        case ']': case '}':
					/* TODO: This is not correct! a return must be made if necessary !!!! */
          x->ip++;
        case 0:
          /*S_jump(x);*/ S_call(x); break;
        default:
          S_call(x); break;
        }
        break;
      case '?':
        switch (S_peek(x)) {
        case ']': case '}':
          x->ip++;
        case 0:
          /*S_zjump(x);*/ break;
        default:
          /*S_zcall(x);*/ break;
        }
        break;
      case 'q': exit(0); break;
      /* Memory */
      case 'm': S_malloc(x); break;
      case 'f': S_free(x); break;
      case 'c': S_lit(x, szC); break;
      case ':': S_bfetch(x); break;
      case ';': S_bstore(x); break;
			case '.': S_cfetch(x); break;
			case ',': S_cstore(x); break;
      case 'i': S_inspect(x); break;
      /* Input/output */
			case 'k': x->key(x); break;
			case 'e': x->emit(x); break;
      /* Helpers */
      case 'h':
        switch (S_token(x)) {
        case 'c': S_compare(x); break;
			  case 'a': S_accept(x); break;
			  case 't': S_type(x); break;
        /*case '.': S_repr(x); break;*/
        }
        break;
      /* Block */
      case 'b':
        switch (S_token(x)) {
        case 'a': S_allot(x); break;
        case 'b': S_lit(x, (C)(x->b)); break;
        case 'f': S_find(x); break;
        case 'p': S_parse_name(x); break;
        case 'c': S_create(x); break;
        case ';': S_bcompile(x); break;
        case ',': S_ccompile(x); break;
        case 'h': S_lit(x, (C)(x->b + HERE(x))); break;
        case 'q': S_qcompile(x); break;
        case 's': S_scompile(x); break;
        }
			}
    }
	} while(1);
}

#endif