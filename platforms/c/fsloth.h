#ifndef SLOTH_FLOATING_POINT_WORD_SET_HEADER
#define SLOTH_FLOATING_POINT_WORD_SET_HEADER

#include<math.h>

typedef double FLOAT;
typedef float SFLOAT;
typedef double DFLOAT;
	
#define sFLOAT sizeof(FLOAT)
#define sSFLOAT sizeof(SFLOAT)
#define sDFLOAT sizeof(DFLOAT)

#define sFLOAT_BITS sFLOAT*8

struct sloth_VM;

/* Pre-definitions */

void sloth_fpush(struct sloth_VM* x, FLOAT v);
void sloth_fliteral(struct sloth_VM* x, FLOAT n);

#include"sloth.h"

/* ----------------------------------------------------- */
/* ---------------- Virtual machine -------------------- */
/* ----------------------------------------------------- */

/* -- Floating point stack ----------------------------- */
	
void sloth_fpush(X* x, FLOAT v);
FLOAT sloth_fpop(X* x);
FLOAT sloth_fpick(X* x, CELL a);

/* -- Memory ------------------------------------------- */

void sloth_fstore(X* x, CELL a, FLOAT v);
FLOAT sloth_ffetch(X* x, CELL a);
void sloth_sfstore(X* x, CELL a, SFLOAT v);
SFLOAT sloth_sffetch(X* x, CELL a);
void sloth_dfstore(X* x, CELL a, DFLOAT v);
DFLOAT sloth_dffetch(X* x, CELL a);

/* -- Inner interpreter -------------------------------- */

FLOAT sloth_fop(X* x);

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* Setting and getting variables (cell and char sized) */

/* ABS CHANGES
void sloth_fset(X* x, CELL a, FLOAT v);
FLOAT sloth_fget(X* x, CELL a);
*/

/* Compilation */

void sloth_fcomma(X* x, FLOAT v);

void sloth_fliteral(X* x, FLOAT n);

/* -- Primitives -------------------------------------- */

void sloth_flit_(X* x);

/* -- Floating point word set ------------------------- */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x);
void sloth_f_aligned_(X* x);
void sloth_f_literal_(X* x);
void sloth_floats_(X* x);
void sloth_float_plus_(X* x);

void sloth_s_f_aligned_(X* x);
void sloth_d_f_aligned_(X* x);

void sloth_s_floats_(X* x);
void sloth_d_floats_(X* x);

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
void sloth_s_f_fetch_(X* x);
void sloth_s_f_store_(X* x);
void sloth_d_f_fetch_(X* x);
void sloth_d_f_store_(X* x);

/* Commands to define data structures */

void sloth_f_constant_(X* x);
void sloth_f_variable_(X* x);

/* Number-type conversion operators */

void sloth_d_to_f_(X* x);
void sloth_f_to_d_(X* x);

/* Arithmetic and logical operations */

void sloth_f_abs_(X* x);
void sloth_f_plus_(X* x);
void sloth_f_minus_(X* x);
void sloth_f_star_(X* x);
void sloth_f_star_star_(X* x);
void sloth_f_slash_(X* x);
void sloth_floor_(X* x);
void sloth_f_max_(X* x);
void sloth_f_min_(X* x);
void sloth_f_negate_(X* x);
void sloth_f_round_(X* x);
void sloth_f_proximate_(X* x);
void sloth_f_atan2_(X* x);
void sloth_f_sqrt_(X* x);
void sloth_f_l_n_(X* x);
void sloth_f_sine_(X* x);
void sloth_f_cos_(X* x);
void sloth_f_sine_cos_(X* x);
void sloth_f_tan_(X* x);
void sloth_f_a_sine_(X* x);
void sloth_f_a_cos_(X* x);
void sloth_f_a_tan_(X* x);
void sloth_f_exp_(X* x);
void sloth_f_exp_m_one_(X* x);
void sloth_f_log_ten_(X* x);
void sloth_f_l_n_p_one_(X* x);
void sloth_f_a_log_(X* x);
void sloth_f_sin_h_(X* x);
void sloth_f_cos_h_(X* x);
void sloth_f_tan_h_(X* x);
void sloth_f_a_sine_h_(X* x);
void sloth_f_a_cos_h_(X* x);

/* String/numeric conversion */

void sloth_to_float_(X* x);
void sloth_represent_(X* x);

/* Output operations */

