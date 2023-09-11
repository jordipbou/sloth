#ifndef SLOTH2_H
#define SLOTH2_H

#include<stdio.h> /* fgets, stdin */
#include<stdint.h> /* intptr_t */
#include<stdlib.h> /* malloc, free */
#include<string.h> /* strncpy */

#define V void /* inline void ? */

typedef char B;
typedef intptr_t C;

typedef struct _Symbol {
  struct _Symbol* previous;
  C flags;
  B* interpretation;
  B* compilation;
  C nlen;
  B name[sizeof(C)];
} S;

#define STACK_SIZE 64
#define RSTACK_SIZE 64
#define INPUT_BUFFER_SIZE 255
#define DEFAULT_DICT_SIZE 65536

typedef struct _Context {
  C* s; C sp; C ss;
  B** r; C rp; C rs;
	B* ip;
  C err;
  void (**ext)(struct _Context*);
  C state;
  S* latest;
  B ibuf[INPUT_BUFFER_SIZE];
  C in;
  C here;
  B dict[DEFAULT_DICT_SIZE];
} X;

#include "trace.h"

#define EXT(x, l) (x->ext[l - 'A'])

X* S_init() {
	X* x = malloc(sizeof(X));
  x->s = malloc(STACK_SIZE*sizeof(C));
  x->r = malloc(RSTACK_SIZE*sizeof(C));
	x->sp = x->rp = 0;
  x->ss = STACK_SIZE;
  x->rs = RSTACK_SIZE;
  x->ext = malloc(26*sizeof(C));
  x->ip = 0;
  x->err = 0;
  x->state = 0;
  x->latest = 0;
  x->in = 0;
  x->here = 0;	
  return x;
}

void S_inner(X* x);

B S_peek(X* x) { return x->ip == 0 ? 0 : *x->ip; }
B S_token(X* x) { B tk = S_peek(x); x->ip++; return tk; }
C S_is_digit(B c) { return c >= '0' && c <= '9'; }

#define TS(x) (x->s[x->sp - 1])
#define NS(x) (x->s[x->sp - 2])
#define NNS(x) (x->s[x->sp - 3])

#define S_lit(x, v) (((X*)(x))->s[((X*)(x))->sp++] = (C)(v))

/* Parsing */

V S_parse_quotation(X* x) { 
	C t = 1; 
	S_lit(x, (C)(x->ip)); 
	while (t) { 
    switch (S_token(x)) { 
    case '[': t++; break; 
    case ']': t--; break;
    } 
  }
}

/* Stack instructions */

V S_dup(X* x) { S_lit(x, TS(x)); }
V S_over(X* x) { S_lit(x, NS(x)); }
V S_rot(X* x) { C t = NNS(x); NNS(x) = NS(x); NS(x) = TS(x); TS(x) = t; TS(x); }
V S_swap(X* x) { C t = TS(x); TS(x) = NS(x); NS(x) = t; }
C S_drop(X* x) { return x->s[--x->sp]; }
V S_nip(X* x) { NS(x) = TS(x); x->sp--; }

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

V S_push(X* x) { x->r[x->rp++] = (B*)x->s[--x->sp]; }
V S_pop(X* x) { x->s[x->sp++] = (C)x->r[--x->rp]; }
V S_jump(X* x) { x->ip = (B*)S_drop(x); }
V S_zjump(X* x) { S_swap(x); if (!S_drop(x)) S_jump(x); else S_drop(x); }

#define TC(x, t) (x->ip && t && t != ']' && t != '}')
V S_call(X* x) { B t = S_peek(x); if TC(x, t) x->r[x->rp++] = x->ip; S_jump(x); }

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

V S_branch(X* x) { 
  S_rot(x); 
  if (!S_drop(x)) { S_swap(x); }
  S_drop(x);
  S_call(x);
}

V S_times(X* x) {
  B* q = (B*)S_drop(x);
  C n = S_drop(x);
  for (; n > 0; n--) S_eval(x, q);
}

V S_while(X* x) {
  B* q = (B*)S_drop(x);
  B* c = (B*)S_drop(x); 
  do {
    S_eval(x, c);
    if (S_drop(x)) S_eval(x, q);
    else break;
  } while (1);
}

