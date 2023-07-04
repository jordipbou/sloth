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

void key(X* x) { S_lit(x, (C)_getch()); }
void emit(X* x) { printf("%c", (B)S_drop(x)); }

void hello(X* x) {
	printf("Hello world!\n");
}

void memory(X* x) {
	C n;
	B* s;
	switch (S_token(x)) {
	case 'a': n = S_drop(x); S_lit(x, (C)malloc(n)); break;
	case 'f': free((void*)S_drop(x)); break;
	case 'd': s = (B*)S_drop(x); for (n = 0; n < 16; n++) { printf("%c", s[n]); }; break;
	}
}

/* I do need a quotation to define a word and a quotation to find a word. */

/*
Base address of dicionary: b.
Pointer to free space on dictionary: fetch: b.. store: b., (Address 0)
Compile char: b..'<char>;1b..+b.,
Compile cell: b..#<cell>,cb..+b.,
Pointer to latest defined word: fetch: cb.+. store: cb.+, (Address cellsize)
*/

B* ROM =
"\"SLOTH\"t10e"
/* Allocate a dictionary of 4096 bytes, and save as b variable */
"4096mb,";

/* How I imagine this should work:

[1][[[accept input][word][find][compile][interpret]"interpret]"quit"]w

*/

int main(int argc, char** argv) {
	FILE* fptr;
	B buf[255];
	C i;
	B* j;
	X* x = S_init();
	
  x->key = &key;
  x->emit = &emit;
  EXT(x, 'H') = &hello;
	EXT(x, 'M') = &memory;

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
	
	/* S_eval(x, ROM); */

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
