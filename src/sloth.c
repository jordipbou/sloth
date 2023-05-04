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

/* TODO: This should use an external allocator. This would allow not accessing out of bounds
memory if using a memory block, for example */
ARRAY* newV(CELL size) {
	ARRAY* vector;

	vector = calloc(size + 3, sizeof(CELL));

	vector->type = 0;
	vector->size = size;
	vector->length = 0;

	return vector;
}

CONTEXT* init() {
	CONTEXT* x = malloc(sizeof(CONTEXT));

	x->data_stack = newV(64);
	x->call_stack = newV(64);

	x->extensions = newV(26);

	x->code = 0;
	x->data = 0;

	x->ip = 0;

	return x;
}

void hello(CONTEXT* x) {
	printf("Hello world!\n");
}

int main() {
	CONTEXT* x = init();
	char buf[255];
	char* str;
	CELL i;

	/*
	BYTE* a = (BYTE*)":d1>?1-d1-`s`+();";

	x->ip = a;
	PUSH(x, 15);
	inner(x);
	printf("%ld\n", S(x)->data[0]);
	*/

	E(x)->data['H' - 'A'] = (CELL)&hello;

	do {
		for (i = 0; i < 255; i++) { buf[i] = 0; }
		printf("IN: ");
		str = fgets(buf, 255, stdin);
		for (i = 0; i < 255; i++) { if (buf[i] < 32) { buf[i] = 0; } }
		x->ip = (BYTE*)buf;
		/*inner(x);*/
		trace(x);
	} while(1);
}
