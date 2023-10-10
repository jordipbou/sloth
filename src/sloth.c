#include<stdio.h>
#include"sloth.h"

V hello(X* x) { printf("Hello world!\n"); }

int main(int argc, char** argv) {
  B buf[255];
	X* x = init_ANS_FORTH();
  /*EXT(x, 'H') = &hello;*/
	while (1) {
    printf("> ");
	  fgets(buf, 255, stdin);
    /* evaluate(x, buf); */
    evaluate(x, buf);
    dump_context(x);
	}
}
