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
}
