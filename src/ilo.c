/* Experimental SLOTH/ILO mashup */

/***************************************************************
   crc's _ _
        (_) | ___
        | | |/ _ \  a tiny virtual computer
        | | | (_) | 64kw RAM, 32-bit, Dual Stack, MISC
        |_|_|\___/  ilo.c (c) charles childers
 **************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define T(x) x->ds[x->sp]    /* Top of Data Stack    */
#define N(x) x->ds[x->sp-1]  /* Next on Data Stack   */
#define R(x) x->as[x->rp]    /* Top of Address Stack */
#define V void
#define I int
#define C char

typedef struct _Context {
  I ip;
  I sp;
  I rp;
  I ds[33];
  I as[257];
  I m[65536];
  C* blocks;
  C* rom;
  I a, b, f, s, d, l;
  C i[1];
} X;

V push(X* x, I v) { x->ds[x->sp + 1] = v; x->sp += 1; }
I pop(X* x) { x->sp -= 1; return x->ds[x->sp + 1]; }

V load_image(X* x) {
  x->f = open(x->rom, O_RDONLY, 0666);
  if (!x->f) { return; }
  read(x->f, &x->m, 65536 * 4);
  close(x->f);
  x->ip = x->sp = x->rp = 0;
}

V save_image(X* x) {
  x->f = open(x->rom, O_WRONLY, 0666);
  write(x->f, &x->m, 65536 * 4);
  close(x->f);
}

V block_common(X* x) {
  x->b = pop(x); /* block buffer */
  x->a = pop(x); /* block number */
  lseek(x->f, 4096 * x->a, SEEK_SET);
}

V read_block(X* x) {
  x->f = open(x->blocks, O_RDONLY, 0666);
  block_common(x);
  read(x->f, x->m + x->b, 4096);
  close(x->f);
}

V write_block(X* x) {
  x->f = open(x->blocks, O_WRONLY, 0666);
  block_common(x);
  write(x->f, x->m + x->b, 4096);
  close(x->f);
}

V save_ip(X* x) { x->rp += 1; R(x) = x->ip; }
V symmetric(X* x) { if (x->b >= 0 && N(x) < 0) { T(x) += 1; N(x) -= x->b; } }

V li(X* x) { x->ip += 1; push(x, x->m[x->ip]); }
V du(X* x) { push(x, T(x)); }
V dr(X* x) { x->ds[x->sp] = 0; x->sp -= 1; }
V sw(X* x) { x->a = T(x); T(x) = N(x); N(x) = x->a; }
V pu(X* x) { x->rp += 1; R(x) = pop(x); }
V po(X* x) { push(x, R(x)); x->rp -= 1; }
V ju(X* x) { x->ip = pop(x) - 1; }
V ca(X* x) { save_ip(x); x->ip = pop(x) - 1; }
V cc(X* x) { x->a = pop(x); if (pop(x)) { save_ip(x); x->ip = x->a - 1; } }
V cj(X* x) { x->a = pop(x); if (pop(x)) { x->ip = x->a - 1; } }
V re(X* x) { x->ip = R(x); x->rp -= 1; }
V eq(X* x) { N(x) = (N(x) == T(x)) ? -1 : 0; x->sp -= 1; }
V ne(X* x) { N(x) = (N(x) != T(x)) ? -1 : 0; x->sp -= 1; }
V lt(X* x) { N(x) = (N(x) <  T(x)) ? -1 : 0; x->sp -= 1; }
V gt(X* x) { N(x) = (N(x) >  T(x)) ? -1 : 0; x->sp -= 1; }
V fe(X* x) { T(x) = x->m[T(x)]; }
V st(X* x) { x->m[T(x)] = N(x); x->sp -= 2; }
V ad(X* x) { N(x) += T(x); x->sp -= 1; }
V su(X* x) { N(x) -= T(x); x->sp -= 1; }
V mu(X* x) { N(x) *= T(x); x->sp -= 1; }
V di(X* x) { x->a = T(x); x->b = N(x); T(x) = x->b / x->a; N(x) = x->b % x->a; symmetric(x); }
V an(X* x) { N(x) = T(x) & N(x); x->sp -= 1; }
V or(X* x) { N(x) = T(x) | N(x); x->sp -= 1; }
V xo(X* x) { N(x) = T(x) ^ N(x); x->sp -= 1; }
V sl(X* x) { N(x) = N(x) << T(x); x->sp -= 1; }
V sr(X* x) { N(x) = N(x) >> T(x); x->sp -= 1; }
V cp(X* x) { x->l = pop(x); x->d = pop(x); x->s = T(x); T(x) = -1;
         while (x->l) { if (x->m[x->d] != x->m[x->s]) { T(x) = 0; }
                     x->l -= 1; x->s += 1; x->d += 1; } }
