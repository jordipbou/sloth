#define UNITY_INCLUDE_DOUBLE
#define UNITY_DOUBLE_PRECISION 1e-10
#include "sloth.h"
#include "unity.h"

#include <math.h>

#define EPSILON 1e-10
#define FLOAT_EPSILON 1e-6

X* x;

void setUp() {
	x = sloth_create(256, 32767, 1024);
}

void tearDown() {
	sloth_free(x);
}

/* -- Float stack ----------------------------------------- */

void test_fpush_fpop() {
	sloth_fpush(x, 1.5);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(1.5, x->f[0]);

	sloth_fpush(x, 2.5);
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(2.5, x->f[1]);

	TEST_ASSERT_EQUAL_FLOAT(2.5, sloth_fpop(x));
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(1.5, sloth_fpop(x));
	TEST_ASSERT_EQUAL(0, x->fp);
}

void test_fpick() {
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 2.0);
	sloth_fpush(x, 3.0);
	TEST_ASSERT_EQUAL_FLOAT(3.0, sloth_fpick(x, 0));
	TEST_ASSERT_EQUAL_FLOAT(2.0, sloth_fpick(x, 1));
	TEST_ASSERT_EQUAL_FLOAT(1.0, sloth_fpick(x, 2));
	TEST_ASSERT_EQUAL(3, x->fp); /* fpick is non-destructive */
}

/* -- Float memory ---------------------------------------- */

void test_fstore_ffetch() {
	FCELL val = 3.14159;
	CELL addr = sloth_to_abs(x, 1000);
	sloth_fstore(x, addr, val);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, val, sloth_ffetch(x, addr));
}

void test_sfstore_sffetch() {
	SFCELL val = 2.71f;
	CELL addr = sloth_to_abs(x, 1000);
	sloth_sfstore(x, addr, val);
	TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, val, sloth_sffetch(x, addr));
}

void test_dfstore_dffetch() {
	DFCELL val = 1.41421356237;
	CELL addr = sloth_to_abs(x, 1000);
	sloth_dfstore(x, addr, val);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, val, sloth_dffetch(x, addr));
}

/* -- Float compilation ----------------------------------- */

void test_fcomma() {
	CELL here = sloth_here(x);
	sloth_fcomma(x, 2.718281828);
	TEST_ASSERT_EQUAL(here + sFCELL, sloth_here(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.718281828, sloth_ffetch(x, here));
}

/* -- (FLIT) ---------------------------------------------- */

void test_flit_() {
	CELL here = sloth_here(x);
	sloth_fcomma(x, 1.234);
	x->ip = here;
	sloth_flit_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.234, sloth_fpop(x));
	TEST_ASSERT_EQUAL(here + sFCELL, x->ip);
}

/* -- Alignment ------------------------------------------- */

void test_f_align_() {
	/* Force HERE to be misaligned relative to FCELL */
	sloth_allot(x, 1);
	sloth_f_align_(x);
	TEST_ASSERT_EQUAL(0, sloth_here(x) % sFCELL);
}

void test_f_aligned_() {
	CELL unaligned = sloth_to_abs(x, 3);
	sloth_push(x, unaligned);
	sloth_f_aligned_(x);
	CELL result = sloth_pop(x);
	TEST_ASSERT_EQUAL(0, result % sFCELL);
	TEST_ASSERT_TRUE(result >= unaligned);
}

void test_s_f_aligned_() {
	CELL unaligned = sloth_to_abs(x, 3);
	sloth_push(x, unaligned);
	sloth_s_f_aligned_(x);
	CELL result = sloth_pop(x);
	TEST_ASSERT_EQUAL(0, result % sSFCELL);
	TEST_ASSERT_TRUE(result >= unaligned);
}

void test_d_f_aligned_() {
	CELL unaligned = sloth_to_abs(x, 3);
	sloth_push(x, unaligned);
	sloth_d_f_aligned_(x);
	CELL result = sloth_pop(x);
	TEST_ASSERT_EQUAL(0, result % sDFCELL);
	TEST_ASSERT_TRUE(result >= unaligned);
}

