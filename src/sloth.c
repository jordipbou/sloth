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

void hello(X* x) {
	printf("Hello world!\n");
}

int main() {
	char* str;
	char buf[255];
	C i;

	X* x = init();

	x->e->d['H' - 'A'] = (C)&hello;

	do {
		for (i = 0; i < 255; i++) { OP(x, i) = 0; }
		for (i = 0; i < HERE(x); i++) { printf("|%x", AT(x, i)); }
		printf("\nIN: ");
		str = fgets((char*)x->c->d, 255, stdin);
		IP(x) = 0;
		/*inner(x);*/
		i = trace(x);
		if (i != ERR_OK) { printf("ERROR: %ld\n", i); }
	} while(1);
}
