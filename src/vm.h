/* TODOS:
  - Find the relation between a packed memory and directly executing strings (maybe don't used packed memory? But, in high level languages everything is stored as objects, not pointers, or packing its used or a table holding references)
  The main problem is the index that points to there (here). Is it byte based or int based?
	- Test shoud be added to ensure call/return/eval/inner works correctly
	- Block could be (and should be) reutilized from parse and trace (and compile if implemented)
	- More basic functions can be added, there are enough ASCII characters left and there's no reason to not do it
 - Adapt trace to use strings
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

V S_halt(X* x) { x->err = -1; }
V S_quit(X* x) { exit(0); }
V S_lit(X* x) { S_push(x, ((*x->ip) << 8 + *(x->ip + 1)) & 0xEFFF); x->ip++; }

#define T(x) (x->ds[x->sp - 1])
#define N(x) (x->ds[x->sp - 2])
#define NN(x) (x->ds[x->sp - 3])
#define R(x) (x->as[x->rp - 1])
/*
#define LBRACE(x, i) if (*i == '{') t++;
#define RBRACE(x, i) if (*i == '}') t--;
#define EOL(x, i) if (i == 0 || *i == 0) return;
#define BLOCK(x, i) { C t = 1; while (t) { EOL(x, i); LBRACE(x, i); RBRACE(x, i); i++; } }
*/
/*V S_block(X* x, B** c) { C t = 1; while (t) { EOL(x, *c); LBRACE(x, *c); RBRACE(x, *c); *c++; } }*/
/*V S_parse_block(X* x) { x->ip += 1; S_push(x, x->ip); S_block(x, &x->ip); }*/
/*V S_parse_block(X* x) { S_push(x, x->ip + 1); BLOCK(x, x->ip); }*/

#define ST(v, b) \
  for (;t && v && *v; v++) { \
    if (*v == '{') { t++; } \
    if (*v == '}' || *v == '\\') { t--; } \
    if (*v != 10) { b; } \
  }

V S_parse_block(X* x) {	C t = 1; x->ip += 1; S_push(x, x->ip); ST(x->ip,); x->ip--; }

V S_char(X* x) { x->ip += 1; S_push(x, *x->ip); }

V S_dup(X* x) { S_push(x, T(x)); }
C S_drop(X* x) { return x->ds[--x->sp]; }
V S_swap(X* x) { C t = T(x); T(x) = N(x); N(x) = t; }
V S_over(X* x) { S_push(x, N(x)); }

V S_to_R(X* x) { x->as[x->rp++] = (B*)x->ds[--x->sp]; }
V S_from_R(X* x) { x->ds[x->sp++] = (C)x->as[--x->rp]; }

V S_save_ip(X* x) { x->as[x->rp++] = x->ip; }
V S_jump(X* x) { x->ip = (B*)S_drop(x); }
V S_call(X* x) { S_save_ip(x); S_jump(x); }
V S_ccall(X* x) { S_swap(x); if (S_drop(x)) S_call(x); else S_drop(x); }
V S_cjump(X* x) { S_swap(x); if (S_drop(x)) S_jump(x); else S_drop(x); }
V S_return(X* x) { if (x->rp > 0) { x->ip = R(x); x->rp--; } else { x->rp = 0; x->ip = 0; } }

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
/* Times is used for testing quotations mainly, should it stay on VM or on combinators extension? */
V S_times(X* x) {
	B* q = (B*)S_drop(x);
	C n = S_drop(x);
	for (;n > 0; n--) { S_eval(x, q); }
}

#define DL C i, t, n; i = t = n = 0
#define DC(a) { b += t = sprintf(b, "%ld ", a); n += t; }
#define DB(a) { b += t = sprintf(b, "%c", a); n += t; }
#define DS(a) { b += t = sprintf(b, "%s", a); n += t; }
#define DR return n