/* -- Size words ------------------------------------------ */

void test_floats_() {
	sloth_push(x, 3);
	sloth_floats_(x);
	TEST_ASSERT_EQUAL(3 * sFCELL, sloth_pop(x));
}

void test_s_floats_() {
	sloth_push(x, 3);
	sloth_s_floats_(x);
	TEST_ASSERT_EQUAL(3 * sSFCELL, sloth_pop(x));
}

void test_d_floats_() {
	sloth_push(x, 3);
	sloth_d_floats_(x);
	TEST_ASSERT_EQUAL(3 * sDFCELL, sloth_pop(x));
}

/* -- Stack manipulation ---------------------------------- */

void test_f_depth_() {
	sloth_f_depth_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_fpush(x, 1.0);
	sloth_fpush(x, 2.0);
	sloth_f_depth_(x);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));
}

void test_f_drop_() {
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 2.0);
	sloth_f_drop_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_EQUAL_FLOAT(1.0, x->f[0]);
}

void test_f_dup_() {
	sloth_fpush(x, 3.5);
	sloth_f_dup_(x);
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_fpop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_fpop(x));
}

void test_f_over_() {
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 2.0);
	sloth_f_over_(x);
	/* F: 1.0 2.0 1.0 */
	TEST_ASSERT_EQUAL(3, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));
}

void test_f_swap_() {
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 2.0);
	sloth_f_swap_(x);
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));
}

void test_f_rot_() {
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 2.0);
	sloth_fpush(x, 3.0);
	sloth_f_rot_(x);
	/* F: 1.0 2.0 3.0 -- F: 2.0 3.0 1.0 */
	TEST_ASSERT_EQUAL(3, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_fpop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));
}

/* -- Comparisons ----------------------------------------- */

void test_f_less_than_() {
	/* 1.0 < 2.0 = true */
	sloth_fpush(x, 1.0); sloth_fpush(x, 2.0);
	sloth_f_less_than_(x);
	TEST_ASSERT_EQUAL(0, x->fp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	/* 2.0 < 1.0 = false */
	sloth_fpush(x, 2.0); sloth_fpush(x, 1.0);
	sloth_f_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	/* 1.0 < 1.0 = false */
	sloth_fpush(x, 1.0); sloth_fpush(x, 1.0);
	sloth_f_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_zero_less_than_() {
	sloth_fpush(x, -1.0);
	sloth_f_zero_less_than_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_fpush(x, 0.0);
	sloth_f_zero_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_fpush(x, 1.0);
	sloth_f_zero_less_than_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_zero_equals_() {
	sloth_fpush(x, 0.0);
	sloth_f_zero_equals_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_fpush(x, 1.0);
	sloth_f_zero_equals_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_fpush(x, -0.0);
	sloth_f_zero_equals_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
}

/* -- Memory/stack transfer ------------------------------- */

void test_f_fetch_store_() {
	CELL addr = sloth_to_abs(x, 1000);
	sloth_fpush(x, 9.99);
	sloth_push(x, addr);
	sloth_f_store_(x);
	TEST_ASSERT_EQUAL(0, x->fp);

	sloth_push(x, addr);
	sloth_f_fetch_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 9.99, sloth_fpop(x));
}

void test_sf_fetch_store_() {
	CELL addr = sloth_to_abs(x, 1000);
	sloth_fpush(x, 3.14);
	sloth_push(x, addr);
	sloth_s_f_store_(x);
	TEST_ASSERT_EQUAL(0, x->fp);

	sloth_push(x, addr);
	sloth_s_f_fetch_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 3.14f, (float)sloth_fpop(x));
}

void test_df_fetch_store_() {
	CELL addr = sloth_to_abs(x, 1000);
	sloth_fpush(x, 2.71828182845);
	sloth_push(x, addr);
	sloth_d_f_store_(x);
	TEST_ASSERT_EQUAL(0, x->fp);

	sloth_push(x, addr);
	sloth_d_f_fetch_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.71828182845, sloth_fpop(x));
}