void sloth_f_dot_(X* x);
void sloth_f_s_dot_(X* x);
void sloth_f_e_dot_(X* x);

/* Non ANS floating point helpers */

void sloth_f_dot_s_(X* x);

#ifdef SLOTH_IMPLEMENTATION

/* ----------------------------------------------------- */
/* ---------------- Virtual machine -------------------- */
/* ----------------------------------------------------- */

/* -- Floating point stack ----------------------------- */

void sloth_fpush(X* x, FLOAT v) { x->f[x->fp] = v; x->fp++; }
FLOAT sloth_fpop(X* x) { x->fp--; return x->f[x->fp]; }
FLOAT sloth_fpick(X* x, CELL a) { return x->f[x->fp - a - 1]; }

/* -- Memory ------------------------------------------- */

void sloth_fstore(X* x, CELL a, FLOAT v) { *((FLOAT*)a) = v; }
FLOAT sloth_ffetch(X* x, CELL a) { return *((FLOAT*)a); }
void sloth_sfstore(X* x, CELL a, SFLOAT v) { *((SFLOAT*)a) = v; }
SFLOAT sloth_sffetch(X* x, CELL a) { return *((SFLOAT*)a); }
void sloth_dfstore(X* x, CELL a, DFLOAT v) { *((DFLOAT*)a) = v; }
DFLOAT sloth_dffetch(X* x, CELL a) { return *((DFLOAT*)a); }

/* -- Inner interpreter -------------------------------- */

FLOAT sloth_fop(X* x) {
	/* ABS CHANGES
	FLOAT n = sloth_ffetch(x, sloth_to_abs(x, x->ip)); */
	FLOAT n = sloth_ffetch(x, x->ip);
	x->ip += sFLOAT;
	return n;
}

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

/* ABS CHANGES
void sloth_fset(X* x, CELL a, FLOAT v) { 
	sloth_fstore(x, sloth_to_abs(x, a), v); 
}
FLOAT sloth_fget(X* x, CELL a) { 
	return sloth_ffetch(x, sloth_to_abs(x, a)); 
}
*/

/* Compilation */

void sloth_fcomma(X* x, FLOAT v) { 
	/* ABS CHANGES
	sloth_fset(x, sloth_here(x), v); */
	sloth_fstore(x, sloth_here(x), v);
	sloth_allot(x, sFLOAT); 
}

void sloth_fliteral(X* x, FLOAT n) {
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(FLIT)")));
	sloth_fcomma(x, n);
}

/* -- Primitives -------------------------------------- */

void sloth_flit_(X* x) { sloth_fpush(x, sloth_fop(x)); }

/* == Floating point word set ========================== */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x) { 
	sloth_set(
		x, 
		SLOTH_HERE, 
		ALIGNED(sloth_get(x, SLOTH_HERE), sFLOAT));
}
void sloth_f_aligned_(X* x) { 
	sloth_push(x, ALIGNED(sloth_pop(x), sFLOAT)); 
}
void sloth_f_literal_(X* x) { /* TODO */ }
void sloth_floats_(X* x) { sloth_push(x, sloth_pop(x) * sFLOAT); }
void sloth_float_plus_(X* x) { /* TODO */ }

void sloth_s_f_aligned_(X* x) { 
	sloth_push(x, ALIGNED(sloth_pop(x), sSFLOAT)); 
}
void sloth_d_f_aligned_(X* x) {
	sloth_push(x, ALIGNED(sloth_pop(x), sDFLOAT)); 
}

void sloth_s_floats_(X* x) { sloth_push(x, sloth_pop(x) * sSFLOAT); }
void sloth_d_floats_(X* x) { sloth_push(x, sloth_pop(x) * sDFLOAT); }

/* Manipulating stack items */

void sloth_f_depth_(X* x) { sloth_push(x, x->fp); }
void sloth_f_drop_(X* x) { sloth_fpop(x); }
void sloth_f_dup_(X* x) { sloth_fpush(x, sloth_fpick(x, 0)); }
void sloth_f_over_(X* x) { sloth_fpush(x, sloth_fpick(x, 1)); }
void sloth_f_rot_(X* x) { 
	FLOAT c = sloth_fpop(x);
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_fpush(x, b);
	sloth_fpush(x, c);
	sloth_fpush(x, a);
}
void sloth_f_swap_(X* x) { 
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_fpush(x, b);
	sloth_fpush(x, a);
}