C D_co(X* x, B* c) { C t = 1; if (c == 0) DC(0); ST(c, DB(*c)); DR;}
C D_ds(X* x, B* b) { DL; for (;i < x->sp; i++) DC(x->ds[i]); DR; }
V S_trace(X* x) {
	C i;
  B buf[255];
  memset(buf, 0, 255);
  D_ds(x, buf);
  printf("%s", buf);
  printf("| ");
  memset(buf, 0, 255);
  D_co(x, x->ip);
	/*S_trace_code(x, x->ip);*/
	for (i = x->rp - 1; i >= 0; i--) { DS(" : "); D_co(x, x->as[i]); } /*S_trace_code(x, x->as[i]); } */
	printf("\n");
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
		case 0: case '}': S_return(x); break;
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

V S_inner(X* x) { C rp = x->rp; while(!x->err && x->rp >= rp && x->ip) { S_step(x); x->ip += 1; } }

V S_eval(X* x, B* s) { S_push(x, s); S_call(x); S_inner(x); /*S_return(x);*/ }

X* S_init() {
	S* s = malloc(sizeof(S));
	X* x = malloc(sizeof(X));
	return x;
}

/* 
#include "trace.h"

void S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); if (x->ip) x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define S_lit(x, v) (x->ds[x->sp++] = (C)(v))

V S_compile_block(X* x) {
	C t = 1;
	B* s = x->ip;
	S_lit(x, x->m->data + x->m->here);
	while (t) { 
    switch (S_token(x)) { 
    case '[': t++; break; 
    case ']': t--; break;
    }
  }
  t = x->ip - s;	
	strncpy(x->m->data + x->m->here, s, t);
	x->m->here += t;
}

V S_parse_string(X* x) {
	C l = 0;
	S_lit(x, x->ip);
	while (S_token(x) != '"') { l++; }
	S_lit(x, l);
}

V S_parse_number(X* x) {
	C n = 0;
	C d = 1;
	C tk = S_token(x);
	while (tk > 47 && tk < 58) {
		n = n*d + (tk - 48);
		d *= 10;
		tk = S_token(x);
	}
	S_lit(x, n);
}

V S_over(X* x) { S_lit(x, NS(x)); }
V S_rot(X* x) { C t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
V S_nip(X* x) { NS(x) = TS(x); --x->sp; }

V S_add(X* x) { NS(x) = NS(x) + TS(x); --x->sp; }
V S_sub(X* x) { NS(x) = NS(x) - TS(x); --x->sp; }
V S_mul(X* x) { NS(x) = NS(x) * TS(x); --x->sp; }
V S_div(X* x) { NS(x) = NS(x) / TS(x); --x->sp; }
V S_mod(X* x) { NS(x) = NS(x) % TS(x); --x->sp; }

V S_and(X* x) { NS(x) = NS(x) & TS(x); --x->sp; }
V S_or(X* x) { NS(x) = NS(x) | TS(x); --x->sp; }
V S_xor(X* x) { NS(x) = NS(x) ^ TS(x); --x->sp; }
V S_not(X* x) { TS(x) = !TS(x); }
V S_invert(X* x) { TS(x) = ~TS(x); }

V S_lt(X* x) { NS(x) = NS(x) < TS(x); --x->sp; }
V S_eq(X* x) { NS(x) = NS(x) == TS(x); --x->sp; }
V S_gt(X* x) { NS(x) = NS(x) > TS(x); --x->sp; }

V S_jump(X* x) { x->ip = (B*)S_drop(x); }
V S_zjump(X* x) { S_swap(x); if (!S_drop(x)) S_jump(x); else S_drop(x); }

#define TC(x, t) (x->ip && t && t != ']' && t != '}')
V S_call(X* x) { B t = S_peek(x); if TC(x, t) x->r[x->rp++] = x->ip; S_jump(x); }

V S_ccall(X* x) { C f = S_drop(x); B* a = (B*)S_drop(x); 

B* S_return(X* x, C f) {
  if (x->rp > f && x->rp > 0) {
    x->ip = x->r[--x->rp];
  }	else {
    x->ip = 0;
  }
  return x->ip;
}

V S_eval(X* x, B* q) { S_lit(x, (C)q); S_call(x); S_inner(x); }

V S_bstore(X* x) { B* a = (B*)S_drop(x); *a = (B)S_drop(x); }
V S_cstore(X* x) { C* a = (C*)S_drop(x); *a = S_drop(x); }
V S_bfetch(X* x) { S_lit(x, *((B*)S_drop(x))); }
V S_cfetch(X* x) { S_lit(x, *((C*)S_drop(x))); }

V S_malloc(X* x) { S_lit(x, (C)malloc(S_drop(x))); }
V S_free(X* x) { free((void*)S_drop(x)); }

V S_branch(X* x) { S_rot(x); if (!S_drop(x)) { S_swap(x); } S_drop(x); S_call(x); }
V S_times(X* x) { B* q = (B*)S_drop(x); C n = S_drop(x); for (; n > 0; n--) S_eval(x, q); }
V S_while(X* x) {
  B* q = (B*)S_drop(x);
  B* c = (B*)S_drop(x); 
  do {
    S_eval(x, c);
    if (S_drop(x)) S_eval(x, q);
    else break;
  } while (1);
}


V load_image(X* x) {
	C f = open(x->s->r, O_RDONLY, 0666);
	if (!f) { S_lit(x, -1:); return; }
  read(f, &x->s->m, DICT_SIZE * 4);
  close(f);	
	x->ip = x->sp = x->rp = 0;
}


V block_common(X* x) {
  x->b = pop(x);
  x->a = pop(x);
  lseek(x->f, 4096 * x->a, SEEK_SET);
}

V read_block(X* x) {
	C a = S_drop(x);
	C b = S_drop(x);
  C f = open(x->s->b, O_RDONLY, 0666);
	lseek(f, 4096 * a, SEEK_SET);
  read(f, x->s->m + b, 4096);
  close(f);
}

V write_block(X* x) {
	C a = S_drop(x);
	C b = S_drop(x);
  C f = open(x->s->b, O_WRONLY, 0666);
	lseek(f, 4096 * x->a, SEEK_SET);
  write(f, x->s->m + b, 4096);
  close(f);
}

V S_Hliteral(X* x) { x->ip += 1; S_lit(x, x->m[x->ip]); }

void S_step(X* x, B token) {
  B l;
  C frame = x->rp;
  do {
    S_trace(x);
    switch (S_peek(x)) {
    case 0: if (!S_return(x, frame)) { return; } break;
		case ' ': return; break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      EXT(x, S_token(x))(x);
      break;
    default:
      switch (S_token(x)) {
			case 0: break;
			case 1: S_Hliteral(x); break;
      case 2: case 'd': S_dup(x); break;
      case 3: case '_': S_drop(x); break;
      case 4: case 's': S_swap(x); break;
      case 5: case '(': S_push(x); break;
      case 6: case ')': S_pop(x); break;
      case 7: case 'j': S_jump(x); break;
      case 8: case '$': S_call(x); break;
			case 9: S_ccall(x); break;
      case '{': S_parse_block(x); break;
			case '[': S_compile_block(x); break;
      case '0': S_lit(x, 0); break;
      case '1': S_lit(x, 1); break;
      case '#': S_parse_number(x); break; 
      case '\'': S_lit(x, S_token(x)); break;
			case '"': S_parse_string(x); break;
      case 'o': S_over(x); break;
      case 'r': S_rot(x); break;
			case 'n': S_nip(x); break;
      case '+': S_add(x); break;
      case '-': S_sub(x); break;
      case '*': S_mul(x); break;
      case '/': S_div(x); break;
      case '%': S_mod(x); break;
      case '!': S_not(x); break;
      case '~': S_invert(x); break;
      case '&': S_and(x); break;
      case '|': S_or(x); break;
      case '^': S_xor(x); break;
      case '<': S_lt(x); break;
      case '=': S_eq(x); break;
      case '>': S_gt(x); break;
      case '}': if (!S_return(x, frame)) { return; } break;
      case 'z': S_zjump(x); break;
      case 'q': exit(0); break;
      case '?': S_branch(x); break;
      case 't': S_times(x); break;
      case 'w': S_while(x); break;
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      case '.': S_cfetch(x); break;
      case ':': S_bfetch(x); break;
      case ',': S_cstore(x); break;
      case ';': S_bstore(x); break;
      case 'c': S_lit(x, sizeof(C)); break;
      }
    }
  } while(1);
}

V S_step(X* x, B tk) {
  switch (tk) {
    case  0:        break;   case  1: li(x); break;
    case  2: du(x); break;   case  3: dr(x); break;
    case  4: sw(x); break;   case  5: pu(x); break;
    case  6: po(x); break;   case  7: ju(x); break;
    case  8: ca(x); break;   case  9: cc(x); break;
    case 10: cj(x); break;   case 11: re(x); break;
    case 12: eq(x); break;   case 13: ne(x); break;
    case 14: lt(x); break;   case 15: gt(x); break;
    case 16: fe(x); break;   case 17: st(x); break;
    case 18: ad(x); break;   case 19: su(x); break;
    case 20: mu(x); break;   case 21: di(x); break;
    case 22: an(x); break;   case 23: or(x); break;
    case 24: xo(x); break;   case 25: sl(x); break;
    case 26: sr(x); break;   case 27: cp(x); break;
    case 28: cy(x); break;   case 29: io(x); break;
    default: break;
  }
}

V S_process_bundle(X* x, H opcode)
  S_step(x, opcode & 0xFF);
  S_step(x, (opcode >> 8) & 0xFF);
  S_step(x, (opcode >> 16) & 0xFF);
  S_step(x, (opcode >> 24) & 0xFF);
}

V S_inner(X* x) {
  while (x->ip < DICT_SIZE) {
		S_process_bundle(x, x->m[x->ip]);
    x->ip += 1;
  }
}

X* S_init() {
	X* x = malloc(sizeof(X));
	x->sp = x->rp = 0;
  x->ip = 0;
  x->err = 0;
	x->m = malloc(sizeof(M));
 
	return x;
}
*/
#endif