/* -- Conversion ------------------------------------------ */

void test_d_to_f_positive() {
	/* D>F: 3. ( lo=3 hi=0 ) -> 3.0 */
	sloth_push(x, 3);
	sloth_push(x, 0);
	sloth_d_to_f_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_fpop(x));
}

void test_d_to_f_negative() {
	/* D>F: -3. ( lo=-3 hi=-1 ) -> -3.0 */
	sloth_push(x, -3);
	sloth_push(x, -1);
	sloth_d_to_f_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -3.0, sloth_fpop(x));
}

void test_d_to_f_zero() {
	sloth_push(x, 0);
	sloth_push(x, 0);
	sloth_d_to_f_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));
}

void test_f_to_d_positive() {
	/* F>D: 3.7 -> lo=3 hi=0 (truncates toward zero) */
	sloth_fpush(x, 3.7);
	sloth_f_to_d_(x);
	TEST_ASSERT_EQUAL(0, x->fp);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));  /* hi */
	TEST_ASSERT_EQUAL(3, sloth_pop(x));  /* lo */
}

void test_f_to_d_negative() {
	/* F>D: -3.7 -> lo=-3 hi=-1 */
	sloth_fpush(x, -3.7);
	sloth_f_to_d_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x)); /* hi */
	TEST_ASSERT_EQUAL(-3, sloth_pop(x)); /* lo */
}

void test_f_to_d_zero() {
	sloth_fpush(x, 0.0);
	sloth_f_to_d_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

/* -- Arithmetic ------------------------------------------ */

void test_f_plus_() {
	sloth_fpush(x, 1.5);
	sloth_fpush(x, 2.5);
	sloth_f_plus_(x);
	TEST_ASSERT_EQUAL(1, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 4.0, sloth_fpop(x));
}

void test_f_minus_() {
	sloth_fpush(x, 5.0);
	sloth_fpush(x, 1.5);
	sloth_f_minus_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_fpop(x));
}

void test_f_star_() {
	sloth_fpush(x, 3.0);
	sloth_fpush(x, 4.0);
	sloth_f_star_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 12.0, sloth_fpop(x));
}

void test_f_slash_() {
	sloth_fpush(x, 10.0);
	sloth_fpush(x, 4.0);
	sloth_f_slash_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.5, sloth_fpop(x));
}

void test_f_star_star_() {
	sloth_fpush(x, 2.0);
	sloth_fpush(x, 10.0);
	sloth_f_star_star_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1024.0, sloth_fpop(x));

	/* 9.0 ** 0.5 = 3.0 */
	sloth_fpush(x, 9.0);
	sloth_fpush(x, 0.5);
	sloth_f_star_star_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_fpop(x));
}

void test_f_abs_() {
	sloth_fpush(x, -3.5);
	sloth_f_abs_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_fpop(x));

	sloth_fpush(x, 3.5);
	sloth_f_abs_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.5, sloth_fpop(x));

	sloth_fpush(x, 0.0);
	sloth_f_abs_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));
}

void test_f_negate_() {
	sloth_fpush(x, 3.5);
	sloth_f_negate_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -3.5, sloth_fpop(x));

	sloth_fpush(x, -2.0);
	sloth_f_negate_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));

	sloth_fpush(x, 0.0);
	sloth_f_negate_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));
}

void test_f_max_() {
	sloth_fpush(x, 3.0);
	sloth_fpush(x, 5.0);
	sloth_f_max_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 5.0, sloth_fpop(x));

	sloth_fpush(x, 5.0);
	sloth_fpush(x, 3.0);
	sloth_f_max_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 5.0, sloth_fpop(x));

	sloth_fpush(x, -1.0);
	sloth_fpush(x, -2.0);
	sloth_f_max_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -1.0, sloth_fpop(x));
}

