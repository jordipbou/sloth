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

void key(X* x) { S_lit(x, (I)_getch()); }
void emit(X* x) { printf("%c", (C)S_drop(x)); }

void hello(X* x) {
	printf("Hello world!\n");
}

/* I do need a quotation to define a word and a quotation to find a word. */
C* ROM =
"[SLOTH]5[dc@e1+]t_10e"
"[Reserve first 1024 bytes for TIB and initialize to 0]_"
"1024[0c,]t"
"[Reserve address 1024 for latest word variable]_"
"0,"
"[Define quotation for defining word header]_"
"[[Set link]_hb1024+@,b1024+!"
" [Set flags]_0"
" [Compile name]_dc,[dc,1+]t_]"
"d[find]4ri";

int main(int argc, char** argv) {
	FILE* fptr;
	C buf[255];
	I i;
	C* j;
	X* x = S_init();
	
	x->key = &key;
	x->emit = &emit;

  EXT(x, 'H') = &hello;

	/*
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
	*/
	S_eval(x, ROM);

	if (argc == 1 || argc == 3) {
		do {
			fgets(buf, 255, stdin);
			x->ip = buf;	
			S_inner(x);
			/*
      memset(buf, 0, 255);
      S_dump_X(buf, x);
      printf("%s\n", buf);
			*/
			if (x->err != 0) { printf("ERROR: %ld", x->err); return; }
			printf("Ok "); for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } printf("\n");
		} while(1);
	}
}
