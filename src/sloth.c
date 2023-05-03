#include<stdio.h>
#include "sloth.h"

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

	(*(x->registers))->data['H' - 'A'] = (CELL)&hello;

	do {
		for (i = 0; i < 255; i++) { buf[i] = ';'; }
		printf("IN: ");
		str = fgets(buf, 255, stdin);
		for (i = 0; i < 255; i++) { if (buf[i] < 32) { buf[i] = ';'; } }
		x->ip = (BYTE*)buf;
		inner(x);
	} while(1);
}
