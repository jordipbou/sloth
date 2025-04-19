#ifndef SLOTH_FLOATING_POINT_WORD_SET_HEADER
#define SLOTH_FLOATING_POINT_WORD_SET_HEADER

/* #include"sloth.h" */

/* -- Floating point word set -------------------------- */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x);
void sloth_f_aligned_(X* x);
void sloth_f_literal_(X* x);
void sloth_floats_(X* x);
void sloth_float_plus_(X* x);

/* Manipulating stack items */

void sloth_f_depth_(X* x);
void sloth_f_drop_(X* x);
void sloth_f_dup_(X* x);
void sloth_f_over_(X* x);
void sloth_f_rot_(X* x);
void sloth_f_swap_(X* x);

/* Comparison operations */

void sloth_f_less_than_(X* x);
void sloth_f_zero_less_than_(X* x);
void sloth_f_zero_equals_(X* x);

/* Memory-stack transfer operations */

void sloth_f_fetch_(X* x);
void sloth_f_store_(X* x);

/* Commands to define data structures */

void sloth_f_constant_(X* x);
void sloth_f_variable_(X* x);

/* Number-type conversion operators */

void sloth_d_to_f_(X* x);
void sloth_f_to_d_(X* x);
void sloth_s_to_f_(X* x);

/* Arithmetic and logical operations */

void sloth_f_plus_(X* x);
void sloth_f_minus_(X* x);
void sloth_f_star_(X* x);
void sloth_f_slash_(X* x);
void sloth_floor_(X* x);
void sloth_f_max_(X* x);
void sloth_f_min_(X* x);
void sloth_f_negate_(X* x);
void sloth_f_round_(X* x);

/* String/numeric conversion */

void sloth_to_float_(X* x);
void sloth_represent_(X* x);

/* Bootstraping the word set */

void bootstrap_floating_wordset(X* x);

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x) {
	sloth_set(
		x,
		SLOTH_HERE,
		(sloth_get(x, SLOTH_HERE) + (sFLOAT - 1)) & ~(sFLOAT - 1));
}

void sloth_f_aligned_(X* x) {
	sloth_push(x, (sloth_pop(x) + (sFLOAT - 1)) & ~(sFLOAT - 1));
}

void sloth_f_literal_(X* x) { /* TODO */ }
void sloth_floats_(X* x) { /* TODO */ }
void sloth_float_plus_(X* x) { /* TODO */ }

/* String/numeric conversion */

void sloth_to_float_(X* x) { /* TODO */ }
void sloth_represent_(X* x) { /* TODO */ }

/* Non ANS */

void sloth_dot_f_(X* x) {
	int i;
	printf("F:<%ld> ", x->fp);
	for (i = 0; i < x->fp; i++) printf("%f ", x->f[i]);
}

/* Bootstrapping */

void bootstrap_floating_wordset(X* x) {

	/* Constructing compiler and interpreter system extensions */

	sloth_code(x, "FALIGN", sloth_primitive(x, &sloth_f_align_));
	sloth_code(x, "FALIGNED", sloth_primitive(x, &sloth_f_aligned_));
	sloth_code(x, "FLITERAL", sloth_primitive(x, &sloth_f_literal_));
	sloth_code(x, "FLOATS", sloth_primitive(x, &sloth_floats_));
	sloth_code(x, "FLOAT+", sloth_primitive(x, &sloth_float_plus_));

	/*
	code(x, "F.", primitive(x, &_f_dot));
	code(x, "FE.", primitive(x, &_f_e_dot));
	code(x, "FS.", primitive(x, &_f_s_dot));
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
	*/

	/* String/numeric conversion */

	sloth_code(x, ">FLOAT", sloth_primitive(x, &sloth_to_float_));
	sloth_code(x, "REPRESENT", sloth_primitive(x, &sloth_represent_));

	/* Non ANS */
	sloth_code(x, ".F", sloth_primitive(x, &sloth_dot_f_));
}

#endif
