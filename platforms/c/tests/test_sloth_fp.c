#define UNITY_DOUBLE_PRECISION 1e-10
#include "sloth.h"
#include "unity.h"

#include <math.h>

#define EPSILON 1e-10
#define FLOAT_EPSILON 1e-6

#define SLOTH_F_ISNAN(x) ((x) != (x))

X* x;

void setUp(void) {
	x = sloth_create(256, 32767, 1024);
}

void tearDown(void) {
	sloth_free(x);
}

/* -- Float stack ----------------------------------------- */

void test_f_push_f_pop(void) {
	sloth_f_push(x, 1.5);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(1.5, x->f[0]);

	sloth_f_push(x, 2.5);
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(2.5, x->f[1]);

	TEST_ASSERT_EQUAL_FLOAT(2.5, sloth_f_pop(x));
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(1.5, sloth_f_pop(x));
	TEST_ASSERT_EQUAL(0, x->fp);
}

void test_f_pick(void) {
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 2.0);
	sloth_f_push(x, 3.0);
	TEST_ASSERT_EQUAL_FLOAT(3.0, sloth_f_pick(x, 0));
	TEST_ASSERT_EQUAL_FLOAT(2.0, sloth_f_pick(x, 1));
	TEST_ASSERT_EQUAL_FLOAT(1.0, sloth_f_pick(x, 2));
	TEST_ASSERT_EQUAL(3, x->fp); /* f_pick is non-destructive */
}

/* -- Float memory ---------------------------------------- */

void test_f_store_f_fetch(void) {
	FCELL val = 3.14159;
	CELL addr = sloth_to_abs(x, 1000);
	sloth_f_store(x, addr, val);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, val, sloth_f_fetch(x, addr));
}

void test_s_f_store_s_f_fetch(void) {
	SFCELL val = 2.71f;
	CELL addr = sloth_to_abs(x, 1000);
	sloth_s_f_store(x, addr, val);
	TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, val, sloth_s_f_fetch(x, addr));
}

void test_d_f_store_d_f_fetch(void) {
	DFCELL val = 1.41421356237;
	CELL addr = sloth_to_abs(x, 1000);
	sloth_d_f_store(x, addr, val);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, val, sloth_d_f_fetch(x, addr));
}

/* -- Float compilation ----------------------------------- */

void test_f_comma(void) {
	CELL here = sloth_here(x);
	sloth_f_comma(x, 2.718281828);
	TEST_ASSERT_EQUAL(here + sFCELL, sloth_here(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.718281828, sloth_f_fetch(x, here));
}

/* -- (FLIT) ---------------------------------------------- */

void test_f_lit_(void) {
	CELL here = sloth_here(x);
	sloth_f_comma(x, 1.234);
	x->ip = here;
	sloth_f_lit_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.234, sloth_f_pop(x));
	TEST_ASSERT_EQUAL(here + sFCELL, x->ip);
}

/* -- Alignment ------------------------------------------- */

void test_f_align_(void) {
	/* Force HERE to be misaligned relative to FCELL */
	sloth_allot(x, 1);
	sloth_f_align_(x);
	TEST_ASSERT_EQUAL(0, sloth_here(x) % sFCELL);
}

void test_f_aligned_(void) {
	CELL result, unaligned = sloth_to_abs(x, 3);
	sloth_push(x, unaligned);
	sloth_f_aligned_(x);
	result = sloth_pop(x);
	TEST_ASSERT_EQUAL(0, result % sFCELL);
	TEST_ASSERT_TRUE(result >= unaligned);
}

void test_s_f_aligned_(void) {
	CELL result, unaligned = sloth_to_abs(x, 3);
	sloth_push(x, unaligned);
	sloth_s_f_aligned_(x);
	result = sloth_pop(x);
	TEST_ASSERT_EQUAL(0, result % sSFCELL);
	TEST_ASSERT_TRUE(result >= unaligned);
}

void test_d_f_aligned_(void) {
	CELL result, unaligned = sloth_to_abs(x, 3);
	sloth_push(x, unaligned);
	sloth_d_f_aligned_(x);
	result = sloth_pop(x);
	TEST_ASSERT_EQUAL(0, result % sDFCELL);
	TEST_ASSERT_TRUE(result >= unaligned);
}

/* -- Size words ------------------------------------------ */

