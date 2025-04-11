#include"sloth.h"

void _to_float(X* x) {
	/* TODO */
}

void _represent(X* x) {
	/* TODO */
}

void _f_dot(X* x) {
	/* TODO */
}

void _f_e_dot(X* x) {
	/* TODO */
}

void _f_s_dot(X* x) {
	/* TODO */
}

void bootstrap_floating_wordset(X* x) {
	code(x, ">FLOAT", primitive(x, &_to_float));
	code(x, "REPRESENT", primitive(x, &_represent));
	code(x, "F.", primitive(x, &_f_dot));
	code(x, "FE.", primitive(x, &_f_e_dot));
	code(x, "FS.", primitive(x, &_f_s_dot));
}
