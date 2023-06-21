/* TODO: Add error primitives */
/* TODO: Add dictionary (memory) ? */

/* TODO: Create languages on top */

#ifndef SLOTH_VM
#define SLOTH_VM

#include<stdint.h>
#include<stdlib.h>
#include<string.h>

typedef char B;
typedef int16_t W;
typedef int32_t H;
typedef intptr_t I;

#define STACK_SIZE 64
#define RSTACK_SIZE 64
#define DICT_SIZE 4096

typedef struct _X { 
  I s[STACK_SIZE]; I sp; I ss;
  B* r[RSTACK_SIZE]; I rp; I rs;
  B d[DICT_SIZE]; I dp; I ds;
  void (*k)(struct _X*);
  void (*e)(struct _X*);
  void (*ext[26])(struct _X*);
  I err;
  I tr;
} X;

#define KEY(x) (x->k)
#define EMIT(x) (x->e)
#define EXT(x, l) (x->ext[l - 'A'])
#define ERROR(x) (x->err)

X* S_init() {
	X* x = malloc(sizeof(X));
	x->sp = x->rp = x->dp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ds = DICT_SIZE;
  x->err = 0;
  x->tr = 0;
	return x;
}

#define FRAME(x) (x->rp - 1)
#define IP(x) (x->r[x->rp - 1])
#define NEXT(x, i) (x->r[x->rp - 1] += i)

B S_peek(X* x) { return IP(x) == 0 ? 0 : *(IP(x)); }
B S_token(X* x) { B tk = S_peek(x); NEXT(x, 1); return tk; }
I S_is_digit(B c) { return c >= '0' && c <= '9'; }

void S_push_R(X* x, B* a) { x->r[x->rp++] = a; }
I S_return(X* x, I f) { x->rp--; return x->rp == f; }

void S_inner(X* x);

void S_eval(X* x, B* q) {
  /* TODO: Condition for tail call elimination */
	S_push_R(x, IP(x));
	IP(x) = q;
  /* TODO: If tracing? trace else inner */
	S_inner(x);
}

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])
#define DROP(x) (x->sp--)

void S_push(X* x, I v) { x->s[x->sp++] = v; }
I S_pop(X* x) { return x->s[--x->sp]; }

#define OP2(x, o) NS(x) = NS(x) o TS(x); DROP(x)

void S_add(X* x) { OP2(x, +); }
void S_sub(X* x) { OP2(x, -); }
void S_mul(X* x) { OP2(x, *); }
void S_div(X* x) { OP2(x, /); }
void S_mod(X* x) { OP2(x, %); }

void S_lt(X* x) { OP2(x, <); }
void S_eq(X* x) { OP2(x, ==); }
void S_gt(X* x) { OP2(x, >); }

void S_and(X* x) { OP2(x, &); }
void S_or(X* x) { OP2(x, |); }
void S_not(X* x) { TS(x) = !TS(x); }