void test_floats_(void) {
	sloth_push(x, 3);
	sloth_floats_(x);
	TEST_ASSERT_EQUAL(3 * sFCELL, sloth_pop(x));
}

void test_s_floats_(void) {
	sloth_push(x, 3);
	sloth_s_floats_(x);
	TEST_ASSERT_EQUAL(3 * sSFCELL, sloth_pop(x));
}

void test_d_floats_(void) {
	sloth_push(x, 3);
	sloth_d_floats_(x);
	TEST_ASSERT_EQUAL(3 * sDFCELL, sloth_pop(x));
}

/* -- Stack manipulation ---------------------------------- */

void test_f_depth_(void) {
	sloth_f_depth_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_push(x, 2.0);
	sloth_f_depth_(x);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));
}

void test_f_drop_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 2.0);
	sloth_f_drop_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(1.0, x->f[0]);
}

void test_f_dup_(void) {
	sloth_f_push(x, 3.5);
	sloth_f_dup_(x);
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_f_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_f_pop(x));
}

void test_f_over_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 2.0);
	sloth_f_over_(x);
	/* F: 1.0 2.0 1.0 */
	TEST_ASSERT_EQUAL(3, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));
}

void test_f_swap_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 2.0);
	sloth_f_swap_(x);
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));
}

void test_f_rot_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 2.0);
	sloth_f_push(x, 3.0);
	sloth_f_rot_(x);
	/* F: 1.0 2.0 3.0 -- F: 2.0 3.0 1.0 */
	TEST_ASSERT_EQUAL(3, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_f_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));
}

/* -- Comparisons ----------------------------------------- */

void test_f_less_than_(void) {
	/* 1.0 < 2.0 = true */
	sloth_f_push(x, 1.0); sloth_f_push(x, 2.0);
	sloth_f_less_than_(x);
	TEST_ASSERT_EQUAL(0, x->fp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	/* 2.0 < 1.0 = false */
	sloth_f_push(x, 2.0); sloth_f_push(x, 1.0);
	sloth_f_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	/* 1.0 < 1.0 = false */
	sloth_f_push(x, 1.0); sloth_f_push(x, 1.0);
	sloth_f_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_zero_less_than_(void) {
	sloth_f_push(x, -1.0);
	sloth_f_zero_less_than_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_f_push(x, 0.0);
	sloth_f_zero_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_zero_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_zero_equals_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_zero_equals_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_zero_equals_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_f_push(x, -0.0);
	sloth_f_zero_equals_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
}

/* -- Memory/stack transfer ------------------------------- */

void test_f_fetch_store_(void) {
	CELL addr = sloth_to_abs(x, 1000);
	sloth_f_push(x, 9.99);
	sloth_push(x, addr);
	sloth_f_store_(x);
	TEST_ASSERT_EQUAL(0, x->fp);

	sloth_push(x, addr);
	sloth_f_fetch_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 9.99, sloth_f_pop(x));
}

void test_s_f_fetch_store_(void) {
	CELL addr = sloth_to_abs(x, 1000);
	sloth_f_push(x, 3.14);
	sloth_push(x, addr);
	sloth_s_f_store_(x);
	TEST_ASSERT_EQUAL(0, x->fp);

	sloth_push(x, addr);
	sloth_s_f_fetch_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 3.14f, (float)sloth_f_pop(x));
}

void test_d_f_fetch_store_(void) {
	CELL addr = sloth_to_abs(x, 1000);
	sloth_f_push(x, 2.71828182845);
	sloth_push(x, addr);
	sloth_d_f_store_(x);
	TEST_ASSERT_EQUAL(0, x->fp);

	sloth_push(x, addr);
	sloth_d_f_fetch_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.71828182845, sloth_f_pop(x));
}

/* -- Conversion ------------------------------------------ */

void test_d_to_f_positive(void) {
	/* D>F: 3. ( lo=3 hi=0 ) -> 3.0 */
	sloth_push(x, 3);
	sloth_push(x, 0);
	sloth_d_to_f_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_f_pop(x));
}

void test_d_to_f_negative(void) {
	/* D>F: -3. ( lo=-3 hi=-1 ) -> -3.0 */
	sloth_push(x, -3);
	sloth_push(x, -1);
	sloth_d_to_f_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -3.0, sloth_f_pop(x));
}

void test_d_to_f_zero(void) {
	sloth_push(x, 0);
	sloth_push(x, 0);
	sloth_d_to_f_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));
}