V S_parse_name(X* x) {
  while (x->in < INPUT_BUFFER_SIZE && 
         x->ibuf[x->in] != 0 &&
         isspace(x->ibuf[x->in])) {
    x->in++;
  }
  S_lit(x, &x->ibuf[x->in]);
  while (x->in < INPUT_BUFFER_SIZE &&
         x->ibuf[x->in] != 0 &&
         !isspace(x->ibuf[x->in])) {
    x->in++;
  }
  S_lit(x, &x->ibuf[x->in] - TS(x));
}

V S_header(X* x) {
  C nlen = S_drop(x);
  B* name = (B*)S_drop(x);
  S* s = (S*)(&x->dict[x->here]);
  x->here += sizeof(S) + nlen - sizeof(C) + 1;
  s->previous = x->latest;
  x->latest = s;
  s->flags = 0;
  s->interpretation = (S*)(&x->dict[x->here]);;
  s->compilation = 0;
  s->nlen = nlen;
  strncpy(s->name, name, nlen);
  s->name[nlen] = 0;    
  S_lit(x, s);
}

V S_create(X* x) {
  S_parse_name(x);
  S_header(x);
}

V S_colon(X* x) {
  printf("Colon\n");
  S_create(x);
  x->state = 1;
  printf("x->latest: %ld\n", x->latest);
  if (x->latest) {
    printf("x->latest->name: %s\n", x->latest->name);
  }
  printf("x->state: %ld\n", x->state);
}

V S_semicolon(X* x) {
  B* i;
  printf("Semicolon\n");
  x->state = 0; 
  for (i = (B*)x->latest; i < &x->dict[x->here]; i++) {
    printf("%c", *i);
  }
}

/* Can be done with a step function as a macro, as it was done? */
/* That way, tracing does not belong to here */
void S_inner(X* x) {
  B l;
  C frame = x->rp;
  do {
    switch (S_peek(x)) {
    case 0: if (!S_return(x, frame)) { return; } break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      l = S_token(x);
      EXT(x, l)(x);
      break;
    default:
      switch (S_token(x)) {
      case '[': S_parse_quotation(x); break;
      /* Literals */
      case '0': S_lit(x, 0); break;
      case '1': S_lit(x, 1); break;
      case '#': S_lit(x, *((C*)x->ip)); x->ip += sizeof(C); break;
      case '@': S_lit(x, (C)(x->ip + ((B)S_token(x)))); break;
      /* Stacks */
      case '_': S_drop(x); break;
      case 's': S_swap(x); break;
      case 'd': S_dup(x); break;
      case 'o': S_over(x); break;
      case 'r': S_rot(x); break;
      case 'n': S_nip(x); break;
      case '(': S_push(x); break;
      case ')': S_pop(x); break;
      /* Arithmetics */
      case '+': S_add(x); break;
      case '-': S_sub(x); break;
      case '*': S_mul(x); break;
      case '/': S_div(x); break;
      case '%': S_mod(x); break;
      /* Bitwise */
      case '!': S_not(x); break;
      case '~': S_invert(x); break;
      case '&': S_and(x); break;
      case '|': S_or(x); break;
      case '^': S_xor(x); break;
      /* Comparison */
      case '<': S_lt(x); break;
      case '=': S_eq(x); break;
      case '>': S_gt(x); break;
      /* Execution */
      case ']': case '}': if (!S_return(x, frame)) { return; } break;
      case 'i': S_call(x); break;
      case 'j': S_jump(x); break;
      case 'z': S_zjump(x); break;
      case 'q': /* Set error */ exit(0); break;
      /* Helpers */
      case '?': S_branch(x); break;
      case 't': S_times(x); break;
      case 'w': S_while(x); break;
      /* Memory */
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      case '.': S_cfetch(x); break;
      case ':': S_bfetch(x); break;
      case ',': S_cstore(x); break;
      case ';': S_bstore(x); break;
      case 'c': S_lit(x, sizeof(C)); break;
      /* Colon */
      case '$': S_colon(x); break;
      case '{': S_semicolon(x); break;
      }
    }
  } while(1);
}

