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

void key(X* x) { PUSH(x, _getch()); }
void emit(X* x) { printf("%c", (char)POP(x)); }

void hello(X* x) {
	printf("Hello world!\n");
}

int main() {
	X* x = init();
	EX(x, 'K') = (C)&key;
	EX(x, 'E') = (C)&emit;
	repl(x);
}
