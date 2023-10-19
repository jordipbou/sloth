#include<stdio.h>
#include<stdlib.h>
#include"vm.h"

int main(int argc, char** argv) {
	char* r;
	char buf[255];
	C i;

	X* x = init();
	if (!x) exit(EXIT_FAILURE);

	if (argc == 2) {
		FILE *f = fopen(argv[1], "r");
		if (!f) {
			exit(EXIT_FAILURE);
		} else {
			char* line = 0;
			size_t len = 0;
			size_t read;

			while ((read = getline(&line, &len, f)) != -1) {
				printf("--> %s\n", line);
				evaluate(x, line);
				if (x->err) {
					printf("ERROR: %ld\n", x->err);
					if (x->err == -13) {
						printf("UNDEFINED WORD::IBUF: %s\n", &x->m->ibuf[x->m->ipos]);
					}
					if (x->err == -16) {
						printf("ZERO LENGTH NAME::IBUF: %s\n", &x->m->ibuf[x->m->ipos]);
					}
					exit(EXIT_FAILURE);
				}
			}

			fclose(f);
			if (line) free(line);
		}
	}

	printf("SLOTH v0.1\n");
	while (1) {
	  r = fgets(buf, 255, stdin);
    evaluate(x, (B*)buf);
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

/*
int main(int argc, char** argv) {
	char* r;
  int i;
  B buf[255];

	X* x = init_VM(init_MEM());

	if (argc == 2) {
		FILE *f = fopen(argv[1], "r");
		if (!f) {
			exit(EXIT_FAILURE);
		} else {
			char* line = 0;
			size_t len = 0;
			size_t read;

			while ((read = getline(&line, &len, f)) != -1) {
				printf("--> %s\n", line);
				evaluate(x, line);
				if (x->err) {
					printf("ERROR: %ld\n", x->err);
					if (x->err == -13) {
						printf("UNDEFINED WORD::IBUF: %s\n", x->ibuf);
					}
					if (x->err == -16) {
						printf("ZERO LENGTH NAME::IBUF: %s\n", x->ibuf);
					}
					exit(EXIT_FAILURE);
				}
			}

			fclose(f);
			if (line) free(line);
		}
	}

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
*/