/* Comparison operations */

void sloth_f_less_than_(X* x) { 
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_push(x, a < b ? -1 : 0);
}
void sloth_f_zero_less_than_(X* x) { 
	sloth_push(x, sloth_fpop(x) < 0.0 ? -1 : 0); 
}
void sloth_f_zero_equals_(X* x) {
	sloth_push(x, sloth_fpop(x) == 0.0 ? -1 : 0);
}

/* Memory-stack transfer operations */

void sloth_f_fetch_(X* x) { 
	sloth_fpush(x, sloth_ffetch(x, sloth_pop(x))); 
}
void sloth_f_store_(X* x) { 
	sloth_fstore(x, sloth_pop(x), sloth_fpop(x)); 
}

void sloth_s_f_fetch_(X* x) { 
	sloth_fpush(x, sloth_sffetch(x, sloth_pop(x))); 
}
void sloth_s_f_store_(X* x) { 
	sloth_sfstore(x, sloth_pop(x), (SFLOAT)sloth_fpop(x));
}

void sloth_d_f_fetch_(X* x) { 
	sloth_fpush(x, sloth_dffetch(x, sloth_pop(x))); 
}
void sloth_d_f_store_(X* x) { 
	sloth_dfstore(x, sloth_pop(x), sloth_fpop(x)); 
}

/* Commands to define data structures */

void sloth_f_constant_(X* x) { /* TODO */}
void sloth_f_variable_(X* x) { /* TODO */}

/* Number-type conversion operators */

void sloth_d_to_f_(X* x) {
	CELL hi = sloth_pop(x);
	CELL lo = sloth_pop(x);
	double r;
	if (hi >= 0) {
		r = ldexp((double)hi, sFLOAT_BITS) + (double)lo;
	} else {
		/* negative number: -(2^128 - unsigned_value) */
		/* compute unsigned_value = ( (uint64_t)hi << 64 ) | lo */
		/* but better to subtract from 0.0 */
		r = - ( ldexp((double)(~(uCELL)hi), sFLOAT_BITS) + (double)(~lo) + 1.0 );
	}
	sloth_fpush(x, r);
}
void sloth_f_to_d_(X* x) {
	FLOAT i;
	modf(sloth_fpop(x), &i);
	sloth_push(x, (CELL)i);
	sloth_push(x, i < 0 ? -1 : 0);
}

/* Arithmetic and logical operations */