void test_f_to_d_positive(void) {
	/* F>D: 3.7 -> lo=3 hi=0 (truncates toward zero) */
	sloth_f_push(x, 3.7);
	sloth_f_to_d_(x);
	TEST_ASSERT_EQUAL(0, x->fp);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));  /* hi */
	TEST_ASSERT_EQUAL(3, sloth_pop(x));  /* lo */
}

void test_f_to_d_negative(void) {
	/* F>D: -3.7 -> lo=-3 hi=-1 */
	sloth_f_push(x, -3.7);
	sloth_f_to_d_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x)); /* hi */
	TEST_ASSERT_EQUAL(-3, sloth_pop(x)); /* lo */
}

void test_f_to_d_zero(void) {
	sloth_f_push(x, 0.0);
	sloth_f_to_d_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

/* -- Arithmetic ------------------------------------------ */

void test_f_plus_(void) {
	sloth_f_push(x, 1.5);
	sloth_f_push(x, 2.5);
	sloth_f_plus_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 4.0, sloth_f_pop(x));
}

void test_f_minus_(void) {
	sloth_f_push(x, 5.0);
	sloth_f_push(x, 1.5);
	sloth_f_minus_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_f_pop(x));
}

void test_f_star_(void) {
	sloth_f_push(x, 3.0);
	sloth_f_push(x, 4.0);
	sloth_f_star_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 12.0, sloth_f_pop(x));
}

void test_f_slash_(void) {
	sloth_f_push(x, 10.0);
	sloth_f_push(x, 4.0);
	sloth_f_slash_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.5, sloth_f_pop(x));
}

void test_f_star_star_(void) {
	sloth_f_push(x, 2.0);
	sloth_f_push(x, 10.0);
	sloth_f_star_star_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1024.0, sloth_f_pop(x));

	/* 9.0 ** 0.5 = 3.0 */
	sloth_f_push(x, 9.0);
	sloth_f_push(x, 0.5);
	sloth_f_star_star_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_f_pop(x));
}

void test_f_abs_(void) {
	sloth_f_push(x, -3.5);
	sloth_f_abs_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_f_pop(x));

	sloth_f_push(x, 3.5);
	sloth_f_abs_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_f_pop(x));

	sloth_f_push(x, 0.0);
	sloth_f_abs_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));
}

void test_f_negate_(void) {
	sloth_f_push(x, 3.5);
	sloth_f_negate_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -3.5, sloth_f_pop(x));

	sloth_f_push(x, -2.0);
	sloth_f_negate_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));

	sloth_f_push(x, 0.0);
	sloth_f_negate_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));
}

void test_f_max_(void) {
	sloth_f_push(x, 3.0);
	sloth_f_push(x, 5.0);
	sloth_f_max_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 5.0, sloth_f_pop(x));

	sloth_f_push(x, 5.0);
	sloth_f_push(x, 3.0);
	sloth_f_max_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 5.0, sloth_f_pop(x));

	sloth_f_push(x, -1.0);
	sloth_f_push(x, -2.0);
	sloth_f_max_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -1.0, sloth_f_pop(x));
}

void test_f_min_(void) {
	sloth_f_push(x, 3.0);
	sloth_f_push(x, 5.0);
	sloth_f_min_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_f_pop(x));

	sloth_f_push(x, 5.0);
	sloth_f_push(x, 3.0);
	sloth_f_min_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_f_pop(x));

	sloth_f_push(x, -1.0);
	sloth_f_push(x, -2.0);
	sloth_f_min_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -2.0, sloth_f_pop(x));
}

void test_floor_(void) {
	sloth_f_push(x, 2.7);
	sloth_floor_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));

	sloth_f_push(x, -2.7);
	sloth_floor_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -3.0, sloth_f_pop(x));

	sloth_f_push(x, 2.0);
	sloth_floor_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));
}

void test_f_round_(void) {
	sloth_f_push(x, 0.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, -0.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));

	sloth_f_push(x, -1.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -2.0, sloth_f_pop(x));

	sloth_f_push(x, 2.4);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));
}

/* -- Proximate (F~) -------------------------------------- */

