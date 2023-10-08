#include<stdio.h>
/*
#include"sloth.h"
*/
#include"vm.h"

int main(int argc, char** argv) {
  B buf[255];
	X* x = init_SLOTH();
	while (1) {
    printf("> ");
	  fgets(buf, 255, stdin);
    S_evaluate(x, buf);
    dump_context(x);
	}
}
