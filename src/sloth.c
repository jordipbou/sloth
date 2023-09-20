#include<stdio.h>
#include"sloth.h"

int main(int argc, char** argv) {
  B buf[255];
  X* x = S_forth();
	while (1) {
	  fgets(buf, 255, stdin);
		S_evaluate(x, buf);
	}
}