void test_f_proximate_exact(void) {
	/* r3 = 0.0: exact bitwise comparison */
	sloth_f_push(x, 1.5);
	sloth_f_push(x, 1.5);
	sloth_f_push(x, 0.0);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_f_push(x, 1.5);
	sloth_f_push(x, 1.6);
	sloth_f_push(x, 0.0);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_proximate_absolute(void) {
	/* r3 > 0: absolute tolerance */
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 1.09);
	sloth_f_push(x, 0.1);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_push(x, 1.11);
	sloth_f_push(x, 0.1);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_proximate_relative(void) {
	/* r3 < 0: relative tolerance |r1-r2| < |r3|*(|r1|+|r2|) */
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 1.001);
	sloth_f_push(x, -0.01);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_push(x, 1.1);
	sloth_f_push(x, -0.01);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

/* -- Transcendental functions ---------------------------- */

void test_f_sqrt_(void) {
	sloth_f_push(x, 4.0);
	sloth_f_sqrt_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));

	sloth_f_push(x, 2.0);
	sloth_f_sqrt_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.41421356237, sloth_f_pop(x));

	sloth_f_push(x, 0.0);
	sloth_f_sqrt_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));
}

void test_f_l_n_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_l_n_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, M_E);
	sloth_f_l_n_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, 10.0);
	sloth_f_l_n_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.302585092994, sloth_f_pop(x));
}

void test_f_exp_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_exp_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_exp_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_E, sloth_f_pop(x));

	sloth_f_push(x, -1.0);
	sloth_f_exp_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0 / M_E, sloth_f_pop(x));
}

void test_f_exp_m_one_(void) {
	/* FEXPM1 should be accurate near zero where FEXP-1 loses bits */
	sloth_f_push(x, 0.0);
	sloth_f_exp_m_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_exp_m_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_E - 1.0, sloth_f_pop(x));

	/* Near-zero value: implementation uses exp(x)-1.0 which loses a few ULPs
	   due to cancellation. Tolerance is 1e-17 to accommodate this. A port using
	   expm1() directly would be accurate to within 1 ULP (~1e-20 at this input). */
	sloth_f_push(x, 1e-10);
	sloth_f_exp_m_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(1e-17, 1e-10, sloth_f_pop(x));
}

void test_f_log_ten_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_log_ten_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 10.0);
	sloth_f_log_ten_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, 100.0);
	sloth_f_log_ten_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_f_pop(x));
}

void test_f_l_n_p_one_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_l_n_p_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, M_E - 1.0);
	sloth_f_l_n_p_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));
}

void test_f_a_log_(void) {
	/* FALOG: 10^r */
	sloth_f_push(x, 0.0);
	sloth_f_a_log_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_a_log_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 10.0, sloth_f_pop(x));

	sloth_f_push(x, 2.0);
	sloth_f_a_log_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 100.0, sloth_f_pop(x));
}

/* -- Trig ------------------------------------------------ */

void test_f_sine_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, M_PI / 2.0);
	sloth_f_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, M_PI);
	sloth_f_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));
}

void test_f_cos_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, M_PI / 2.0);
	sloth_f_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, M_PI);
	sloth_f_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -1.0, sloth_f_pop(x));
}

void test_f_sine_cos_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_sine_cos_(x);
	/* ANS: F: r -- r1(sin) r2(cos), TOS = cos */
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x)); /* cos */
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x)); /* sin */
}

void test_f_tan_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, M_PI / 4.0);
	sloth_f_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));
}

void test_f_atan2_(void) {
	/* atan2(0, 1) = 0 */
	sloth_f_push(x, 0.0);
	sloth_f_push(x, 1.0);
	sloth_f_atan2_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	/* atan2(1, 1) = pi/4 */
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 1.0);
	sloth_f_atan2_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 4.0, sloth_f_pop(x));

	/* atan2(1, 0) = pi/2 */
	sloth_f_push(x, 1.0);
	sloth_f_push(x, 0.0);
	sloth_f_atan2_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 2.0, sloth_f_pop(x));
}

void test_f_a_sine_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_a_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_a_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 2.0, sloth_f_pop(x));
}

void test_f_a_cos_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_a_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 0.0);
	sloth_f_a_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 2.0, sloth_f_pop(x));
}

void test_f_a_tan_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_a_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_a_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 4.0, sloth_f_pop(x));
}

/* -- Hyperbolic ------------------------------------------ */

void test_f_sin_h_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_sin_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_sin_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, sinh(1.0), sloth_f_pop(x));
}

void test_f_cos_h_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, cosh(1.0), sloth_f_pop(x));
}

void test_f_tan_h_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_tan_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_tan_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, tanh(1.0), sloth_f_pop(x));
}