V cy(X* x) { x->l = pop(x); x->d = pop(x); x->s = pop(x);
         while (x->l) { x->m[x->d] = x->m[x->s]; x->l -= 1; x->s += 1; x->d += 1; } }
V ioa(X* x) { x->i[0] = (char)pop(x); write(1, &x->i, 1); }
V iob(X* x) { read(0, &x->i, 1); push(x, x->i[0]); }
V ioc(X* x) { read_block(x); }
V iod(X* x) { write_block(x); }
V ioe(X* x) { save_image(x); }
V iof(X* x) { load_image(x); x->ip = -1; }
V iog(X* x) { x->ip = 65536; }
V ioh(X* x) { push(x, x->sp); push(x, x->rp); }
V io(X* x) {
  switch (pop(x)) {
    case 0: ioa(x); break;  case 1: iob(x); break;
    case 2: ioc(x); break;  case 3: iod(x); break;
    case 4: ioe(x); break;  case 5: iof(x); break;
    case 6: iog(x); break;  case 7: ioh(x); break;
    default: break;
  }
}

/* Using a switch here instead of a jump table to avoid   */
/* some issues w/relocation stuff when building w/o libc  */

V process(X* x, I o) {
  switch (o) {
    case  0:        break;   case  1: li(x); break;
    case  2: case 'd': du(x); break;   case  3: case '_': dr(x); break;
    case  4: case 's': sw(x); break;   case  5: case '(': pu(x); break;
    case  6: case ')': po(x); break;   case  7: case 'j': ju(x); break;
    case  8: case '$': ca(x); break;   case  9: case '?': cc(x); break;
    case 10: case 'i': cj(x); break;   case 11: case '#': re(x); break;
    case 12: case '=': eq(x); break;   case 13: case '~': ne(x); break;
    case 14: case '<': lt(x); break;   case 15: case '>': gt(x); break;
    case 16: case '@': fe(x); break;   case 17: case '!': st(x); break;
    case 18: case '+': ad(x); break;   case 19: case '-': su(x); break;
    case 20: case '*': mu(x); break;   case 21: case '/': di(x); break;
    case 22: case '&': an(x); break;   case 23: case '|': or(x); break;
    case 24: case '^': xo(x); break;   case 25: case 'l': sl(x); break;
    case 26: case 'r': sr(x); break;   case 27: case 'm': cp(x); break;
    case 28: case 'y': cy(x); break;   case 29: io(x); break;
    default: break;
  }
}

V process_bundle(X* x, I opcode) {
  process(x, opcode & 0xFF);
  process(x, (opcode >> 8) & 0xFF);
  process(x, (opcode >> 16) & 0xFF);
  process(x, (opcode >> 24) & 0xFF);
}

V execute(X* x) {
  while (x->ip < 65536) {
    process_bundle(x, x->m[x->ip]);
    x->ip += 1;
  }
}

#ifndef NOSTDLIB

I main(I argc, C **argv) {
  X* x = malloc(sizeof(X));
  x->blocks = (argc > 1) ? argv[1] : "src/ilo.blocks";
  x->rom    = (argc > 2) ? argv[2] : "src/ilo.rom";
  load_image(x);
  execute(x);
  for (; x->sp > 0; x->sp -= 1) printf(" %d", x->ds[x->sp]); printf("\n");
  return 0;
}

#else

I main(I argc, C **argv) {
  X* x = malloc(sizeof(X));
  
  x->blocks = "ilo.blocks";
  x->rom = "ilo.rom";
  load_image(x);
  execute(x);
  return 0;
}

#endif
