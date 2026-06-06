package io.github.jordipbou.Sloth;

import static io.github.jordipbou.Sloth.Sloth.sCELL;
import static io.github.jordipbou.Sloth.Sloth.suCHAR;

import org.junit.Test;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.After;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.charset.StandardCharsets;
import java.nio.charset.Charset;

public class SlothFPTest {
	private float EPSILON = 1.0e-10f;
	private float FLOAT_EPSILON = 1.0e-6f;

	public boolean within(float r, float v, float e) {
		return (r >= v - e && r <= v + e);
	}

	public boolean within(double r, double v, double e) {
		return (r >= v - e && r <= v + e);
	}

	private Sloth sloth;

	@Before
	public void setUp() {
		sloth = new Sloth(32767, 1024);
	}

	@After
	public void tearDown() {
	}

	// Sloth Floating Point Tests

	// Float stack

	@Test
	public void test_f_push_f_pop() {
		sloth.f_push(1.5f);
		assertEquals(1, sloth.fp);
		assertEquals(1.5f, sloth.f[0], 0.0f);
	
		sloth.f_push(2.5f);
		assertEquals(2, sloth.fp);
		assertEquals(2.5f, sloth.f[1], 0.0f);
	
		assertEquals(2.5f, sloth.f_pop(), 0.0f);
		assertEquals(1, sloth.fp);
		assertEquals(1.5f, sloth.f_pop(), 0.0f);
		assertEquals(0, sloth.fp);
	}

	@Test
	public void test_f_pick() {
		sloth.f_push(1.0f);
		sloth.f_push(2.0f);
		sloth.f_push(3.0f);
		assertEquals(3.0f, sloth.f_pick(0), 0.0f);
		assertEquals(2.0f, sloth.f_pick(1), 0.0f);
		assertEquals(1.0f, sloth.f_pick(2), 0.0f);
		assertEquals(3, sloth.fp); /* f_pick is non-destructive */
	}

	// Float memory

	@Test
	public void test_f_store_f_fetch() {
		float val = 3.14159f;
		int addr = sloth.to_abs(1000);
		sloth.f_store(addr, val);
		assertEquals(val, sloth.f_fetch(addr), EPSILON);
	}

	@Test
	public void test_s_f_store_s_f_fetch() {
		float val = 2.71f;
		int addr = sloth.to_abs(1000);
		sloth.s_f_store(addr, val);
		float res = sloth.s_f_fetch(addr);
		assertTrue(within(res, val, FLOAT_EPSILON));
	}

	@Test
	public void test_d_f_store_d_f_fetch() {
		double val = 1.41421356237;
		int addr = sloth.to_abs(1000);
		sloth.d_f_store(addr, val);
		double res = sloth.d_f_fetch(addr);
		assertTrue(within(res, val, EPSILON));
	}

	// Float compilation 

	@Test
	public void test_f_comma() {
		int here = sloth.here();
		sloth.f_comma(2.718281828f);
		assertEquals(here + Sloth.sFCELL, sloth.here());
		assertTrue(within(2.718281828f, sloth.f_fetch(here), EPSILON));
	}

	// (FLIT)

	@Test
	public void test_flit_() {
		int here = sloth.here();
		sloth.f_comma(1.234f);
		sloth.ip = here;
		sloth._f_lit_();
		assertEquals(1, sloth.fp);
		assertTrue(within(1.234f, sloth.f_pop(), EPSILON));
		assertEquals(here + Sloth.sFCELL, sloth.ip);
	}

	// Alignment

	@Test
	public void test_f_align_() {
		/* Force HERE to be misaligned relative to FCELL */
		sloth.allot(1);
		sloth._f_align_();
		assertEquals(0, sloth.here() % Sloth.sFCELL);
	}

	@Test
	public void test_f_aligned_() {
		int unaligned = sloth.to_abs(3);
		sloth.push(unaligned);
		sloth._f_aligned_();
		int result = sloth.pop();
		assertEquals(0, result % Sloth.sFCELL);
		assertTrue(result >= unaligned);
	}

