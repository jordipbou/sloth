#include<stdio.h>
#include<stdlib.h>
#include"vm.h"
/*
#include"combinators.h"
*/

/*#include"sloth.h"*/

#ifdef _WIN32
  #include <conio.h>
#else
  #include <unistd.h>
  #include <termios.h>
#endif

void do_error(X* x) {
	/*
	if (x->err == -13) {
		printf("UNDEFINED WORD [%.*s]\n", (int)T(x), (char *)N(x));
	} else if (x->err == -16) {
		printf("ZERO LENGTH NAME::IBUF: %s\n", &x->m->ibuf[x->m->ipos]);
	} else {
	*/
	if (x->err == -256) { exit(0); }
	else { 
		printf("ERROR: %ld\n", x->err); 
		x->sp = 0;
		x->rp = 0;
		x->err = 0;
	}
	/*
	}
	*/
}
/*
 Source code for getch is taken from:
 Crossline readline (https://github.com/jcwangxp/Crossline).
 It's a fantastic readline cross-platform replacement, but only getch was
 needed and there's no need to include everything else.
*/
#ifdef _WIN32
int _getch (void) {	fflush (stdout); return _getch(); }
#else
int _getch ()
{
  char ch = 0;
  struct termios old_term, cur_term;
  fflush (stdout);
  if (tcgetattr(STDIN_FILENO, &old_term) < 0)	{ perror("tcsetattr"); }
  cur_term = old_term;
  cur_term.c_lflag &= ~(ICANON | ECHO | ISIG); 
  cur_term.c_cc[VMIN] = 1;
  cur_term.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &cur_term) < 0)	{ perror("tcsetattr"); }
  if (read(STDIN_FILENO, &ch, 1) < 0)	{ } 
  if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old_term) < 0)	{ perror("tcsetattr"); }
  return ch;
}
#endif

V key(X* x) { PUSH(x, _getch()); }
V emit(X* x) { L1(x, B, c); printf("%c", c); }

int main(int argc, char** argv) {
	char* r;
	char buf[255];

	/*X* x = init_context(init_dict(1024 * 1024));*/
	X* x = (X*)init_sloth(init_dict(1024 * 1024));
	EXT(x, 'E') = &emit;
	EXT(x, 'K') = &key;
	EXT(x, 'S') = &sloth_ext;

	x->tr = 1;

/*
 	if (argc == 2) {
		FILE *f = fopen(argv[1], "r");
		if (!f) {
			exit(EXIT_FAILURE);
		} else {
			char* line = 0;
			size_t len = 0;
			size_t read;

			while ((read = getline(&line, &len, f)) != -1) {
				printf("--> %s", line);
				evaluate(s, line);
				if (s->err) {
					do_error(s);
					exit(EXIT_FAILURE);
				}
			}

			fclose(f);
			if (line) free(line);
		}
	}
*/
 printf("SLOTH v0.1\n");
  
	while (1) {
	  r = fgets(buf, 255, stdin);
		/*assembler(x, buf);*/
		evaluate((S*)x, buf);
		if (!x->err) {
			trace(x);
			printf("Ok\n");
		} else {
			do_error(x);
			/* reset(x); */
		}
	}

/*
	char* r;
	char buf[255];
	C i;

	M* m = init_VM(init_DICT(65536));
	EXT(m, 'E') = &emit;
  EXT(m, 'K') = &key;
	EXT(m, 'C') = &combinators_ext;
	if (!m) {
    printf("Init error\n");
    exit(EXIT_FAILURE);
  }

	if (argc == 2) {
		FILE *f = fopen(argv[1], "r");
		if (!f) {
			exit(EXIT_FAILURE);
		} else {
			char* line = 0;
			size_t len = 0;
			size_t read;

			while ((read = getline(&line, &len, f)) != -1) {
				printf("--> %s", line);
				evaluate(m, line);
				if (m->err) {
					do_error(m);
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
		evaluate(m, buf);
		if (!m->err) {
			trace(m);
			printf(" Ok\n");
		} else {
			do_error(m);
			reset(m);
		}
	}
*/
}