void test_f_min_() {
	sloth_fpush(x, 3.0);
	sloth_fpush(x, 5.0);
	sloth_f_min_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_fpop(x));

	sloth_fpush(x, 5.0);
	sloth_fpush(x, 3.0);
	sloth_f_min_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.0, sloth_fpop(x));

	sloth_fpush(x, -1.0);
	sloth_fpush(x, -2.0);
	sloth_f_min_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -2.0, sloth_fpop(x));
}

void test_floor_() {
	sloth_fpush(x, 2.7);
	sloth_floor_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));

	sloth_fpush(x, -2.7);
	sloth_floor_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -3.0, sloth_fpop(x));

	sloth_fpush(x, 2.0);
	sloth_floor_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));
}

void test_f_round_() {
	/* Rounds half to nearest even ("banker's rounding") is NOT required;
	   ANS Forth requires "round to nearest, ties away from zero". */
	sloth_fpush(x, 0.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, -0.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -1.0, sloth_fpop(x));

	sloth_fpush(x, 1.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));

	sloth_fpush(x, -1.5);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -2.0, sloth_fpop(x));

	sloth_fpush(x, 2.4);
	sloth_f_round_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));
}

/* -- Proximate (F~) -------------------------------------- */

void test_f_proximate_exact() {
	/* r3 = 0.0: exact bitwise comparison */
	sloth_fpush(x, 1.5);
	sloth_fpush(x, 1.5);
	sloth_fpush(x, 0.0);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_fpush(x, 1.5);
	sloth_fpush(x, 1.6);
	sloth_fpush(x, 0.0);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_proximate_absolute() {
	/* r3 > 0: absolute tolerance */
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 1.09);
	sloth_fpush(x, 0.1);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_fpush(x, 1.0);
	sloth_fpush(x, 1.11);
	sloth_fpush(x, 0.1);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_f_proximate_relative() {
	/* r3 < 0: relative tolerance |r1-r2| < |r3|*(|r1|+|r2|) */
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 1.001);
	sloth_fpush(x, -0.01);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_fpush(x, 1.0);
	sloth_fpush(x, 1.1);
	sloth_fpush(x, -0.01);
	sloth_f_proximate_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

/* -- Transcendental functions ---------------------------- */

void test_f_sqrt_() {
	sloth_fpush(x, 4.0);
	sloth_f_sqrt_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));

	sloth_fpush(x, 2.0);
	sloth_f_sqrt_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.41421356237, sloth_fpop(x));

	sloth_fpush(x, 0.0);
	sloth_f_sqrt_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));
}

void test_f_l_n_() {
	sloth_fpush(x, 1.0);
	sloth_f_l_n_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, M_E);
	sloth_f_l_n_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, 10.0);
	sloth_f_l_n_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.302585092994, sloth_fpop(x));
}

void test_f_exp_() {
	sloth_fpush(x, 0.0);
	sloth_f_exp_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_exp_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_E, sloth_fpop(x));

	sloth_fpush(x, -1.0);
	sloth_f_exp_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0 / M_E, sloth_fpop(x));
}

void test_f_exp_m_one_() {
	/* FEXPM1 should be accurate near zero where FEXP-1 loses bits */
	sloth_fpush(x, 0.0);
	sloth_f_exp_m_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_exp_m_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_E - 1.0, sloth_fpop(x));

	/* Near-zero value: implementation uses exp(x)-1.0 which loses a few ULPs
	   due to cancellation. Tolerance is 1e-17 to accommodate this. A port using
	   expm1() directly would be accurate to within 1 ULP (~1e-20 at this input). */
	sloth_fpush(x, 1e-10);
	sloth_f_exp_m_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(1e-17, 1e-10, sloth_fpop(x));
}

void test_f_log_ten_() {
	sloth_fpush(x, 1.0);
	sloth_f_log_ten_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 10.0);
	sloth_f_log_ten_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, 100.0);
	sloth_f_log_ten_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 2.0, sloth_fpop(x));
}

void test_f_l_n_p_one_() {
	sloth_fpush(x, 0.0);
	sloth_f_l_n_p_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, M_E - 1.0);
	sloth_f_l_n_p_one_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));
}