void sloth_f_abs_(X* x) {
	sloth_fpush(x, fabs(sloth_fpop(x)));
}
void sloth_f_plus_(X* x) { 
	FLOAT b = sloth_fpop(x);
	sloth_fpush(x, sloth_fpop(x) + b);
}
void sloth_f_minus_(X* x) { 
	FLOAT b = sloth_fpop(x);
	sloth_fpush(x, sloth_fpop(x) - b);
}
void sloth_f_star_(X* x) { 
	FLOAT b = sloth_fpop(x);
	sloth_fpush(x, sloth_fpop(x) * b);
}
void sloth_f_star_star_(X* x) {
	FLOAT b = sloth_fpop(x);
	sloth_fpush(x, pow(sloth_fpop(x), b));
}
void sloth_f_slash_(X* x) { 
	FLOAT b = sloth_fpop(x);
	sloth_fpush(x, sloth_fpop(x) / b);
}
void sloth_floor_(X* x) { 
	sloth_fpush(x, floor(sloth_fpop(x)));
}
void sloth_f_max_(X* x) { 
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_fpush(x, a > b ? a : b);
}
void sloth_f_min_(X* x) { 
	FLOAT b = sloth_fpop(x);
	FLOAT a = sloth_fpop(x);
	sloth_fpush(x, a < b ? a : b);
}
void sloth_f_negate_(X* x) { 
	sloth_fpush(x, -sloth_fpop(x));
}
void sloth_f_round_(X* x) {
	FLOAT r = sloth_fpop(x);
	if (r >= 0.0) {
		/* floor(x + 0.5) works for non-negative x */
		sloth_fpush(x, floor(r + 0.5));
	} else {
		/* for negative x, ties away from zero: ceil(x - 0.5)
		 * but many C89 libs lack ceil(), so use -floor(0.5 - x)
		 */
		sloth_fpush(x, -floor(0.5 - r));
	}
}
void sloth_f_proximate_(X* x) {
	FLOAT r3 = sloth_fpop(x);
	FLOAT r2 = sloth_fpop(x);
	FLOAT r1 = sloth_fpop(x);
	if (isnan(r3)) {
		sloth_push(x, 0);
	} else if (r3 > 0.0) {
		sloth_push(x, fabs(r1 - r2) < r3 ? -1 : 0);
	} else if (r3 < 0.0) {
		sloth_push(x, fabs(r1 - r2) < (fabs(r3)*(fabs(r1)+fabs(r2))) ? -1 : 0);
	} else {
		uint64_t a, b;
		memcpy(&a, &r1, sizeof(double));
		memcpy(&b, &r2, sizeof(double));
		sloth_push(x, a == b ? -1 : 0);
	}
}
void sloth_f_atan2_(X* x) {
	FLOAT b = sloth_fpop(x);
	sloth_fpush(x, atan2(sloth_fpop(x), b));
}
void sloth_f_sqrt_(X* x) {
	sloth_fpush(x, sqrt(sloth_fpop(x)));
}
void sloth_f_l_n_(X* x) {
	sloth_fpush(x, log(sloth_fpop(x)));
}
void sloth_f_sine_(X* x) {
	sloth_fpush(x, sin(sloth_fpop(x)));
}
void sloth_f_cos_(X* x) {
	sloth_fpush(x, cos(sloth_fpop(x)));
}
void sloth_f_sine_cos_(X* x) {
	FLOAT r = sloth_fpop(x);
	sloth_fpush(x, sin(r));
	sloth_fpush(x, cos(r));
}
void sloth_f_tan_(X* x) {
	sloth_fpush(x, tan(sloth_fpop(x)));
}
void sloth_f_a_sine_(X* x) {
	sloth_fpush(x, asin(sloth_fpop(x)));
}
void sloth_f_a_cos_(X* x) {
	sloth_fpush(x, acos(sloth_fpop(x)));
}
void sloth_f_a_tan_(X* x) {
	sloth_fpush(x, atan(sloth_fpop(x)));
}
void sloth_f_exp_(X* x) {
	sloth_fpush(x, exp(sloth_fpop(x)));
}
void sloth_f_exp_m_one_(X* x) {
	sloth_fpush(x, exp(sloth_fpop(x)) - 1.0);
}
void sloth_f_log_ten_(X* x) {
	sloth_fpush(x, log10(sloth_fpop(x)));
}
void sloth_f_l_n_p_one_(X* x) {
	sloth_fpush(x, log(sloth_fpop(x) + 1.0));
}
void sloth_f_a_log_(X* x) {
	sloth_fpush(x, pow(10.0, sloth_fpop(x)));
}
void sloth_f_sin_h_(X* x) {
	sloth_fpush(x, sinh(sloth_fpop(x)));
}
void sloth_f_cos_h_(X* x) {
	sloth_fpush(x, cosh(sloth_fpop(x)));
}
void sloth_f_tan_h_(X* x) {
	sloth_fpush(x, tanh(sloth_fpop(x)));
}
/* There's no asinh function in math.h in C89. It */
/* appeared on C99. */
void sloth_f_a_sine_h_(X* x) {
	FLOAT r = sloth_fpop(x);
	if (r == 0) {
		sloth_fpush(x, 0.0);
	} else if (r > 0) {
		sloth_fpush(x, log(r + sqrt(r * r + 1.0)));
	} else {
		sloth_fpush(x, -log(-r + sqrt(r * r + 1.0)));
	}
}
void sloth_f_a_cos_h_(X* x) {
	FLOAT r = sloth_fpop(x);
	if (r < 1.0) {
		/* undefined, push NaN */
		sloth_fpush(x, nan(""));
	} else {
		sloth_fpush(x, log(r + sqrt(r * r - 1.0)));
	}
}

/* String/numeric conversion */