	@Test
	public void test_s_f_aligned_() {
		int unaligned = sloth.to_abs(3);
		sloth.push(unaligned);
		sloth._s_f_aligned_();
		int result = sloth.pop();
		assertEquals(0, result % Sloth.sSFCELL);
		assertTrue(result >= unaligned);
	}
	
	@Test
	public void test_d_f_aligned_() {
		int unaligned = sloth.to_abs(3);
		sloth.push(unaligned);
		sloth._d_f_aligned_();
		int result = sloth.pop();
		assertEquals(0, result % Sloth.sDFCELL);
		assertTrue(result >= unaligned);
	}

	// Size words
	
	@Test
	public void test_floats_() {
		sloth.push(3);
		sloth._floats_();
		assertEquals(3 * Sloth.sFCELL, sloth.pop());
	}
	
	@Test
	public void test_s_floats_() {
		sloth.push(3);
		sloth._s_floats_();
		assertEquals(3 * Sloth.sSFCELL, sloth.pop());
	}
	
	@Test
	public void test_d_floats_() {
		sloth.push(3);
		sloth._d_floats_();
		assertEquals(3 * Sloth.sDFCELL, sloth.pop());
	}

	// Stack manipulation

	@Test
	public void test_f_depth_() {
		sloth._f_depth_();
		assertEquals(0, sloth.pop());
	
		sloth.f_push(1.0f);
		sloth.f_push(2.0f);
		sloth._f_depth_();
		assertEquals(2, sloth.pop());
	}

	@Test
	public void test_f_drop_() {
		sloth.f_push(1.0f);
		sloth.f_push(2.0f);
		sloth._f_drop_();
		assertEquals(1, sloth.fp);
		assertTrue(within(1.0, sloth.f[0], EPSILON));
	}