V S_compile(X* x) {
  S* s = (S*)S_drop(x);
  x->dict[x->here] = '#';
  x->here++;
  *((C*)(&x->dict[x->here])) = s->interpretation;
  x->here+=sizeof(C);
  x->dict[x->here] = 'i';
  x->here++;
}

V S_find_name(X* x) {
  C nlen = S_drop(x);
  B* name = (B*)S_drop(x);
  S* s = x->latest;
  while (s) {
    if (s->nlen == nlen && !strncmp(s->name, name, nlen)) {
      break;
    }
    s = s->previous;
  }
  S_lit(x, s);
}

V S_name_interpret(X* x) {
  S* s = (S*)S_drop(x);
  S_lit(x, s->interpretation);
}

V S_name_compile(X* x) {
  S* s = (S*)S_drop(x);
  S_lit(x, s->compilation);
}

V S_dual(X* x) {
  S* s = (S*)S_drop(x);
  S_lit(x, s->compilation != 0);
}

V S_to_number(X* x) {
  C slen = S_drop(x);
  B* str = (B*)S_drop(x);
  B* end;
  C n;
  n = strtol(str, &end, 10);
  if (n == 0 && str == end) {
    /* Error */
  } else {
    S_lit(x, n);
  }
}

V S_interpret(X* x) {
  do {
    S_parse_name(x);
    if (!TS(x)) { S_drop(x); S_drop(x); return; }
    S_over(x); S_over(x);
    S_find_name(x);
    if (TS(x)) {
      S_nip(x); S_nip(x); 
      if (!x->state) {
        S_name_interpret(x);
        S_call(x);
        S_inner(x);
      } else {
        S_dup(x);
        S_dual(x);
        if (S_drop(x)) {
          S_name_compile(x);
          S_call(x);
          S_inner(x);
        } else {
          S_compile(x);
        }
      }
    } else {
      S_drop(x);
      S_to_number(x); 
    }
  } while(1);
}

V S_repl(X* x) {
  while (!x->err) {
    fgets(x->ibuf, INPUT_BUFFER_SIZE, stdin);
    x->in = 0;
    S_interpret(x);
    S_trace(x);
  } 
}

V S_add_primitive(X* x, B* name, B* i, B* c) {
  S* s;
  S_lit(x, name);
  S_lit(x, strlen(name));
  S_header(x);
  s = (S*)S_drop(x); 
  s->interpretation = i;
  s->compilation = c;
}

V S_primitives(X* x) {
  S_add_primitive(x, "dup", "d", 0);
  S_add_primitive(x, "over", "o", 0);
  S_add_primitive(x, "swap", "s", 0);
  S_add_primitive(x, "drop", "_", 0);
  S_add_primitive(x, "rot", "r", 0);
  S_add_primitive(x, "nip", "n", 0); 

  S_add_primitive(x, ">r", "(", 0);
  S_add_primitive(x, "r>", ")", 0);

  S_add_primitive(x, "+", "+", 0);
  S_add_primitive(x, "-", "-", 0);
  S_add_primitive(x, "*", "*", 0);
  S_add_primitive(x, "/", "/", 0);
  S_add_primitive(x, "mod", "%", 0);

  S_add_primitive(x, "not", "!", 0);
  S_add_primitive(x, "invert", "~", 0);
  S_add_primitive(x, "and", "&", 0);
  S_add_primitive(x, "or", "|", 0);
  S_add_primitive(x, "xor", "^", 0);

  S_add_primitive(x, "<", "<", 0);
  S_add_primitive(x, "=", "=", 0);
  S_add_primitive(x, ">", ">", 0);

  S_add_primitive(x, "exit", "}", 0);
  S_add_primitive(x, "execute", "i", 0);
  S_add_primitive(x, "branch", "j", 0);
  S_add_primitive(x, "0branch", "z", 0);

  S_add_primitive(x, ":", "$", 0);
  S_add_primitive(x, ";", 0, "{");

 /* 
      case 'f': S_free(x); break;
      case 'm': S_malloc(x); break;
      case '.': S_cfetch(x); break;
      case ':': S_bfetch(x); break;
      case ',': S_cstore(x); break;
      case ';': S_bstore(x); break;
      case 'c': S_lit(x, sizeof(C)); break;
  */
}

#endif