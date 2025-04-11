#include"sloth.h"

/* Arithmetic and logical operations */

void _d_abs(X* x) { 
	/* TODO */ 
}

void _d_max(X* x) {
	/* TODO */
}

void _d_min(X* x) {
	/* TODO */
}

void _d_minus(X* x) {
	/* TODO */
}

void _m_plus(X* x) {
	/* TODO */
}

void _d_plus_store(X* x) {
	/* TODO */
}

void _d_slash(X* x) {
	/* TODO */
}

void _m_star_slash(X* x) {
	/* TODO */
}

void _d_two_star(X* x) {
	/* TODO */
}

void _d_two_slash(X* x) {
	/* TODO */
}

/* Manipulating stack items */

void _two_rot(X* x) { /* TODO */ }

void bootstrap_double_wordset(X* x) {
	code(x, "DABS", primitive(x, &_d_abs));
	code(x, "DMAX", primitive(x, &_d_max));
	code(x, "DMIN", primitive(x, &_d_min));
	code(x, "D-", primitive(x, &_d_minus));
	code(x, "M+", primitive(x, &_m_plus));
	code(x, "D+!", primitive(x, &_d_plus_store));
	code(x, "D/", primitive(x, &_d_slash));
	code(x, "M*/", primitive(x, &_m_star_slash));
	code(x, "D2*", primitive(x, &_d_two_star));
	code(x, "D2/", primitive(x, &_d_two_slash));
	code(x, "2ROT", primitive(x, &_two_rot));

}
