#include<stdio.h>
#include"vm.h"
#include"symbols.h"

int main(int argc, char** argv) {
  B buf[255];
  /*X* x = S_forth();*/
	/*X* x = S_init();*/
	X* x = S_env_init(S_init());
	while (1) {
	  fgets(buf, 255, stdin);
		/*S_evaluate(x, buf);*/
		S_eval(x, buf);
	}
}
