#include<stdio.h>
/*
#include"sloth.h"
*/
#include"vm.h"

int main(int argc, char** argv) {
  B buf[255];
	C i;
  /*X* x = S_forth();*/
	X* x = S_init();
	/*X* x = S_init();
	S_env_init(x);
	*/
	/*S_forth_init(x);*/
	/*X* x = S_sloth();*/
  printf("> ");
	while (1) {
	  fgets(buf, 255, stdin);
		/*S_evaluate(x, buf);*/
		/*S_eval(x, buf);*/
		S_ev(x, buf);
    /*S_evaluate(x, buf);*/
		/*
		for (i = 0; i < x->sp; i++) {
			printf("%ld ", x->ds[i]);
		}
		printf("Ok\n");
		*/
    S_tr(x);
		printf("> "); /* S_trace(x); */
	}
}
