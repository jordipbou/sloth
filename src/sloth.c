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
ARRAY newV(CELL size) {
	ARRAY array;

	array = calloc(size + 3, sizeof(CELL));

	array->tp = 0;
	array->sz = size;
	array->len = 0;

	return array;
}

CONTEXT* init() {
	CONTEXT* ctx = malloc(sizeof(CONTEXT));

	ctx->s = newV(64);
	ctx->r = newV(64);

	ctx->x = newV(26);

	ctx->c = 0;
	ctx->d = 0;

	IP = 0;

	return ctx;
}

void hello(CONTEXT* x) {
	printf("Hello world!\n");
}

int main() {
	char* str;
	CELL i;

	ARRAY code = newV(32);
	CONTEXT* ctx = init();

	code->len = 32*sizeof(CELL);
	ctx->c = code;

	ctx->x->dt['H' - 'A'] = (CELL)&hello;

	do {
		for (i = 0; i < 255; i++) { C[i] = 0; }
		printf("IN: ");
		str = fgets((char*)C, 255, stdin);
		IP = 0;
		/*inner(x);*/
		i = trace(ctx);
		if (i != ERR_OK) { printf("ERROR: %ld\n", i); }
	} while(1);
}