void test_f_a_log_() {
	/* FALOG: 10^r */
	sloth_fpush(x, 0.0);
	sloth_f_a_log_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_a_log_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 10.0, sloth_fpop(x));

	sloth_fpush(x, 2.0);
	sloth_f_a_log_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 100.0, sloth_fpop(x));
}

/* -- Trig ------------------------------------------------ */

void test_f_sine_() {
	sloth_fpush(x, 0.0);
	sloth_f_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, M_PI / 2.0);
	sloth_f_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, M_PI);
	sloth_f_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));
}

void test_f_cos_() {
	sloth_fpush(x, 0.0);
	sloth_f_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, M_PI / 2.0);
	sloth_f_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, M_PI);
	sloth_f_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -1.0, sloth_fpop(x));
}

void test_f_sine_cos_() {
	sloth_fpush(x, 0.0);
	sloth_f_sine_cos_(x);
	/* ANS: F: r -- r1(sin) r2(cos), TOS = cos */
	TEST_ASSERT_EQUAL(2, x->fp);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x)); /* cos */
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x)); /* sin */
}

void test_f_tan_() {
	sloth_fpush(x, 0.0);
	sloth_f_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, M_PI / 4.0);
	sloth_f_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));
}

void test_f_atan2_() {
	/* atan2(0, 1) = 0 */
	sloth_fpush(x, 0.0);
	sloth_fpush(x, 1.0);
	sloth_f_atan2_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	/* atan2(1, 1) = pi/4 */
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 1.0);
	sloth_f_atan2_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 4.0, sloth_fpop(x));

	/* atan2(1, 0) = pi/2 */
	sloth_fpush(x, 1.0);
	sloth_fpush(x, 0.0);
	sloth_f_atan2_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 2.0, sloth_fpop(x));
}

void test_f_a_sine_() {
	sloth_fpush(x, 0.0);
	sloth_f_a_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_a_sine_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 2.0, sloth_fpop(x));
}

void test_f_a_cos_() {
	sloth_fpush(x, 1.0);
	sloth_f_a_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 0.0);
	sloth_f_a_cos_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 2.0, sloth_fpop(x));
}

void test_f_a_tan_() {
	sloth_fpush(x, 0.0);
	sloth_f_a_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_a_tan_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, M_PI / 4.0, sloth_fpop(x));
}

/* -- Hyperbolic ------------------------------------------ */

void test_f_sin_h_() {
	sloth_fpush(x, 0.0);
	sloth_f_sin_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_sin_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, sinh(1.0), sloth_fpop(x));
}

void test_f_cos_h_() {
	sloth_fpush(x, 0.0);
	sloth_f_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, cosh(1.0), sloth_fpop(x));
}

void test_f_tan_h_() {
	sloth_fpush(x, 0.0);
	sloth_f_tan_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_tan_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, tanh(1.0), sloth_fpop(x));
}

void test_f_a_sine_h_() {
	sloth_fpush(x, 0.0);
	sloth_f_a_sine_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 1.0);
	sloth_f_a_sine_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, log(1.0 + sqrt(2.0)), sloth_fpop(x));

	/* Negative input */
	sloth_fpush(x, -1.0);
	sloth_f_a_sine_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -log(1.0 + sqrt(2.0)), sloth_fpop(x));
}

void test_f_a_cos_h_() {
	sloth_fpush(x, 1.0);
	sloth_f_a_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));

	sloth_fpush(x, 2.0);
	sloth_f_a_cos_h_(x);
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, log(2.0 + sqrt(3.0)), sloth_fpop(x));

	/* Domain error: r < 1.0 should produce NaN */
	sloth_fpush(x, 0.5);
	sloth_f_a_cos_h_(x);
	TEST_ASSERT_TRUE(isnan(sloth_fpop(x)));
}

/* -- String/numeric conversion --------------------------- */