/* This function gets complicated to correctly represent */
/* the ANS Forth standard. It would be easier if just was */
/* a mirror to the strtod C function. */
void sloth_to_float_(X* x) {
	char buf[64]; 
	char *endptr;
	int tlen = (int)sloth_pop(x);
	char* tok = (char*)sloth_pop(x);
	FLOAT n;
	int i, j, nlen, marker;;
	/* >FLOAT does not allow trailing spaces (although */
	/* strtod does). But, at the same time, a string of */
	/* blanks must be considered as a special case */
	/* representing zero. */
	if (*tok == ' ' || tlen == 0) {
		for (i = 0; i < tlen; i++) {
			/* If a non space character is found, we have a */
			/* trailing space string, and that means we */
			/* cannot convert it (by the standard). */
			if (*(tok + i) != ' ') {
				sloth_push(x, 0);
				return;
			}
		}
		/* If the string was made only of blanks, its a 0E */
		sloth_push(x, -1);
		sloth_fpush(x, 0.0);
		return;
	}
	if (*(tok + tlen - 1) == ' ') {
		sloth_push(x, 0);
		return;
	}
	/* Let's copy the string but taking into account the */
	/* possibility of not having the E in the string. */
	nlen = tlen;
	marker = 0;
	for (i = 0, j = 0; i < tlen; i++) {
		/* Forth's >FLOAT is a lot more restrictive than strtod */
		/* so we check if any non specified character appears in */
		/* the string to just do not allow the conversion. */
		if (*(tok + i) != '0' && *(tok + i) != '1' && *(tok + i) != '2'
		 && *(tok + i) != '3' && *(tok + i) != '4' && *(tok + i) != '5'
		 && *(tok + i) != '6' && *(tok + i) != '7' && *(tok + i) != '8'
		 && *(tok + i) != '9' && *(tok + i) != '+' && *(tok + i) != '-'
		 && *(tok + i) != 'D' && *(tok + i) != 'd' && *(tok + i) != 'E'
		 && *(tok + i) != 'e' && *(tok + i) != '.') {
			sloth_push(x, 0);
			return;
		}
		/* Being correct characters, D/d E/e must not appear */
		/* more than once. */
		if (*(tok + i) == 'D' || *(tok + i) == 'd' || *(tok + i) == 'E'
		 || *(tok + i) == 'e') {
			if (marker == 0) marker = 1;
			else {
				sloth_push(x, 0);
				return;
			}
		}
		if (i != 0
		 && (*(tok + i) == '+' || *(tok + i) == '-')
		 && (*(tok + i - 1) != 'E' && *(tok + i - 1) != 'e')) {
			buf[j++] = 'E';
			nlen++;
		}
		buf[j++] = *(tok + i);
	}
	buf[nlen] = 0;
	n = strtod(buf, &endptr);
	if (n == 0 && endptr == buf) {
		sloth_push(x, 0);
	} else {
		sloth_fpush(x, n);
		sloth_f_dot_s_(x); printf("\n");
		sloth_push(x, -1);
	}
}

void sloth_represent_(X* x) {
	FLOAT r = sloth_fpop(x);
	CELL u = sloth_pop(x);
	CELL addr = sloth_pop(x);
	/* This implementation uses the algorithm found in */
	/* represent_in_c.zip in Taygeta FTP. */
	/* I'm ignoring the REPRESENT-CHARS part */
	int i, j;
	char buf[64], *endptr;
	/* 1. Fill buffer at caddr with n blanks (space chars) */
	/* where n is the greater of n1 or REPRESENT-CHARS. */
	for (i = 0; i < u; i++) sloth_cstore(x, addr + i, ' ');
	/* 2. Apply sprintf to r using %#.*E where * is */
	/* MAX-FLOAT-DIGITS less 1. */
	sprintf(buf, "%#.*E", (int)(u - 1), r);
	/* 3. Check if its a non-number representation. */
	for (i = 0; i < strlen(buf); i++) {
		if (buf[i] == 'n' || buf[i] == 'N') {
			for (j = 0; j < strlen(buf); j++) {
				sloth_cstore(x, addr + j, buf[j]);
				sloth_push(x, 0);
				sloth_push(x, 0);
				return;
			}
		}
	}
	/* 4. r was a finite number. */
	for (i = 0; i < u; i++) sloth_cstore(x, addr + i, '0');
	for (i = 0, j = 0; i < strlen(buf); i++) {
		if (buf[i] == 'E') {
			sloth_push(x, (strtol(buf + i + 1, &endptr, 10)) + 1);
			break;
		} else if (buf[i] != '-' && buf[i] != '.') {
			sloth_cstore(x, addr + j, buf[i]);
			j++;
		}
	}
	sloth_push(x, r < 0.0 ? -1 : 0);
	/* When should this return 0 as invalid result? */
	sloth_push(x, -1);
}

/* Output operations */

