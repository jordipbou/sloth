#include<stdio.h>
#include"vm.h"

V hello(X* x) { printf("Hello world!\n"); }

int main(int argc, char** argv) {
	char* r;
  int i;
  B buf[255];
	X* x = init_SLOTH(init_VM(init_MEM()));
  /*EXT(x, 'H') = &hello;*/
	while (1) {
    printf("> ");
	  r = fgets(buf, 255, stdin);
    evaluate(x, buf);
		if (!x->err) {
			for (i = 0; i < x->dp; i++) {
			  printf("%ld ", x->d[i]);
			}
			printf("Ok\n");
		} else {
			printf("ERROR: %ld\n", x->err);
			reset_context(x);
		}
	}
}
