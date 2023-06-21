#include<stdio.h>
#include "sloth.h"

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

void key(X* x) { S_push(x, (I)_getch()); }
void emit(X* x) { printf("%c", (B)S_pop(x)); }

void hello(X* x) {
	printf("Hello world!\n");
}

B* ROM =
	"[Hello world!]lp10e";

int main(int argc, char** argv) {
	FILE* fptr;
	B buf[255];
	I i;
	B* j;
	X* x = S_init();
	
	KEY(x) = &key;
	EMIT(x) = &emit;

  EXT(x, 'H') = &hello;

	if (argc == 2 || argc == 3) {
		fptr = fopen(argv[1], "r");
		while (fgets(buf, 255, fptr)) {
			S_eval(x, buf);
			if (ERROR(x) != 0) {
					printf("ERROR: %ld\n", x->err);
					return;
			}
			x->err = 0;
		}
	}

	S_eval(x, ROM);

	if (argc == 1 || argc == 3) {
		do {
			fgets(buf, 255, stdin);
			S_push_R(x, buf);
			S_inner(x);
			/* Tracing */
			printf("OUT: <%ld> ", x->sp);
			for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } 
			for (i = x->rp - 1; i >= 0; i--) { printf(" : "); for (j = x->r[i]; *j != 0 && *j != 10&& *j != ']'; j++) { printf("%c", *j); } }
			printf(" <%ld>\n", x->rp);
		} while(1);
	}
}
