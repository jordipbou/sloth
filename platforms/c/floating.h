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

/* Constructing compiler and interpreter system extensions */

void _f_align(X* x) { /* TODO */ }
void _f_aligned(X* x) { /* TODO */ }
void _f_literal(X* x) { /* TODO */ }
void _floats(X* x) { /* TODO */ }
void _float_plus(X* x) { /* TODO */ }

/* Manipulating stack items */

void _f_depth(X* x) { /* TODO */ }
void _f_drop(X* x) { /* TODO */ }
void _f_dup(X* x) { /* TODO */ }
void _f_over(X* x) { /* TODO */ }
void _f_rot(X* x) { /* TODO */ }
void _f_swap(X* x) { /* TODO */ }

/* Comparison operations */

void _f_less_than(X* x) { /* TODO */ }
void _f_zero_equals(X* x) { /* TODO */ }
void _f_zero_less_than(X* x) { /* TODO */ }

/* Memory-stack transfer operations */

void _f_fetch(X* x) { /* TODO */ }
void _f_store(X* x) { /* TODO */ }

/* Commands to define data structures */

void _f_constant(X* x) { /* TODO */ }
void _f_variable(X* x) { /* TODO */ }

/* Number-type conversion operators */

void _d_to_f(X* x) { /* TODO */ }
void _f_to_d(X* x) { /* TODO */ }

/* Arithmetic and logical operations */

void _f_star(X* x) { /* TODO */ }
void _f_slash(X* x) { /* TODO */ }
void _f_plus(X* x) { /* TODO */ }
void _f_plus_store(X* x) { /* TODO */ }
void _floor(X* x) { /* TODO */ }
void _f_max(X* x) { /* TODO */ }
void _f_min(X* x) { /* TODO */ }
void _f_negate(X* x) { /* TODO */ }
void _f_round(X* x) { /* TODO */ }

void bootstrap_floating_wordset(X* x) {
	code(x, ">FLOAT", primitive(x, &_to_float));
	code(x, "REPRESENT", primitive(x, &_represent));
	code(x, "F.", primitive(x, &_f_dot));
	code(x, "FE.", primitive(x, &_f_e_dot));
	code(x, "FS.", primitive(x, &_f_s_dot));
	code(x, "FALIGN", primitive(x, &_f_align));
	code(x, "FALIGNED", primitive(x, &_f_aligned));
	code(x, "FLITERAL", primitive(x, &_f_literal));
	code(x, "FLOATS", primitive(x, &_floats));
	code(x, "FLOAT+", primitive(x, &_float_plus));
	code(x, "FDEPTH", primitive(x, &_f_depth));
	code(x, "FDROP", primitive(x, &_f_drop));
	code(x, "FDUP", primitive(x, &_f_dup));
	code(x, "FOVER", primitive(x, &_f_over));
	code(x, "FROT", primitive(x, &_f_rot));
	code(x, "FSWAP", primitive(x, &_f_swap));
	code(x, "F<", primitive(x, &_f_less_than));
	code(x, "F0=", primitive(x, &_f_zero_equals));
	code(x, "F0<", primitive(x, &_f_zero_less_than));
	code(x, "F@", primitive(x, &_f_fetch));
	code(x, "F!", primitive(x, &_f_store));
	code(x, "FCONSTANT", primitive(x, &_f_constant));
	code(x, "FVARIABLE", primitive(x, &_f_variable));
	code(x, "D>F", primitive(x, &_d_to_f));
	code(x, "F>D", primitive(x, &_f_to_d));
	code(x, "F*", primitive(x, &_f_star));
	code(x, "F/", primitive(x, &_f_slash));
	code(x, "F+", primitive(x, &_f_plus));
	code(x, "F+!", primitive(x, &_f_plus_store));
	code(x, "FLOOR", primitive(x, &_floor));
	code(x, "FMAX", primitive(x, &_f_max));
	code(x, "FMIN", primitive(x, &_f_min));
	code(x, "FNEGATE", primitive(x, &_f_negate));
	code(x, "FROUND", primitive(x, &_f_round));
}
