#include<stdio.h>
#include"sloth.h"

int main(int argc, char** argv) {
  B buf[255];
  /*X* x = S_forth();*/
	/*X* x = S_init();*/
	/*X* x = S_init();
	S_env_init(x);
	*/
	/*S_forth_init(x);*/
	X* x = S_sloth();
	while (1) {
	  fgets(buf, 255, stdin);
		S_evaluate(x, buf);
		/*S_eval(x, buf);*/
	}
}
