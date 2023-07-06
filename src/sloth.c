#include<stdio.h>
#include "sloth.h"
/* #include "forth.h" */

#ifdef _WIN32
  #include <conio.h>
#else
	#include <unistd.h>
	#include <termios.h>
#endif

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
	cur_term.c_lflag &= ~(ICANON | ECHO | ISIG); /* echoing off, canonical off, no signal chars */
	cur_term.c_cc[VMIN] = 1;
	cur_term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &cur_term) < 0)	{ perror("tcsetattr"); }
	if (read(STDIN_FILENO, &ch, 1) < 0)	{ /* perror("read()"); */ } /* signal will interrupt */
	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old_term) < 0)	{ perror("tcsetattr"); }
	return ch;
}
#endif

void key(X* x) { S_lit(x, (C)_getch()); }
void emit(X* x) { printf("%c", (B)S_drop(x)); }

B* bootForth =
"4096mb,"
"cc+b.," /* 0: HERE, 1: LATEST */
/* Find WORD should be compiled as code on cc+b.+ to not need any
  variable later on */
"[b.b..+;1b..+b.,]w(" /* C, : w0 */
"[b.b..+,cb..+b.,]w(" /* ,  : w1 */
"[b.b..+ cb.+. w1$ cb.+, 0w0$ dw0$ [d:w0$1+]n_ [d:w0$1+]n_]w(" /* HEADER : w2 */
"[c+1+1+]w(" /* NFA : w3 */
"[dc+1+:c+1+1++]w(" /* CFA : w4 */
"[_]2\"DROP\"w2$"
"[s]2\"SWAP\"w2$"
"[o]2\"OVER\"w2$"
"[@]2\"ROT\"w2$";

int main(int argc, char** argv) {
	FILE* fptr;
	B buf[255];
	C i;
	B* j;
	X* x = S_init();
	
  x->key = &key;
  x->emit = &emit;
  /*EXT(x, 'F') = &F_bytecodes;*/

	if (argc == 2 || argc == 3) {
		fptr = fopen(argv[1], "r");
		while (fgets(buf, 255, fptr)) {
			S_eval(x, buf);
			if (x->err != 0) {
					printf("ERROR: %ld\n", x->err);
					return;
			}
			x->err = 0;
		}
		printf("Ok "); for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } printf("\n");
	}

  S_eval(x, bootForth);
  
	if (argc == 1 || argc == 3) {
		do {
			fgets(buf, 255, stdin);
			x->ip = buf;	
			S_inner(x);
			/*
      memset(buf, 0, 255);
      S_dump_X(buf, x, 50);
      printf("%s\n", buf);
			*/
			if (x->err != 0) { printf("ERROR: %ld", x->err); return; }
			printf("Ok "); for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } printf("\n");
		} while(1);
	}
}