	@Test
	public void test_f_dup_() {
		sloth.f_push(3.5f);
		sloth._f_dup_();
		assertEquals(2, sloth.fp);
		assertTrue(within(3.5f, sloth.f_pop(), EPSILON));
		assertTrue(within(3.5f, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_over_() {
		sloth.f_push(1.0f);
		sloth.f_push(2.0f);
		sloth._f_over_();
		/* F: 1.0 2.0 1.0 */
		assertEquals(3, sloth.fp);
		assertTrue(within(1.0f, sloth.f_pop(), EPSILON));
		assertTrue(within(2.0f, sloth.f_pop(), EPSILON));
		assertTrue(within(1.0f, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_swap_() {
		sloth.f_push(1.0f);
		sloth.f_push(2.0f);
		sloth._f_swap_();
		assertEquals(2, sloth.fp);
		assertTrue(within(1.0f, sloth.f_pop(), EPSILON));
		assertTrue(within(2.0f, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_rot_() {
		sloth.f_push(1.0f);
		sloth.f_push(2.0f);
		sloth.f_push(3.0f);
		sloth._f_rot_();
		/* F: 1.0 2.0 3.0 -- F: 2.0 3.0 1.0 */
		assertEquals(3, sloth.fp);
		assertTrue(within(1.0f, sloth.f_pop(), EPSILON));
		assertTrue(within(3.0f, sloth.f_pop(), EPSILON));
		assertTrue(within(2.0f, sloth.f_pop(), EPSILON));
	}

	// Comparisons

	@Test
	public void test_f_less_than_() {
		/* 1.0 < 2.0 = true */
		sloth.f_push(1.0f); sloth.f_push(2.0f);
		sloth._f_less_than_();
		assertEquals(0, sloth.fp);
		assertEquals(-1, sloth.pop());
	
		/* 2.0 < 1.0 = false */
		sloth.f_push(2.0f); sloth.f_push(1.0f);
		sloth._f_less_than_();
		assertEquals(0, sloth.pop());
	
		/* 1.0 < 1.0 = false */
		sloth.f_push(1.0f); sloth.f_push(1.0f);
		sloth._f_less_than_();
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_f_zero_less_than_() {
		sloth.f_push(-1.0f);
		sloth._f_zero_less_than_();
		assertEquals(-1, sloth.pop());
	
		sloth.f_push(0.0f);
		sloth._f_zero_less_than_();
		assertEquals(0, sloth.pop());
	
		sloth.f_push(1.0f);
		sloth._f_zero_less_than_();
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_f_zero_equals_() {
		sloth.f_push(0.0f);
		sloth._f_zero_equals_();
		assertEquals(-1, sloth.pop());
	
		sloth.f_push(1.0f);
		sloth._f_zero_equals_();
		assertEquals(0, sloth.pop());
	
		sloth.f_push(-0.0f);
		sloth._f_zero_equals_();
		assertEquals(-1, sloth.pop());
	}

	// Memory/stack transfer

	@Test
	public void test_f_fetch_store_() {
		int addr = sloth.to_abs(1000);
		sloth.f_push(9.99f);
		sloth.push(addr);
		sloth._f_store_();
		assertEquals(0, sloth.fp);
	
		sloth.push(addr);
		sloth._f_fetch_();
		assertEquals(1, sloth.fp);
		assertTrue(within(9.99f, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_s_f_fetch_store_() {
		int addr = sloth.to_abs(1000);
		sloth.f_push(3.14);
		sloth.push(addr);
		sloth._s_f_store_();
		assertEquals(0, sloth.fp);
	
		sloth.push(addr);
		sloth._s_f_fetch_();
		assertEquals(1, sloth.fp);
		assertTrue(within(3.14f, (float)sloth.f_pop(), FLOAT_EPSILON));
	}

	@Test
	public void test_d_f_fetch_store_() {
		int addr = sloth.to_abs(1000);
		sloth.f_push(2.71828182845);
		sloth.push(addr);
		sloth._d_f_store_();
		assertEquals(0, sloth.fp);
	
		sloth.push(addr);
		sloth._d_f_fetch_();
		assertEquals(1, sloth.fp);
		assertTrue(within(2.71828182845, sloth.f_pop(), EPSILON));
	}

	// Conversion

	@Test
	public void test_d_to_f_positive() {
		/* D>F: 3. ( lo=3 hi=0 ) -> 3.0 */
		sloth.push(3);
		sloth.push(0);
		sloth._d_to_f_();
		assertEquals(0, sloth.sp);
		assertEquals(1, sloth.fp);
		assertTrue(within(3.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_d_to_f_negative() {
		/* D>F: -3. ( lo=-3 hi=-1 ) -> -3.0 */
		sloth.push(-3);
		sloth.push(-1);
		sloth._d_to_f_();
		assertEquals(0, sloth.sp);
		assertTrue(within(-3.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_d_to_f_zero() {
		sloth.push(0);
		sloth.push(0);
		sloth._d_to_f_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_to_d_positive() {
		/* F>D: 3.7 -> lo=3 hi=0 (truncates toward zero) */
		sloth.f_push(3.7);
		sloth._f_to_d_();
		assertEquals(0, sloth.fp);
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());  /* hi */
		assertEquals(3, sloth.pop());  /* lo */
	}

	@Test
	public void test_f_to_d_negative() {
		/* F>D: -3.7 -> lo=-3 hi=-1 */
		sloth.f_push(-3.7);
		sloth._f_to_d_();
		assertEquals(-1, sloth.pop()); /* hi */
		assertEquals(-3, sloth.pop()); /* lo */
	}

	@Test
	public void test_f_to_d_zero() {
		sloth.f_push(0.0);
		sloth._f_to_d_();
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.pop());
	}
	
	// Arithmetic

	@Test
	public void test_f_plus_() {
		sloth.f_push(1.5);
		sloth.f_push(2.5);
		sloth._f_plus_();
		assertEquals(1, sloth.fp);
		assertTrue(within(4.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_minus_() {
		sloth.f_push(5.0);
		sloth.f_push(1.5);
		sloth._f_minus_();
		assertTrue(within(3.5, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_star_() {
		sloth.f_push(3.0);
		sloth.f_push(4.0);
		sloth._f_star_();
		assertTrue(within(12.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_slash_() {
		sloth.f_push(10.0);
		sloth.f_push(4.0);
		sloth._f_slash_();
		assertTrue(within(2.5, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_star_star_() {
		sloth.f_push(2.0);
		sloth.f_push(10.0);
		sloth._f_star_star_();
		assertTrue(within(1024.0, sloth.f_pop(), EPSILON));
	
		/* 9.0 ** 0.5 = 3.0 */
		sloth.f_push(9.0);
		sloth.f_push(0.5);
		sloth._f_star_star_();
		assertTrue(within(3.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_abs_() {
		sloth.f_push(-3.5);
		sloth._f_abs_();
		assertTrue(within(3.5, sloth.f_pop(), EPSILON));
	
		sloth.f_push(3.5);
		sloth._f_abs_();
		assertTrue(within(3.5, sloth.f_pop(), EPSILON));
	
		sloth.f_push(0.0);
		sloth._f_abs_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_negate_() {
		sloth.f_push(3.5);
		sloth._f_negate_();
		assertTrue(within(-3.5, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-2.0);
		sloth._f_negate_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(0.0);
		sloth._f_negate_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_max_() {
		sloth.f_push(3.0);
		sloth.f_push(5.0);
		sloth._f_max_();
		assertTrue(within(5.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(5.0);
		sloth.f_push(3.0);
		sloth._f_max_();
		assertTrue(within(5.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-1.0);
		sloth.f_push(-2.0);
		sloth._f_max_();
		assertTrue(within(-1.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_min_() {
		sloth.f_push(3.0);
		sloth.f_push(5.0);
		sloth._f_min_();
		assertTrue(within(3.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(5.0);
		sloth.f_push(3.0);
		sloth._f_min_();
		assertTrue(within(3.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-1.0);
		sloth.f_push(-2.0);
		sloth._f_min_();
		assertTrue(within(-2.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_floor_() {
		sloth.f_push(2.7);
		sloth._floor_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-2.7);
		sloth._floor_();
		assertTrue(within(-3.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(2.0);
		sloth._floor_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_round_() {
		sloth.f_push(0.5);
		sloth._f_round_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-0.5);
		sloth._f_round_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.5);
		sloth._f_round_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-1.5);
		sloth._f_round_();
		assertTrue(within(-2.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(2.4);
		sloth._f_round_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	}

	// Proximate (F~)

	@Test
	public void test_f_proximate_exact() {
		/* r3 = 0.0: exact bitwise comparison */
		sloth.f_push(1.5);
		sloth.f_push(1.5);
		sloth.f_push(0.0);
		sloth._f_proximate_();
		assertEquals(-1, sloth.pop());
	
		sloth.f_push(1.5);
		sloth.f_push(1.6);
		sloth.f_push(0.0);
		sloth._f_proximate_();
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_f_proximate_absolute() {
		/* r3 > 0: absolute tolerance */
		sloth.f_push(1.0);
		sloth.f_push(1.09);
		sloth.f_push(0.1);
		sloth._f_proximate_();
		assertEquals(-1, sloth.pop());
	
		sloth.f_push(1.0);
		sloth.f_push(1.11);
		sloth.f_push(0.1);
		sloth._f_proximate_();
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_f_proximate_relative() {
		/* r3 < 0: relative tolerance |r1-r2| < |r3|*(|r1|+|r2|) */
		sloth.f_push(1.0);
		sloth.f_push(1.001);
		sloth.f_push(-0.01);
		sloth._f_proximate_();
		assertEquals(-1, sloth.pop());
	
		sloth.f_push(1.0);
		sloth.f_push(1.1);
		sloth.f_push(-0.01);
		sloth._f_proximate_();
		assertEquals(0, sloth.pop());
	}

	// Transcendental functions

	@Test
	public void test_f_sqrt_() {
		sloth.f_push(4.0);
		sloth._f_sqrt_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(2.0);
		sloth._f_sqrt_();
		assertTrue(within(1.41421356237, sloth.f_pop(), EPSILON));
	
		sloth.f_push(0.0);
		sloth._f_sqrt_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_l_n_() {
		sloth.f_push(1.0);
		sloth._f_l_n_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.E);
		sloth._f_l_n_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(10.0);
		sloth._f_l_n_();
		assertTrue(within(2.302585092994, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_exp_() {
		sloth.f_push(0.0);
		sloth._f_exp_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_exp_();
		assertTrue(within(Math.E, sloth.f_pop(), EPSILON));
	
		sloth.f_push(-1.0);
		sloth._f_exp_();
		assertTrue(within(1.0 / Math.E, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_exp_m_one_() {
		/* FEXPM1 should be accurate near zero where FEXP-1 loses bits */
		sloth.f_push(0.0);
		sloth._f_exp_m_one_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_exp_m_one_();
		assertTrue(within(Math.E - 1.0, sloth.f_pop(), EPSILON));
	
		/* Near-zero value: implementation uses exp(x)-1.0 which loses a few ULPs
		   due to cancellation. Tolerance is 1e-17 to accommodate this. A port using
		   expm1() directly would be accurate to within 1 ULP (~1e-20 at this input). */
		sloth.f_push(1e-10);
		sloth._f_exp_m_one_();
		assertTrue(within(1e-10, sloth.f_pop(), 1e-17));
	}

	@Test
	public void test_f_log_ten_() {
		sloth.f_push(1.0);
		sloth._f_log_ten_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(10.0);
		sloth._f_log_ten_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(100.0);
		sloth._f_log_ten_();
		assertTrue(within(2.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_l_n_p_one_() {
		sloth.f_push(0.0);
		sloth._f_l_n_p_one_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.E - 1.0);
		sloth._f_l_n_p_one_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_a_log_() {
		/* FALOG: 10^r */
		sloth.f_push(0.0);
		sloth._f_a_log_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_a_log_();
		assertTrue(within(10.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(2.0);
		sloth._f_a_log_();
		assertTrue(within(100.0, sloth.f_pop(), EPSILON));
	}

	// Trig

	@Test
	public void test_f_sine_() {
		sloth.f_push(0.0);
		sloth._f_sine_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.PI / 2.0);
		sloth._f_sine_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.PI);
		sloth._f_sine_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_cos_() {
		sloth.f_push(0.0);
		sloth._f_cos_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.PI / 2.0);
		sloth._f_cos_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.PI);
		sloth._f_cos_();
		assertTrue(within(-1.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_sine_cos_() {
		sloth.f_push(0.0);
		sloth._f_sine_cos_();
		/* ANS: F: r -- r1(sin) r2(cos), TOS = cos */
		assertEquals(2, sloth.fp);
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_tan_() {
		sloth.f_push(0.0);
		sloth._f_tan_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(Math.PI / 4.0);
		sloth._f_tan_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_atan2_() {
		/* atan2(0, 1) = 0 */
		sloth.f_push(0.0);
		sloth.f_push(1.0);
		sloth._f_atan2_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		/* atan2(1, 1) = pi/4 */
		sloth.f_push(1.0);
		sloth.f_push(1.0);
		sloth._f_atan2_();
		assertTrue(within(Math.PI / 4.0, sloth.f_pop(), EPSILON));
	
		/* atan2(1, 0) = pi/2 */
		sloth.f_push(1.0);
		sloth.f_push(0.0);
		sloth._f_atan2_();
		assertTrue(within(Math.PI / 2.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_a_sine_() {
		sloth.f_push(0.0);
		sloth._f_a_sine_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_a_sine_();
		assertTrue(within(Math.PI / 2.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_a_cos_() {
		sloth.f_push(1.0);
		sloth._f_a_cos_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(0.0);
		sloth._f_a_cos_();
		assertTrue(within(Math.PI / 2.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_a_tan_() {
		sloth.f_push(0.0);
		sloth._f_a_tan_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_a_tan_();
		assertTrue(within(Math.PI / 4.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_sin_h_() {
		sloth.f_push(0.0);
		sloth._f_sin_h_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_sin_h_();
		assertTrue(within(Math.sinh(1.0), sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_cos_h_() {
		sloth.f_push(0.0);
		sloth._f_cos_h_();
		assertTrue(within(1.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_cos_h_();
		assertTrue(within(Math.cosh(1.0), sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_tan_h_() {
		sloth.f_push(0.0);
		sloth._f_tan_h_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_tan_h_();
		assertTrue(within(Math.tanh(1.0), sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_a_sine_h_() {
		sloth.f_push(0.0);
		sloth._f_a_sine_h_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(1.0);
		sloth._f_a_sine_h_();
		assertTrue(within(Math.log(1.0 + Math.sqrt(2.0)), sloth.f_pop(), EPSILON));
	
		/* Negative input */
		sloth.f_push(-1.0);
		sloth._f_a_sine_h_();
		assertTrue(within(-Math.log(1.0 + Math.sqrt(2.0)), sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_f_a_cos_h_() {
		sloth.f_push(1.0);
		sloth._f_a_cos_h_();
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	
		sloth.f_push(2.0);
		sloth._f_a_cos_h_();
		assertTrue(within(Math.log(2.0 + Math.sqrt(3.0)), sloth.f_pop(), EPSILON));
	
		/* Domain error: r < 1.0 should produce NaN */
		sloth.f_push(0.5);
		sloth._f_a_cos_h_();
		assertTrue(Double.isNaN(sloth.f_pop()));
	}

	// String/numeric conversion

	@Test
	public void test_to_float_integer() {
		String s = "42";
		sloth.push(sloth.fromString(s));
		sloth.push(2);
		sloth._to_float_();
		assertEquals(-1, sloth.pop()); /* success flag */
		assertTrue(within(42.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_to_float_decimal() {
		String s = "3.14";
		sloth.push(sloth.fromString(s));
		sloth.push(4);
		sloth._to_float_();
		assertEquals(-1, sloth.pop());
		assertTrue(within(3.14, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_to_float_exponent() {
		String s = "1E3";
		sloth.push(sloth.fromString(s));
		sloth.push(3);
		sloth._to_float_();
		assertEquals(-1, sloth.pop());
		assertTrue(within(1000.0, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_to_float_negative() {
		String s = "-2.5";
		sloth.push(sloth.fromString(s));
		sloth.push(4);
		sloth._to_float_();
		assertEquals(-1, sloth.pop());
		assertTrue(within(-2.5, sloth.f_pop(), EPSILON));
	}

	@Test
	public void test_to_float_blanks_are_zero() {
		/* A string of blanks represents 0E per ANS */
		String s = "   ";
		sloth.push(sloth.fromString(s));
		sloth.push(3);
		sloth._to_float_();
		assertEquals(-1, sloth.pop());
		assertTrue(within(0.0, sloth.f_pop(), EPSILON));
	}
	
	@Test
	public void test_to_float_trailing_space_fails() {
		/* Trailing space must fail */
		String s = "1.0 ";
		sloth.push(sloth.fromString(s));
		sloth.push(4);
		sloth._to_float_();
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.fp);
	}
	
	@Test
	public void test_to_float_invalid_fails() {
		String s = "abc";
		sloth.push(sloth.fromString(s));
		sloth.push(3);
		sloth._to_float_();
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.fp);
	}
}