void S_swap(X* x) { I t = TS(x); TS(x) = NS(x); NS(x) = t; }
void S_dup(X* x) { S_push(x, TS(x)); }
void S_rot(X* x) { I t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
void S_over(X* x) { S_push(x, NS(x)); }
void S_drop(X* x) { S_pop(x); }

void S_jump(X* x) { IP(x) = (B*)S_pop(x); }
void S_call(X* x) { S_push_R(x, IP(x)); S_jump(x); }

void S_exec_i(X* x) { B* q = (B*)S_pop(x); S_eval(x, q); }
void S_exec_x(X* x) { B* q = (B*)S_pop(x); S_push(x, (I)q); S_eval(x, q); }

void S_ifthen(X* x) { B* f = (B*)S_pop(x); B* t = (B*)S_pop(x); if (S_pop(x)) S_eval(x, t); else S_eval(x, f); }
void S_times(X* x) { B* q = (B*)S_pop(x); I t = S_pop(x); for (;t > 0; t--) { S_eval(x, q); } }

void S_br(X* x, B* c, B* t, B* r1, B* r2) { S_eval(x, c); if (S_pop(x)) { S_eval(x, t); return; } S_eval(x, r1); S_br(x, c, t, r1, r2); S_swap(x); S_br(x, c, t, r1, r2); S_eval(x, r2); }
void S_bin_rec(X* x) { B* r2 = (B*)S_pop(x); B* r1 = (B*)S_pop(x); B* t = (B*)S_pop(x); B* c = (B*)S_pop(x); S_br(x, c, t, r1, r2); }

void S_lit_i8(X* x) { S_push(x, (I)*((B*)IP(x))); NEXT(x, 1); }
void S_lit_i16(X* x) { S_push(x, (I)*((W*)IP(x))); NEXT(x, 2); }
void S_lit_i32(X* x) { S_push(x, (I)*((H*)IP(x))); NEXT(x, 4); }
void S_lit_i64(X* x) { S_push(x, *((I*)IP(x))); NEXT(x, 8); }

void S_allocate(X* x) { S_push(x, (I)malloc(S_pop(x))); }
void S_free(X* x) { free((void*)S_pop(x)); }

void S_store_i8(X* x) { B* a = (B*)S_pop(x); *a = (B)S_pop(x); }
void S_store_i16(X* x) { W* a = (W*)S_pop(x); *a = (W)S_pop(x); }
void S_store_i32(X* x) { H* a = (H*)S_pop(x); *a = (H)S_pop(x); }
void S_store_i64(X* x) { I* a = (I*)S_pop(x); *a = S_pop(x); }

void S_fetch_i8(X* x) { S_push(x, *((B*)S_pop(x))); }
void S_fetch_i16(X* x) { S_push(x, *((W*)S_pop(x))); }
void S_fetch_i32(X* x) { S_push(x, *((H*)S_pop(x))); }
void S_fetch_i64(X* x) { S_push(x, *((I*)S_pop(x))); }

void S_rel_i8(X* x) { S_push(x, (I)(IP(x) + (I)*((B*)IP(x)))); NEXT(x, 1); }
void S_rel_i16(X* x) { S_push(x, (I)(IP(x) + (I)*((W*)IP(x)))); NEXT(x, 2); }
void S_rel_i32(X* x) { S_push(x, (I)(IP(x) + (I)*((H*)IP(x)))); NEXT(x, 4); }
void S_rel_i64(X* x) { S_push(x, (I)(IP(x) + *((I*)IP(x)))); NEXT(x, 8); }

/* Parsing */

void S_parse_literal(X* x) {
	I n = 0;
	while (S_is_digit(S_peek(x))) { n = 10*n + (S_token(x) - '0'); }
	S_push(x, n);
}

void S_parse_quotation(X* x) { 
	I t = 1; B c; 
	S_push(x, (I)(++IP(x))); 
	while (t) { switch (S_token(x)) { case '[': t++; break; case ']': t--; break; } } 
}

/* Input/output */

void S_read(X* x) {
  I n = S_pop(x);
  B* b = (B*)S_pop(x);
  B k;
  I c = 0;
  for (;n > 0; n--, c++) {
    KEY(x)(x);
    k = (B)S_pop(x);
    if (k == 10) {
      S_push(x, c); 
      return;
    } else if (k == 127) {
      /* TODO */ 
    } else {
      S_push(x, k);
      EMIT(x)(x);
      b[c] = k;
    }
  }
}

void S_print(X* x) {
  I l = S_pop(x);
  B* b = (B*)S_pop(x);
  I i;
  for (i = 0; i < l; i++) {
    S_push(x, b[i]);
    EMIT(x)(x);
  }
}

I S_dump_S(B* s, X* x) {
  I i, t, n = 0;
  for (i = 0; i < x->sp; i++) {
    s += t = sprintf(s, "%ld ", x->s[i]); n+= t;
  }
  return n;
}

I S_dump_R(B* s, X* x) {
  I i, t, n = 0;
  B* j;
  for (i = x->rp - 1; i >= 0; i--) {
    s += sprintf(s, " : "); n += 3;
    t = 0;
    /* Show caller on return stack */
    j = (i == (x->rp - 1) ? x->r[i] : (x->r[i] - 1));
    for (; *j != 0 && *j != 10 && t >= 0; j++) {
      *(s++) = *j; n++;
      if (*j == '[') t++;
      if (*j == ']') t--;
    }
  }
  return n;
}

I S_dump_X(B* s, X* x, I nl) {
  I t, n = 0;
  s += t = S_dump_S(s, x); n += t;
  s += t = S_dump_R(s, x); n += t;
  if (nl) { *s++ = '\n'; n++; }
  return n;
}

/* Inner interpreter */

#define TYPED_CASE(op, f) \
case op: \
  switch (S_token(x)) { \
  case 'b': f ## _i8(x); break; \
  case 'w': f ## _i16(x); break; \
  case 'h': f ## _i32(x); break; \
  case 'i': f ## _i64(x); break; \
  case 'c': /* Platform dependent 32/64 bits */ break; \
  /* case 'f': f ## _f32(x); break; */ \
  /* case 'd': f ## _f64(x); break; */ \
} \
break; \

/* TODO: Make STEP(x)? One variable for tracing its easier, isn't it? */
void S_inner(X* x) {
  B buf[255];
	I frame = FRAME(x);
	I l;
	B* j;
	do {
    if (x->tr) {
      memset(buf, 0, sizeof(buf));
		  l = S_dump_X(buf, x, 1);
      S_push(x, (I)buf);
      S_push(x, l);
      S_print(x);
    }
		switch (S_peek(x)) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': 
      S_parse_literal(x); break;
		case '[': 
      S_parse_quotation(x); break;
		case 0: case ']': 
      if (S_return(x, frame)) return; 
      break;
    case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, S_token(x))(x);
      break;
		default:
			switch (S_token(x)) {
      TYPED_CASE('#', S_lit);
      TYPED_CASE('!', S_store);
      TYPED_CASE('@', S_fetch);
      TYPED_CASE('~', S_rel);
      case 'c':
        switch (S_token(x)) {
        case 's': 
          switch (S_token(x)) {
          case 'a': S_push(x, (I)(&x->s)); break;
          case 'p': S_push(x, (I)(&x->sp)); break;
          case 'z': S_push(x, (I)(&x->ss)); break;
          }
          break;
        case 'r': 
          switch (S_token(x)) {
          case 'a': S_push(x, (I)(&x->r)); break;
          case 'p': S_push(x, (I)(&x->rp)); break;
          case 'z': S_push(x, (I)(&x->rs)); break;
          }
          break; 
        case 'd': 
          switch (S_token(x)) {
          case 'a': S_push(x, (I)(&x->d)); break;
          case 'p': S_push(x, (I)(&x->dp)); break;
          case 'z': S_push(x, (I)(&x->ds)); break;
          }
          break;
        case 'e': S_push(x, (I)(&x->err)); break;
        case 't': S_push(x, (I)(&x->tr)); break;
        }
        break;
      case 'm': S_allocate(x); break;
      case 'f': S_free(x); break;
      case 'k': KEY(x)(x); break;
      case 'e': EMIT(x)(x); break;
      case 'a': S_read(x); break;
      case 'p': S_print(x); break;
			case '+': S_add(x); break;
			case '-': S_sub(x); break;
			case '*': S_mul(x); break;
			case '/': S_div(x); break;
			case '%': S_mod(x); break;
			case '<': S_lt(x); break;
			case '=': S_eq(x); break;
			case '>': S_gt(x); break;
			case '&': S_and(x); break;
			case '|': S_or(x); break;
			case '_': S_not(x); break;
			case 's': S_swap(x); break;
			case 'd': S_dup(x); break;
			case 'r': S_rot(x); break;
			case 'o': S_over(x); break;
			case '\\': S_drop(x); break;
      case '^': S_jump(x); break;
      case '$': S_call(x); break;
			case 'i': S_exec_i(x); break;
			case 'x': S_exec_x(x); break;
			case '?': S_ifthen(x); break;
			case 't': S_times(x); break;
			case 'b': S_bin_rec(x); break;
			case 'q': exit(0); break;
			}
		}
	} while(1);
}

#endif