void test_to_float_integer() {
	char *s = "42";
	sloth_push(x, (CELL)s);
	sloth_push(x, 2);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x)); /* success flag */
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 42.0, sloth_fpop(x));
}

void test_to_float_decimal() {
	char *s = "3.14";
	sloth_push(x, (CELL)s);
	sloth_push(x, 4);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 3.14, sloth_fpop(x));
}

void test_to_float_exponent() {
	char *s = "1E3";
	sloth_push(x, (CELL)s);
	sloth_push(x, 3);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1000.0, sloth_fpop(x));
}

void test_to_float_negative() {
	char *s = "-2.5";
	sloth_push(x, (CELL)s);
	sloth_push(x, 4);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, -2.5, sloth_fpop(x));
}

void test_to_float_blanks_are_zero() {
	/* A string of blanks represents 0E per ANS */
	char *s = "   ";
	sloth_push(x, (CELL)s);
	sloth_push(x, 3);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));
	TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, sloth_fpop(x));
}

void test_to_float_trailing_space_fails() {
	/* Trailing space must fail */
	char *s = "1.0 ";
	sloth_push(x, (CELL)s);
	sloth_push(x, 4);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->fp);
}

void test_to_float_invalid_fails() {
	char *s = "abc";
	sloth_push(x, (CELL)s);
	sloth_push(x, 3);
	sloth_to_float_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->fp);
}

/* -- REPRESENT ------------------------------------------- */

void test_represent_positive() {
	char buf[32] = {0};
	CELL addr = (CELL)buf;
	sloth_fpush(x, 1.5);
	sloth_push(x, addr);
	sloth_push(x, 6); /* 6 significant digits */
	sloth_represent_(x);
	/* Stack: n sign flag */
	CELL flag = sloth_pop(x);
	CELL sign = sloth_pop(x);
	CELL n    = sloth_pop(x);
	TEST_ASSERT_EQUAL(-1, flag);          /* valid number */
	TEST_ASSERT_EQUAL(0, sign);           /* positive */
	TEST_ASSERT_EQUAL(1, n);              /* decimal point after first digit: 1.5e0 -> n=1 */
	TEST_ASSERT_EQUAL('1', buf[0]);
	TEST_ASSERT_EQUAL('5', buf[1]);
}

void test_represent_negative() {
	char buf[32] = {0};
	CELL addr = (CELL)buf;
	sloth_fpush(x, -1.5);
	sloth_push(x, addr);
	sloth_push(x, 6);
	sloth_represent_(x);
	CELL flag = sloth_pop(x);
	CELL sign = sloth_pop(x);
	CELL n    = sloth_pop(x);
	TEST_ASSERT_EQUAL(-1, flag);
	TEST_ASSERT_EQUAL(-1, sign);   /* negative */
	TEST_ASSERT_EQUAL(1, n);
}

void test_represent_large_exponent() {
	char buf[32] = {0};
	CELL addr = (CELL)buf;
	sloth_fpush(x, 1000.0);
	sloth_push(x, addr);
	sloth_push(x, 4);
	sloth_represent_(x);
	CELL flag = sloth_pop(x);
	CELL sign = sloth_pop(x);
	CELL n    = sloth_pop(x);
	TEST_ASSERT_EQUAL(-1, flag);
	TEST_ASSERT_EQUAL(0, sign);
	TEST_ASSERT_EQUAL(4, n);  /* 1000 -> digits "1000", decimal after position 4 */
}

int main() {
	UNITY_BEGIN();

	/* Float stack */
	RUN_TEST(test_fpush_fpop);
	RUN_TEST(test_fpick);

	/* Float memory */
	RUN_TEST(test_fstore_ffetch);
	RUN_TEST(test_sfstore_sffetch);
	RUN_TEST(test_dfstore_dffetch);

	/* Float compilation */
	RUN_TEST(test_fcomma);

	/* (FLIT) */
	RUN_TEST(test_flit_);

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
	RUN_TEST(test_sf_fetch_store_);
	RUN_TEST(test_df_fetch_store_);

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