void test_f_a_sine_h_(void) {
	sloth_f_push(x, 0.0);
	sloth_f_a_sine_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 1.0);
	sloth_f_a_sine_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, log(1.0 + sqrt(2.0)), sloth_f_pop(x));

	/* Negative input */
	sloth_f_push(x, -1.0);
	sloth_f_a_sine_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -log(1.0 + sqrt(2.0)), sloth_f_pop(x));
}

void test_f_a_cos_h_(void) {
	sloth_f_push(x, 1.0);
	sloth_f_a_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));

	sloth_f_push(x, 2.0);
	sloth_f_a_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, log(2.0 + sqrt(3.0)), sloth_f_pop(x));

	/* Domain error: r < 1.0 should produce NaN */
	sloth_f_push(x, 0.5);
	sloth_f_a_cos_h_(x);
	TEST_ASSERT_TRUE(SLOTH_F_ISNAN(sloth_f_pop(x)));
}

/* -- String/numeric conversion --------------------------- */

void test_to_float_integer(void) {
	char *s = "42";
	sloth_push(x, (CELL)s);
	sloth_push(x, 2);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x)); /* success flag */
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 42.0, sloth_f_pop(x));
}

void test_to_float_decimal(void) {
	char *s = "3.14";
	sloth_push(x, (CELL)s);
	sloth_push(x, 4);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.14, sloth_f_pop(x));
}

void test_to_float_exponent(void) {
	char *s = "1E3";
	sloth_push(x, (CELL)s);
	sloth_push(x, 3);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1000.0, sloth_f_pop(x));
}

void test_to_float_negative(void) {
	char *s = "-2.5";
	sloth_push(x, (CELL)s);
	sloth_push(x, 4);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -2.5, sloth_f_pop(x));
}

void test_to_float_blanks_are_zero(void) {
	/* A string of blanks represents 0E per ANS */
	char *s = "   ";
	sloth_push(x, (CELL)s);
	sloth_push(x, 3);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_f_pop(x));
}

void test_to_float_trailing_space_fails(void) {
	/* Trailing space must fail */
	char *s = "1.0 ";
	sloth_push(x, (CELL)s);
	sloth_push(x, 4);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->fp);
}

void test_to_float_invalid_fails(void) {
	char *s = "abc";
	sloth_push(x, (CELL)s);
	sloth_push(x, 3);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->fp);
}

/* -- REPRESENT ------------------------------------------- */

void test_represent_positive(void) {
	char buf[32] = {0};
	CELL flag, sign, n, addr = (CELL)buf;
	sloth_f_push(x, 1.5);
	sloth_push(x, addr);
	sloth_push(x, 6); /* 6 significant digits */
	sloth_represent_(x);
	/* Stack: n sign flag */
	flag = sloth_pop(x);
	sign = sloth_pop(x);
	n    = sloth_pop(x);
	TEST_ASSERT_EQUAL(-1, flag);          /* valid number */
	TEST_ASSERT_EQUAL(0, sign);           /* positive */
	TEST_ASSERT_EQUAL(1, n);              /* decimal point after first digit: 1.5e0 -> n=1 */
	TEST_ASSERT_EQUAL('1', buf[0]);
	TEST_ASSERT_EQUAL('5', buf[1]);
}

void test_represent_negative(void) {
	char buf[32] = {0};
	CELL flag, sign, n, addr = (CELL)buf;
	sloth_f_push(x, -1.5);
	sloth_push(x, addr);
	sloth_push(x, 6);
	sloth_represent_(x);
	flag = sloth_pop(x);
	sign = sloth_pop(x);
	n    = sloth_pop(x);
	TEST_ASSERT_EQUAL(-1, flag);
	TEST_ASSERT_EQUAL(-1, sign);   /* negative */
	TEST_ASSERT_EQUAL(1, n);
}

void test_represent_large_exponent(void) {
	char buf[32] = {0};
	CELL flag, sign, n, addr = (CELL)buf;
	sloth_f_push(x, 1000.0);
	sloth_push(x, addr);
	sloth_push(x, 4);
	sloth_represent_(x);
	flag = sloth_pop(x);
	sign = sloth_pop(x);
	n    = sloth_pop(x);
	TEST_ASSERT_EQUAL(-1, flag);
	TEST_ASSERT_EQUAL(0, sign);
	TEST_ASSERT_EQUAL(4, n);  /* 1000 -> digits "1000", decimal after position 4 */
}