void sloth_f_dot_(X* x) {
	FLOAT r = sloth_fpop(x);
	int int_digits = (r == 0.0) ? 1 : (int)log10(fabs(r)) + 1;
	int decimals;
	if (r == floor(r)) {
		printf("%.0f. ", r);
	} else if (floor(r) == 0.0 || floor(r) == -1.0) {
		printf("%.*f ", (int)sloth_user_area_get(x, SLOTH_PRECISION), r);
	} else {
		decimals = sloth_user_area_get(x, SLOTH_PRECISION) - int_digits;
		printf("%.*f ", decimals, r);
	}
}

void sloth_f_s_dot_(X* x) {
	printf("%.*E ", (int)sloth_user_area_get(x, SLOTH_PRECISION) - 1, sloth_fpop(x));
}

/* Engineering notation with special case handling */
/* by ChatGPT */
void sloth_f_e_dot_(X* x) {
	FLOAT r = sloth_fpop(x);
	int exp;
	double scaled;

	if (isnan(r)) {
		printf("NaN ");
		return;
	}
	
	if (isinf(r)) {
		if (r > 0) {
			printf("Inf ");
		} else {
			printf("-Inf ");
		}
		return;
	}
	
	if (r == 0.0) {
		/* Will not differentiate between positive and */
		/* negative zero. */
		printf("0.0E+00 ");
		return;
	}
	
	exp = (int)floor(log10(fabs(r)) / 3.0) * 3;
	scaled = r / pow(10, exp);
	
	/* Format with 3 digits after decimal — tweak to taste */
	printf("%.3fE%+03d ", scaled, exp);
}

/* Non ANS floating point helpers */

void sloth_f_dot_s_(X* x) {
	int i;
	printf("F:<%ld> ", x->fp);
	for (i = 0; i < x->fp; i++) printf("%f ", x->f[i]);
}