int main(void) {
	UNITY_BEGIN();

	/* Float stack */
	RUN_TEST(test_f_push_f_pop);
	RUN_TEST(test_f_pick);

	/* Float memory */
	RUN_TEST(test_f_store_f_fetch);
	RUN_TEST(test_s_f_store_s_f_fetch);
	RUN_TEST(test_d_f_store_d_f_fetch);

	/* Float compilation */
	RUN_TEST(test_f_comma);

	/* (FLIT) */
	RUN_TEST(test_f_lit_);

	/* Alignment */
	RUN_TEST(test_f_align_);
	RUN_TEST(test_f_aligned_);
	RUN_TEST(test_s_f_aligned_);
	RUN_TEST(test_d_f_aligned_);

	/* Size words */
	RUN_TEST(test_floats_);
	RUN_TEST(test_s_floats_);
	RUN_TEST(test_d_floats_);

	/* Stack manipulation */
	RUN_TEST(test_f_depth_);
	RUN_TEST(test_f_drop_);
	RUN_TEST(test_f_dup_);
	RUN_TEST(test_f_over_);
	RUN_TEST(test_f_swap_);
	RUN_TEST(test_f_rot_);

	/* Comparisons */
	RUN_TEST(test_f_less_than_);
	RUN_TEST(test_f_zero_less_than_);
	RUN_TEST(test_f_zero_equals_);

	/* Memory/stack transfer */
	RUN_TEST(test_f_fetch_store_);
	RUN_TEST(test_s_f_fetch_store_);
	RUN_TEST(test_d_f_fetch_store_);

	/* Conversion */
	RUN_TEST(test_d_to_f_positive);
	RUN_TEST(test_d_to_f_negative);
	RUN_TEST(test_d_to_f_zero);
	RUN_TEST(test_f_to_d_positive);
	RUN_TEST(test_f_to_d_negative);
	RUN_TEST(test_f_to_d_zero);

	/* Arithmetic */
	RUN_TEST(test_f_plus_);
	RUN_TEST(test_f_minus_);
	RUN_TEST(test_f_star_);
	RUN_TEST(test_f_slash_);
	RUN_TEST(test_f_star_star_);
	RUN_TEST(test_f_abs_);
	RUN_TEST(test_f_negate_);
	RUN_TEST(test_f_max_);
	RUN_TEST(test_f_min_);
	RUN_TEST(test_floor_);
	RUN_TEST(test_f_round_);

	/* Proximate */
	RUN_TEST(test_f_proximate_exact);
	RUN_TEST(test_f_proximate_absolute);
	RUN_TEST(test_f_proximate_relative);

	/* Transcendental */
	RUN_TEST(test_f_sqrt_);
	RUN_TEST(test_f_l_n_);
	RUN_TEST(test_f_exp_);
	RUN_TEST(test_f_exp_m_one_);
	RUN_TEST(test_f_log_ten_);
	RUN_TEST(test_f_l_n_p_one_);
	RUN_TEST(test_f_a_log_);

	/* Trig */
	RUN_TEST(test_f_sine_);
	RUN_TEST(test_f_cos_);
	RUN_TEST(test_f_sine_cos_);
	RUN_TEST(test_f_tan_);
	RUN_TEST(test_f_atan2_);
	RUN_TEST(test_f_a_sine_);
	RUN_TEST(test_f_a_cos_);
	RUN_TEST(test_f_a_tan_);

	/* Hyperbolic */
	RUN_TEST(test_f_sin_h_);
	RUN_TEST(test_f_cos_h_);
	RUN_TEST(test_f_tan_h_);
	RUN_TEST(test_f_a_sine_h_);
	RUN_TEST(test_f_a_cos_h_);

	/* String/numeric conversion */
	RUN_TEST(test_to_float_integer);
	RUN_TEST(test_to_float_decimal);
	RUN_TEST(test_to_float_exponent);
	RUN_TEST(test_to_float_negative);
	RUN_TEST(test_to_float_blanks_are_zero);
	RUN_TEST(test_to_float_trailing_space_fails);
	RUN_TEST(test_to_float_invalid_fails);

	/* REPRESENT */
	RUN_TEST(test_represent_positive);
	RUN_TEST(test_represent_negative);
	RUN_TEST(test_represent_large_exponent);

	return UNITY_END();
}