void sloth_bootstrap_floating_point_word_set(X* x) {

	/* == Primitives ===================================== */

	sloth_code(x, "(FLIT)", sloth_primitive(x, &sloth_flit_));

	/* == Floating point word set ======================== */

	/* Constructing compiler and interpreter system extensions */

	sloth_code(x, "FALIGN", sloth_primitive(x, &sloth_f_align_));
	sloth_code(x, "FALIGNED", sloth_primitive(x, &sloth_f_aligned_));
	sloth_code(x, "FLITERAL", sloth_primitive(x, &sloth_f_literal_));
	sloth_code(x, "FLOATS", sloth_primitive(x, &sloth_floats_));
	sloth_code(x, "FLOAT+", sloth_primitive(x, &sloth_float_plus_));

	sloth_code(x, "SFALIGNED", sloth_primitive(x, &sloth_s_f_aligned_));
	sloth_code(x, "DFALIGNED", sloth_primitive(x, &sloth_d_f_aligned_));

	sloth_code(x, "SFLOATS", sloth_primitive(x, &sloth_s_floats_));
	sloth_code(x, "DFLOATS", sloth_primitive(x, &sloth_d_floats_));

	/* Manipulating stack items */

	sloth_code(x, "FDEPTH", sloth_primitive(x, &sloth_f_depth_));
	sloth_code(x, "FDROP", sloth_primitive(x, &sloth_f_drop_));
	sloth_code(x, "FDUP", sloth_primitive(x, &sloth_f_dup_));
	sloth_code(x, "FOVER", sloth_primitive(x, &sloth_f_over_));
	sloth_code(x, "FROT", sloth_primitive(x, &sloth_f_rot_));
	sloth_code(x, "FSWAP", sloth_primitive(x, &sloth_f_swap_));

	/* Comparison operations */

	sloth_code(x, "F<", sloth_primitive(x, &sloth_f_less_than_));
	sloth_code(x, "F0<", sloth_primitive(x, &sloth_f_zero_less_than_));
	sloth_code(x, "F0=", sloth_primitive(x, &sloth_f_zero_equals_));

	/* Memory-stack transfer operations */

	sloth_code(x, "F@", sloth_primitive(x, &sloth_f_fetch_));
	sloth_code(x, "F!", sloth_primitive(x, &sloth_f_store_));

	sloth_code(x, "SF@", sloth_primitive(x, &sloth_s_f_fetch_));
	sloth_code(x, "SF!", sloth_primitive(x, &sloth_s_f_store_));

	sloth_code(x, "DF@", sloth_primitive(x, &sloth_d_f_fetch_));
	sloth_code(x, "DF!", sloth_primitive(x, &sloth_d_f_store_));

	/* Number-type conversion operators */

	sloth_code(x, "D>F", sloth_primitive(x, &sloth_d_to_f_));
	sloth_code(x, "F>D", sloth_primitive(x, &sloth_f_to_d_));
	/* Not needed: sloth_code(x, "S>F", sloth_primitive(x, &sloth_s_to_f_)); */

	/* Arithmetic and logical operations */

	sloth_code(x, "FABS", sloth_primitive(x, &sloth_f_abs_));
	sloth_code(x, "F+", sloth_primitive(x, &sloth_f_plus_));
	sloth_code(x, "F-", sloth_primitive(x, &sloth_f_minus_));
	sloth_code(x, "F*", sloth_primitive(x, &sloth_f_star_));
	sloth_code(x, "F**", sloth_primitive(x, &sloth_f_star_star_));
	sloth_code(x, "F/", sloth_primitive(x, &sloth_f_slash_));
	sloth_code(x, "FLOOR", sloth_primitive(x, &sloth_floor_));
	sloth_code(x, "FMAX", sloth_primitive(x, &sloth_f_max_));
	sloth_code(x, "FMIN", sloth_primitive(x, &sloth_f_min_));
	sloth_code(x, "FNEGATE", sloth_primitive(x, &sloth_f_negate_));
	sloth_code(x, "FROUND", sloth_primitive(x, &sloth_f_round_));
	sloth_code(x, "F~", sloth_primitive(x, &sloth_f_proximate_));
	sloth_code(x, "FATAN2", sloth_primitive(x, &sloth_f_atan2_));
	sloth_code(x, "FSQRT", sloth_primitive(x, &sloth_f_sqrt_));
	sloth_code(x, "FLN", sloth_primitive(x, &sloth_f_l_n_));
	sloth_code(x, "FSIN", sloth_primitive(x, &sloth_f_sine_));
	sloth_code(x, "FCOS", sloth_primitive(x, &sloth_f_cos_));
	sloth_code(x, "FSINCOS", sloth_primitive(x, &sloth_f_sine_cos_));
	sloth_code(x, "FTAN", sloth_primitive(x, &sloth_f_tan_));
	sloth_code(x, "FASIN", sloth_primitive(x, &sloth_f_a_sine_));
	sloth_code(x, "FACOS", sloth_primitive(x, &sloth_f_a_cos_));
	sloth_code(x, "FATAN", sloth_primitive(x, &sloth_f_a_tan_));
	sloth_code(x, "FEXP", sloth_primitive(x, &sloth_f_exp_));
	sloth_code(x, "FEXPM1", sloth_primitive(x, &sloth_f_exp_m_one_));
	sloth_code(x, "FLOG", sloth_primitive(x, &sloth_f_log_ten_));
	sloth_code(x, "FLNP1", sloth_primitive(x, &sloth_f_l_n_p_one_));
	sloth_code(x, "FALOG", sloth_primitive(x, &sloth_f_a_log_));
	sloth_code(x, "FSINH", sloth_primitive(x, &sloth_f_sin_h_));
	sloth_code(x, "FCOSH", sloth_primitive(x, &sloth_f_cos_h_));
	sloth_code(x, "FTANH", sloth_primitive(x, &sloth_f_tan_h_));
	sloth_code(x, "FASINH", sloth_primitive(x, &sloth_f_a_sine_h_));
	sloth_code(x, "FACOSH", sloth_primitive(x, &sloth_f_a_cos_h_));

	/* String/numeric conversion */

	sloth_code(x, ">FLOAT", sloth_primitive(x, &sloth_to_float_));
	sloth_code(x, "REPRESENT", sloth_primitive(x, &sloth_represent_));

	/* Output operations */

	sloth_code(x, "F.", sloth_primitive(x, &sloth_f_dot_));
	sloth_code(x, "FS.", sloth_primitive(x, &sloth_f_s_dot_));
	sloth_code(x, "FE.", sloth_primitive(x, &sloth_f_e_dot_));

	/* Non ANS floating point helpers */

	sloth_code(x, "F.S", sloth_primitive(x, &sloth_f_dot_s_));
}

void sloth_bootstrap(X* x) {
	sloth_bootstrap_kernel(x);
	sloth_bootstrap_floating_point_word_set(x);
}

#endif
#endif
