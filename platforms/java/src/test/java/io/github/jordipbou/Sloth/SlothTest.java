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

public class SlothTest {
	private Sloth sloth;
	private int debugCount = 0;

	@Before
	public void setUp() {
		sloth = new Sloth(32767, 1024);
	}

	@After
	public void tearDown() {
	}

	// Sloth VM Tests

	@Test
	public void testContextInit() {
		assertNotNull(sloth);
		assertEquals(0, sloth.sp);
		assertEquals(0, sloth.rp);
		assertEquals(-1, sloth.ip);
		assertEquals(0, sloth.d);
		assertEquals(1, sloth.u);
		assertEquals(2, sloth.ts);
		assertEquals(3, sloth.op);
		assertEquals(32767, ((ByteBuffer)(sloth.o.get(sloth.d))).capacity());
		assertEquals(1024, ((ByteBuffer)(sloth.o.get(sloth.u))).capacity());
		assertEquals(2048, ((ByteBuffer)(sloth.o.get(sloth.ts))).capacity());
		assertEquals(3*sCELL, ((ByteBuffer)(sloth.o.get(0))).getInt(0*sCELL));
		assertEquals(0, ((ByteBuffer)(sloth.o.get(0))).getInt(1*sCELL));
		assertEquals(0, ((ByteBuffer)(sloth.o.get(0))).getInt(2*sCELL));
		assertEquals(0, ((ByteBuffer)(sloth.o.get(0))).getInt(3*sCELL));
		assertEquals(2*sCELL, ((ByteBuffer)(sloth.o.get(1))).getInt(0*sCELL));
	}

	/* -- Data and return stack ---------------------------- */

	@Test
	public void testDataStack() {
		sloth.push(42);
		assertEquals(1, sloth.sp);
		assertEquals(42, sloth.pick(0));

		sloth.push(100);
		assertEquals(2, sloth.sp);
		assertEquals(100, sloth.pick(0));
		assertEquals(42, sloth.pick(1));

		sloth._dup_();
		assertEquals(3, sloth.sp);
		assertEquals(100, sloth.pick(0));
		assertEquals(100, sloth.pick(1));
		assertEquals(42, sloth.pick(2));

		sloth._drop_();
		assertEquals(2, sloth.sp);

		sloth._swap_();
		assertEquals(2, sloth.sp);
		assertEquals(42, sloth.pop());
		assertEquals(1, sloth.sp);
		assertEquals(100, sloth.pop());
		assertEquals(0, sloth.sp);
	}

	@Test
	public void testOver() {
		sloth.push(42);
		sloth.push(100);
		sloth._over_();
		assertEquals(3, sloth.sp);
		assertEquals(42, sloth.pop());
		assertEquals(100, sloth.pop());
		assertEquals(42, sloth.pop());
	}

	@Test
	public void returnStack() {
		sloth.rpush(42);
		assertEquals(1, sloth.rp);
		assertEquals(42, sloth.rpick(0));

		sloth.rpush(100);
		assertEquals(2, sloth.rp);
		assertEquals(100, sloth.rpick(0));
		assertEquals(42, sloth.rpick(1));

		sloth._r_from_();
		assertEquals(1, sloth.rp);
		assertEquals(42, sloth.rpick(0));
		assertEquals(1, sloth.sp);
		assertEquals(100, sloth.pick(0));

		sloth._to_r_();
		assertEquals(2, sloth.rp);
		assertEquals(100, sloth.rpick(0));
		assertEquals(42, sloth.rpick(1));

		assertEquals(100, sloth.rpop());
		assertEquals(1, sloth.rp);
		assertEquals(42, sloth.rpop());
		assertEquals(0, sloth.rp);
	}

	/* -- Memory management -------------------------------- */

	@Test
	public void test_memory() {
		assertEquals(0, sloth.to_abs(0, 0));
		assertEquals(0, sloth.to_rel(0));

		sloth.b_store(sloth.to_abs(1000, 0), (byte)-33);
		assertEquals(-33, sloth.b_fetch(sloth.to_abs(1000, 0)));

		sloth.c_store(sloth.to_abs(1000, 0), 'a');
		assertEquals('a', sloth.c_fetch(sloth.to_abs(1000, 0)));

		sloth.store(sloth.to_abs(1000, 0), 123);
		assertEquals(123, sloth.fetch(sloth.to_abs(1000, 0)));

		sloth.set(1000, 13);
		assertEquals(13, sloth.get(1000));

		sloth.user_set(1000, 111);
		assertEquals(111, sloth.user_get(1000));
		// Ensure writing in user area does not overwrite dictionary
		assertEquals(13, sloth.get(1000)); 

		assertEquals(3*sCELL, sloth.here());
		sloth.allot(sCELL);
		assertEquals(4*sCELL, sloth.here());

		sloth.allot(suCHAR);
		assertEquals(5*sCELL, sloth.aligned(sloth.here()));
		sloth._align_();
		assertEquals(5*sCELL, sloth.here());

		sloth.push(99);
		sloth.push(sloth.to_abs(1000, 0));
		sloth._store_();
		sloth.push(sloth.to_abs(1000, 0));
		sloth._fetch_();
		assertEquals(99, sloth.pop());

		sloth.push(127);
		sloth.push(sloth.to_abs(1000, 0));
		sloth._c_store_();
		sloth.push(sloth.to_abs(1000, 0));
		sloth._c_fetch_();
		assertEquals(127, sloth.pop());

		sloth.push(1);
		sloth._cells_();
		assertEquals(sCELL, sloth.pop());

		sloth.push(1);
		sloth._chars_();
		assertEquals(suCHAR, sloth.pop());

		sloth._unused_();
		assertEquals(1, sloth.sp);
		assertEquals(((ByteBuffer)(sloth.o.get(0))).capacity() - sloth.here(), sloth.pop());
	}

	@Test
	public void test_allot_() {
		int here = sloth.here();
		sloth.push(10);
		sloth._allot_();
		assertEquals(0, sloth.sp);
		assertEquals(here + 10, sloth.here());
	}

	@Test
	public void test_here_() {
		sloth._here_();
		assertEquals(sloth.here(), sloth.pop());
		sloth._here_();
		assertEquals(sloth.fetch(sloth.to_abs(0)), sloth.pop());
	}

	/* -- Compilation -------------------------------------- */

	@Test
	public void test_compilation() {
		assertEquals(3*sCELL, sloth.here());
		sloth.comma(13);
		assertEquals(4*sCELL, sloth.here());
		assertEquals(13, sloth.fetch(3*sCELL));

		sloth.c_comma((char)17);
		assertEquals(4*sCELL + suCHAR, sloth.here());
		assertEquals(17, sloth.c_fetch(4*sCELL));
	}

	/* -- Headers ------------------------------------------ */

	@Test
	public void test_get_set_latest() {
		assertEquals(0, sloth.get_latest());
		sloth.set_latest(13);
		assertEquals(13, sloth.get_latest());
	}

	@Test
	public void test_get_link() {
		sloth.set(100, 13);
		assertEquals(13, sloth.get_link(sloth.to_abs(100)));
	}

	@Test
	public void test_get_set_xt() {
		sloth.set_xt(sloth.to_abs(100), 17);
		assertEquals(17, sloth.get_xt(sloth.to_abs(100)));
	}

	@Test
	public void test_get_set_flags() {
		sloth.set_flags(sloth.to_abs(100), 33);
		assertEquals(33, sloth.get_flags(sloth.to_abs(100)));
	}

	@Test
	public void test_has_set_unset_flag() {
		sloth.set_flags(sloth.to_abs(100), 0);
		assertFalse(sloth.has_flag(sloth.to_abs(100), 2));
		sloth.set_flag(sloth.to_abs(100), 2);
		assertTrue(sloth.has_flag(sloth.to_abs(100), 2));
		sloth.unset_flag(sloth.to_abs(100), 2);
		assertFalse(sloth.has_flag(sloth.to_abs(100), 2));
	}

	@Test
	public void test_headers() {
		String name = "NEW-WORD";
		int name_addr = sloth.fromString(name);
		int w = sloth.header(name_addr, name.length());
		assertEquals(sloth.aligned(w + 2*sCELL + 2*suCHAR + name.length()*suCHAR), sloth.here());
		assertEquals(sloth.aligned(sloth.here()), sloth.here());
		assertEquals(3*sCELL, sloth.get_latest());
		assertEquals(sloth.here(), sloth.fetch(4*sCELL));
		assertEquals(0, sloth.c_fetch(5*sCELL));
		assertEquals(name, sloth.toString(5*sCELL + 2*suCHAR, sloth.c_fetch(5*sCELL + suCHAR)));
	}

	@Test
	public void test_name_and_len() {
		String name = "TEST-WORD";
		int name_addr = sloth.fromString(name);
		int w = sloth.header(name_addr, name.length());
		assertEquals(name.length(), sloth.get_namelen(w));
		assertEquals(name, sloth.toString(sloth.get_name_addr(w), sloth.get_namelen(w)));
	}

	/* -- Primitive and word creation ---------------------- */
	
	int my_var = 11;
	void my_primitive(Sloth vm) { my_var = 17; }

	@Test
	public void test_primitive_and_word_creation() {
		assertEquals(0, sloth.p.size());
		assertEquals(-1, sloth.primitive((vm) -> my_primitive(vm)));
		assertEquals(1, sloth.p.size());

		int word = sloth.here();
		sloth.code("MY-PRIMITIVE", -1);
		assertEquals(-1, sloth.fetch(4*sCELL));
		assertEquals("MY-PRIMITIVE", sloth.toString(sloth.get_name_addr(word), sloth.get_namelen(word)));
	}

	/* -- Inner interpreter -------------------------------- */
	
	@Test
	public void test_op() {
		sloth.set(3*sCELL, 11);
		sloth.ip = sloth.to_abs(3*sCELL);
		assertEquals(11, sloth.op());
		assertEquals(sloth.to_abs(4*sCELL), sloth.ip);
	}

	@Test
	public void test_do_prim() {
		assertEquals(11, my_var);
		sloth.primitive((vm) -> my_primitive(vm));
		sloth.do_prim(-1);
		assertEquals(17, my_var);
	}

	@Test
	public void test_call() {
		sloth.ip = -1;
		sloth.call(17);
		assertEquals(17, sloth.ip);
		assertEquals(0, sloth.rp);
	
		/* This test ensures that if there's something in the */
		/* return stack (like storing input source information) */
		/* a call pushes the previous IP (in this case -1) to the */
		/* return stack to not EXIT to a non IP value at the end */
		/* of the called word. */
		sloth.rpush(13);
		sloth.ip = -1;
		sloth.call(17);
		assertEquals(17, sloth.ip);
		assertEquals(2, sloth.rp);
		assertEquals(-1, sloth.r[1]);
		assertEquals(13, sloth.r[0]);
		sloth.rpop();
		sloth.rpop();
	
		sloth.ip = 11;
		sloth.call(19);
		assertEquals(19, sloth.ip);
		assertEquals(1, sloth.rp);
		assertEquals(11, sloth.r[0]);
		sloth.rpop();
		
		sloth.call(-1);
		assertEquals(-1, sloth.ip);
	}

	void test_execute() {
		sloth.primitive((vm) -> my_primitive(vm));

		my_var = 11;
		sloth.execute(-1);
		assertEquals(17, my_var);

		sloth.ip = -1;
		sloth.execute(sloth.to_abs(3*sCELL));
		assertEquals(0, sloth.rp);
		assertEquals(sloth.to_abs(3*sCELL), sloth.ip);
	}

	void my_exit(Sloth vm) { sloth.ip=  -1; }

	@Test
	public void test_inner() {
		sloth.set(3*sCELL, sloth.primitive((vm) -> my_primitive(vm)));
		sloth.set(4*sCELL, sloth.primitive((vm) -> my_exit(vm)));
		sloth.ip = sloth.to_abs(3*sCELL);
		my_var = 11;
		sloth.inner();
		assertEquals(17, my_var);
		assertEquals(-1, sloth.ip);
	}

	@Test
	public void test_eval() {
		sloth.set(3*sCELL, sloth.primitive((vm) -> my_primitive(vm)));
		sloth.set(4*sCELL, sloth.primitive((vm) -> my_exit(vm)));

		sloth.ip = sloth.to_abs(3*sCELL);
		my_var = 11;
		sloth.eval(sloth.ip);
		assertEquals(17, my_var);
		assertEquals(-1, sloth.ip);

		my_var = 11;
		sloth.ip = -1;
		sloth.eval(sloth.ip);
		assertEquals(17, my_var);
	}

	void my_debug(Sloth vm) { my_var = 23; }

	@Test
	public void test_debug_inner() {
		sloth.set(3*sCELL, sloth.primitive((vm) -> my_primitive(vm)));
		sloth.set(4*sCELL, sloth.primitive((vm) -> my_exit(vm)));
		sloth.primitive((vm) -> my_debug(vm));
		my_var = 11;
		sloth.ip = sloth.to_abs(4*sCELL);
		sloth.debug_inner(-3);
		assertEquals(23, my_var);
		assertEquals(1, sloth.sp);
		assertEquals(sloth.to_abs(4*sCELL), sloth.s[0]);
	}

	int pre_var = 11;
	void pre_xt(Sloth vm) { pre_var = 13; }
	int inner_var = 17;
	void inner_xt(Sloth vm) { inner_var = 19; }
	int post_var = 23;
	void post_xt(Sloth vm) { post_var = 27; }

	@Test
	public void test_debug_() {
		sloth.primitive((vm) -> my_exit(vm));
		sloth.primitive((vm) -> pre_xt(vm));
		sloth.primitive((vm) -> inner_xt(vm));
		sloth.primitive((vm) -> post_xt(vm));

		sloth.push(-1);
		sloth.push(-2);
		sloth.push(-3);
		sloth.push(-4);
		
		sloth._debug_();		

		assertEquals(13, pre_var);
		assertEquals(17, inner_var);
		assertEquals(27, post_var);

		sloth.set(3*sCELL, -1);

		pre_var = 11;
		inner_var = 17;
		post_var = 23;

		sloth.push(sloth.to_abs(3*sCELL));
		sloth.push(-2);
		sloth.push(-3);
		sloth.push(-4);
		
		sloth._debug_();

		assertEquals(13, pre_var);
		assertEquals(19, inner_var);
		assertEquals(27, post_var);
	}

	/* -- Exceptions --------------------------------------- */
	
	void throw_42_prim(Sloth vm) { sloth._throw(42); }

	@Test
	public void test_catch_no_throw() {
		int p = sloth.primitive((vm) -> my_primitive(vm));
		sloth._catch(p);
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_catch_throw() {
		int p = sloth.primitive((vm) -> throw_42_prim(vm));
		sloth._catch(p);
		assertEquals(1, sloth.sp);
		assertEquals(42, sloth.pop());
	}

	@Test
	public void test_catch_restores_stack() {
		sloth.push(1);
		sloth.push(2);
		int p = sloth.primitive((vm) -> throw_42_prim(vm));
		sloth._catch(p);
		assertEquals(3, sloth.sp);
		assertEquals(42, sloth.pop());
		assertEquals(2, sloth.pop());
		assertEquals(1, sloth.pop());
	}

	void nested_throw_test_prim(Sloth vm) {
		int p = sloth.primitive((_vm) -> throw_42_prim(_vm));
		sloth._catch(p);
	}

	@Test
	public void test_nested_catch() {
		int p = sloth.primitive((vm) -> nested_throw_test_prim(vm));
		sloth._catch(p);
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(42, sloth.pop());
	}

	@Test
	public void test_catch_throw_prim() {
		int p = sloth.primitive((vm) -> throw_42_prim(vm));
		sloth.push(p);
		sloth._catch_();
		assertEquals(42, sloth.pop());

		sloth.push(0); // No error
		sloth._throw_();
		assertEquals(0, sloth.sp);
	}

	/* -- Inner interpreter primitives -------------------- */

	@Test
	public void test_exit_() {
		// Test rp > 0
		sloth.rpush(100);
		sloth._exit_();
		assertEquals(100, sloth.ip);
		assertEquals(0, sloth.rp);

		// Test rp == 0
		sloth._exit_();
		assertEquals(-1, sloth.ip);
	}

	@Test
	public void test_lit_() {
		sloth.set(3*sCELL, 42);
		sloth.ip = sloth.to_abs(3*sCELL);
		sloth._lit_();
		assertEquals(42, sloth.pop());
		assertEquals(sloth.to_abs(4*sCELL), sloth.ip);
	}

	@Test
	public void test_rip_() {
		int base_ip = sloth.to_abs(3*sCELL);
		sloth.set(3*sCELL, 10*sCELL); // offset
		sloth.ip = base_ip;
		sloth._rip_();
		assertEquals(base_ip + 9*sCELL, sloth.pop());
		assertEquals(base_ip + sCELL, sloth.ip);
	}

	@Test
	public void test_branch_() {
		int base_ip = sloth.to_abs(3*sCELL);
		sloth.set(3*sCELL, 10*sCELL); // offset
		sloth.ip = base_ip;
		sloth._branch_();
		assertEquals(base_ip + 10*sCELL, sloth.ip);
	}

	@Test
	public void test_zbranch_() {
		int base_ip = sloth.to_abs(3*sCELL);
		sloth.set(3*sCELL, 10*sCELL);

		// Case 0: TOS is 0 (should branch)
		sloth.ip = base_ip;
		sloth.push(0);
		sloth._zbranch_();
		assertEquals(base_ip + 10*sCELL, sloth.ip);

		// Case 1: TOS is NOT 0 (should NOT branch)
		sloth.ip = base_ip;
		sloth.push(1);
		sloth._zbranch_();
		assertEquals(base_ip + sCELL, sloth.ip);
	}

	/* -- Arithmetic and logical operations ---------------- */
	
	/* These words implement ANS Forth tests adapted to C */
	
	int MAXUINT = ~0;
	int MAXINT = MAXUINT>>>1;
	int MININT = ~MAXINT;
	int MIDUINT = MAXINT;
	int MIDUINTplus1 = MININT;
	
	int S0 = 0;
	int S1 = MAXUINT;
	
	int MSB = MININT;

	@Test
	public void test_invert_() {
		sloth.push(0);
		sloth._invert_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(-1);
		sloth._invert_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_and_() {
		sloth.push(0);
		sloth.push(0);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(0);
		sloth.push(1);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(1);
		sloth.push(0);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(1);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(0);
		sloth._invert_();
		sloth.push(1);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(1);
		sloth._invert_();
		sloth.push(1);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(S0);
		sloth.push(S0);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(S0, sloth.pop());

		sloth.push(S0);
		sloth.push(S1);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(S0, sloth.pop());
	
		sloth.push(S1);
		sloth.push(S0);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(S0, sloth.pop());

		sloth.push(S1);
		sloth.push(S1);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(S1, sloth.pop());
	}

	@Test
	public void test_l_shift_() {
		sloth.push(1);
		sloth.push(0);
		sloth._l_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(1);
		sloth.push(1);
		sloth._l_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(2, sloth.pop());

		sloth.push(1);
		sloth.push(2);
		sloth._l_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(4, sloth.pop());
	
		sloth.push(1);
		sloth.push(0xF);
		sloth._l_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(0x8000, sloth.pop());
		
		sloth.push(S1);
		sloth.push(1);
		sloth._l_shift_();
		sloth.push(sloth.pop() ^ 1);
		assertEquals(1, sloth.sp);
		assertEquals(S1, sloth.pop());

		sloth.push(MSB);
		sloth.push(1);
		sloth._l_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_minus_() {
		sloth.push(0);
		sloth.push(5);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(-5, sloth.pop());

		sloth.push(5);
		sloth.push(0);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(5, sloth.pop());

		sloth.push(0);
		sloth.push(-5);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(5, sloth.pop());
	
		sloth.push(-5);
		sloth.push(0);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(-5, sloth.pop());

		sloth.push(1);
		sloth.push(2);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(1);
		sloth.push(-2);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(3, sloth.pop());

		sloth.push(-1);
		sloth.push(2);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(-3, sloth.pop());

		sloth.push(-1);
		sloth.push(-2);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(0);
		sloth.push(1);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());
	
		sloth.push(MIDUINTplus1);
		sloth.push(1);
		sloth._minus_();
		assertEquals(1, sloth.sp);
		assertEquals(MIDUINT, sloth.pop());
	}

	@Test
	public void test_plus_() {
		sloth.push(0);
		sloth.push(5);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(5, sloth.pop());

		sloth.push(5);
		sloth.push(0);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(5, sloth.pop());

		sloth.push(0);
		sloth.push(-5);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(-5, sloth.pop());

		sloth.push(-5);
		sloth.push(0);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(-5, sloth.pop());

		sloth.push(1);
		sloth.push(2);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(3, sloth.pop());
	
		sloth.push(1);
		sloth.push(-2);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());
	
		sloth.push(-1);
		sloth.push(2);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());
	
		sloth.push(-1);
		sloth.push(-2);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(-3, sloth.pop());

		sloth.push(-1);
		sloth.push(1);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(MIDUINT);
		sloth.push(1);
		sloth._plus_();
		assertEquals(1, sloth.sp);
		assertEquals(MIDUINTplus1, sloth.pop());
	}

	@Test
	public void test_r_shift_() {
		sloth.push(1);
		sloth.push(0);
		sloth._r_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(1);
		sloth.push(1);
		sloth._r_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(2);
		sloth.push(1);
		sloth._r_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(4);
		sloth.push(2);
		sloth._r_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(0x8000);
		sloth.push(0xF);
		sloth._r_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(1, sloth.pop());

		sloth.push(MSB);
		sloth.push(1);
		sloth._r_shift_();
		sloth.push(MSB);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(MSB);
		sloth.push(1);
		sloth._r_shift_();
		assertEquals(1, sloth.sp);
		assertEquals(MSB, sloth.pop()*2);
	}

	@Test
	public void test_star_() {
		sloth.push(0);
		sloth.push(0);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(0);
		sloth.push(1);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(0);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(2);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(2, sloth.pop());

		sloth.push(2);
		sloth.push(1);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(2, sloth.pop());

		sloth.push(3);
		sloth.push(3);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(9, sloth.pop());

		sloth.push(-3);
		sloth.push(3);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(-9, sloth.pop());

		sloth.push(3);
		sloth.push(-3);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(-9, sloth.pop());

		sloth.push(-3);
		sloth.push(-3);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(9, sloth.pop());

		sloth.push(MIDUINTplus1);
		sloth.push(1);
		sloth._r_shift_();
		sloth.push(2);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(MIDUINTplus1, sloth.pop());

		sloth.push(MIDUINTplus1);
		sloth.push(2);
		sloth._r_shift_();
		sloth.push(4);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(MIDUINTplus1, sloth.pop());

		sloth.push(MIDUINTplus1);
		sloth.push(1);
		sloth._r_shift_();
		sloth.push(MIDUINTplus1 | sloth.pop());
		sloth.push(2);
		sloth._star_();
		assertEquals(1, sloth.sp);
		assertEquals(MIDUINTplus1, sloth.pop());
	}

	@Test
	public void test_two_slash_() {
		sloth.push(S0);
		sloth._two_slash_();
		assertEquals(1, sloth.sp);
		assertEquals(S0, sloth.pop());

		sloth.push(1);
		sloth._two_slash_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(0x4000);
		sloth._two_slash_();
		assertEquals(1, sloth.sp);
		assertEquals(0x2000, sloth.pop());

		sloth.push(S1);
		sloth._two_slash_();
		assertEquals(1, sloth.sp);
		assertEquals(S1, sloth.pop());

		sloth.push(S1^1);
		sloth._two_slash_();
		assertEquals(1, sloth.sp);
		assertEquals(S1, sloth.pop());

		sloth.push(MSB);
		sloth._two_slash_();
		sloth.push(MSB);
		sloth._and_();
		assertEquals(1, sloth.sp);
		assertEquals(MSB, sloth.pop());
	}
	
	@Test
	public void test_u_m_star_() {
		sloth.push(0);
		sloth.push(0);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.pop());

		sloth.push(0);
		sloth.push(1);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(0);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(2);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(2, sloth.pop());
	
		sloth.push(2);
		sloth.push(1);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(2, sloth.pop());

		sloth.push(3);
		sloth.push(3);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(9, sloth.pop());

		sloth.push(MIDUINTplus1>>>1);
		sloth.push(2);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(MIDUINTplus1, sloth.pop());

		sloth.push(MIDUINTplus1);
		sloth.push(2);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(1, sloth.pop());
		assertEquals(0, sloth.pop());
	
		sloth.push(MIDUINTplus1);
		sloth.push(4);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(2, sloth.pop());
		assertEquals(0, sloth.pop());
	
		sloth.push(S1);
		sloth.push(2);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(1, sloth.pop());
		assertEquals(S1<<1, sloth.pop());
	
		sloth.push(MAXUINT);
		sloth.push(MAXUINT);
		sloth._u_m_star_();
		assertEquals(2, sloth.sp);
		assertEquals(~1, sloth.pop());
		assertEquals(1, sloth.pop());
	}

	@Test
	public void test_u_m_slash_mod_() {
		sloth.push(0);
		sloth.push(0);
		sloth.push(1);
		sloth._u_m_slash_mod_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(0);
		sloth.push(2);
		sloth._u_m_slash_mod_();
		assertEquals(2, sloth.sp);
		assertEquals(0, sloth.pop());
		assertEquals(1, sloth.pop());

		sloth.push(3);
		sloth.push(0);
		sloth.push(2);
		sloth._u_m_slash_mod_();
		assertEquals(2, sloth.sp);
		assertEquals(1, sloth.pop());
		assertEquals(1, sloth.pop());

		sloth.push(MAXUINT);
		sloth.push(2);
		sloth._u_m_star_();
		sloth.push(2);
		sloth._u_m_slash_mod_();
		assertEquals(2, sloth.sp);
		assertEquals(MAXUINT, sloth.pop());
		assertEquals(0, sloth.pop());

		sloth.push(MAXUINT);
		sloth.push(2);
		sloth._u_m_star_();
		sloth.push(MAXUINT);
		sloth._u_m_slash_mod_();
		assertEquals(2, sloth.sp);
		assertEquals(2, sloth.pop());
		assertEquals(0, sloth.pop());

		sloth.push(MAXUINT);
		sloth.push(MAXUINT);
		sloth._u_m_star_();
		sloth.push(MAXUINT);
		sloth._u_m_slash_mod_();
		assertEquals(2, sloth.sp);
		assertEquals(MAXUINT, sloth.pop());
		assertEquals(0, sloth.pop());
	}

	/* -- Comparison operators ----------------------------- */

	@Test
	public void test_equals_() {
		sloth.push(0);
		sloth.push(0);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(1);
		sloth.push(1);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(-1);
		sloth.push(-1);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(1);
		sloth.push(0);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(-1);
		sloth.push(0);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(0);
		sloth.push(1);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(0);
		sloth.push(-1);
		sloth._equals_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_less_than_() {
		sloth.push(0);
		sloth.push(1);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(1);
		sloth.push(2);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(-1);
		sloth.push(0);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(-1);
		sloth.push(1);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(MININT);
		sloth.push(0);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());
	
		sloth.push(MININT);
		sloth.push(MAXINT);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(0);
		sloth.push(MAXINT);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(-1, sloth.pop());

		sloth.push(0);
		sloth.push(0);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(1);
		sloth.push(1);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(1);
		sloth.push(0);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	
		sloth.push(2);
		sloth.push(1);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(0);
		sloth.push(-1);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
		
		sloth.push(1);
		sloth.push(-1);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(0);
		sloth.push(MININT);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		sloth.push(MAXINT);
		sloth.push(MININT);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
			
		sloth.push(MAXINT);
		sloth.push(0);
		sloth._less_than_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());
	}

	/* -- Strings ------------------------------------------ */

	@Test
	public void test_string_() {
		sloth.set(100, 5);
		sloth.ip = sloth.to_abs(100);
		sloth._string_();
		assertEquals(2, sloth.sp);
		assertEquals(5, sloth.pop());
		assertEquals(sloth.to_abs(100 + sCELL), sloth.pop());
		assertEquals(sloth.aligned(sloth.to_abs(100 + sCELL + 5*suCHAR)), sloth.ip);
	}

	@Test
	public void test_c_string_() {
		sloth.c_store(sloth.to_abs(100), (char)5);
		sloth.ip = sloth.to_abs(100);
		sloth._c_string_();
		assertEquals(1, sloth.sp);
		assertEquals(sloth.to_abs(100), sloth.pop());
		assertEquals(sloth.aligned(sloth.to_abs(100 + 6*suCHAR)), sloth.ip);
	}

	void do_test_move(int b, int a1, int a2, int u, int v1, int v2, int v3) {
		sloth.push(sloth.to_abs(a1));
		sloth.push(sloth.to_abs(a2));
		sloth.push(u);
		sloth._move_();
		assertEquals(0, sloth.sp);
		assertEquals(v1, sloth.b_fetch(sloth.to_abs(b + 0)));
		assertEquals(v2, sloth.b_fetch(sloth.to_abs(b + 1)));
		assertEquals(v3, sloth.b_fetch(sloth.to_abs(b + 2)));
	}

	@Test
	public void test_move_() {
		int fbuf = 100;
		int sbuf = 103;

		sloth.b_store(fbuf + 0, (byte)20);
		sloth.b_store(fbuf + 1, (byte)20);
		sloth.b_store(fbuf + 2, (byte)20);

		sloth.b_store(sbuf + 0, (byte)12);
		sloth.b_store(sbuf + 1, (byte)34);
		sloth.b_store(sbuf + 2, (byte)56);
	
		do_test_move(fbuf, fbuf, fbuf, 3, 20, 20, 20);
		do_test_move(fbuf, sbuf, fbuf, 0, 20, 20, 20);
		do_test_move(fbuf, sbuf, fbuf, 1, 12, 20, 20);
		do_test_move(fbuf, sbuf, fbuf, 3, 12, 34, 56);
		do_test_move(fbuf, fbuf, fbuf + 1, 2, 12, 12, 34);
		do_test_move(fbuf, fbuf + 1, fbuf, 2, 12, 34, 34);
	}

	/* -- Searching ---------------------------------------- */

	@Test
	public void test_searching() {
		String name = "FIND-ME";
		int name_addr = sloth.fromString(name);
		int w = sloth.header(name_addr, name.length());
		sloth.set_xt(w, 123);

		String s1 = "find-me";
		int s1_addr = sloth.fromString(s1);
		String s2 = "other";
		int s2_addr = sloth.fromString(s2);
		assertTrue(sloth.compare(name_addr, name.length(), s1_addr, s1.length()));
		assertFalse(sloth.compare(name_addr, name.length(), s2_addr, s2.length()));

		assertEquals(w, sloth.search_word(name_addr, name.length()));
		assertEquals(w, sloth.find_word(name));
	
		int cstring = sloth.here();
		sloth.c_comma((char)name.length());
		for (int i = 0; i < name.length(); i++) sloth.c_comma(name.charAt(i));

		sloth.push(cstring);
		sloth._find_();
		assertEquals(-1, sloth.pop()); // Not immediate
		assertEquals(123, sloth.pop());

		sloth._immediate_();
		sloth.push(cstring);
		sloth._find_();
		assertEquals(1, sloth.pop()); // Immediate
		assertEquals(123, sloth.pop());
	}

	/* -- Compile and literal ------------------------------ */

	@Test
	public void test_compile_and_literal() {
		/* Create (LIT) to ensure it's correctly compiled */
		sloth.code("(LIT)", 999);

		sloth.literal(42);
		int h = sloth.here();
		assertEquals(999, sloth.fetch(h - 2*sCELL));
		assertEquals(42, sloth.fetch(h - sCELL));

		sloth.compile(777);
		assertEquals(777, sloth.fetch(sloth.here() - sCELL));
	}

	/* -- Quotations ---------------------------------------- */

	@Test
	public void test_quotation_primitive() {
		sloth.set(100, 3*sCELL);
		sloth.ip = sloth.to_abs(100);
		sloth._quotation_();
		assertEquals(sloth.to_abs(100 + 4*sCELL), sloth.ip);
		assertEquals(sloth.to_abs(100 + sCELL), sloth.pop());
	}

	@Test
	public void test_start_quotation_interpret_mode() {
		sloth.code("(QUOTATION)", 111);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 0);
		sloth._start_quotation_();
		assertEquals(111, sloth.fetch(here));
		assertEquals(0, sloth.fetch(here + sCELL));
		assertEquals(-1, sloth.user_get(Sloth.STATE));
		assertEquals(3, sloth.sp);
		assertEquals(here + sCELL, sloth.pop());
		assertEquals(0, sloth.pop());
		assertEquals(here + 2*sCELL, sloth.pop());
	}

	@Test
	public void test_start_nested_quotation_interpret_mode() {
		sloth.code("(QUOTATION)", 111);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 0);
		sloth._start_quotation_();
		sloth._start_quotation_();
		assertEquals(111, sloth.fetch(here));
		assertEquals(0, sloth.fetch(here + sCELL));
		assertEquals(111, sloth.fetch(here + 2*sCELL));
		assertEquals(0, sloth.fetch(here + 3*sCELL));
		assertEquals(-2, sloth.user_get(Sloth.STATE));
		assertEquals(5, sloth.sp);
		assertEquals(here + 3*sCELL, sloth.pop());
		assertEquals(here + 2*sCELL, sloth.pop());
		assertEquals(here + sCELL, sloth.pop());
		assertEquals(0, sloth.pop());
		assertEquals(here + 2*sCELL, sloth.pop());
	}

	@Test
	public void test_start_quotation_compile_mode() {
		sloth.code("(QUOTATION)", 111);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 1);
		sloth._start_quotation_();
		assertEquals(111, sloth.fetch(here));
		assertEquals(0, sloth.fetch(here + sCELL));
		assertEquals(2, sloth.user_get(Sloth.STATE));
		assertEquals(2, sloth.sp);
		assertEquals(here + sCELL, sloth.pop());
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_start_nested_quotation_compile_mode() {
		sloth.code("(QUOTATION)", 111);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 1);
		sloth._start_quotation_();
		sloth._start_quotation_();
		assertEquals(111, sloth.fetch(here));
		assertEquals(0, sloth.fetch(here + sCELL));
		assertEquals(111, sloth.fetch(here + 2*sCELL));
		assertEquals(0, sloth.fetch(here + 3*sCELL));
		assertEquals(3, sloth.user_get(Sloth.STATE));
		assertEquals(4, sloth.sp);
		assertEquals(here + 3*sCELL, sloth.pop());
		assertEquals(here + 2*sCELL, sloth.pop());
		assertEquals(here + sCELL, sloth.pop());
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_end_quotation_interpret_mode() {
		sloth.code("(QUOTATION)", 111);
		sloth.code("EXIT", 222);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 0);
		sloth._start_quotation_();
		sloth._end_quotation_();

		assertEquals(111, sloth.fetch(here));
		assertEquals(sCELL, sloth.fetch(here + sCELL));
		assertEquals(222, sloth.fetch(here + 2*sCELL));
		assertEquals(0, sloth.user_get(Sloth.STATE));
		assertEquals(1, sloth.sp);
		assertEquals(here + 2*sCELL, sloth.pop());
	}

	@Test
	public void test_end_nested_quotation_interpret_mode() {
		sloth.code("(QUOTATION)", 111);
		sloth.code("EXIT", 222);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 0);
		sloth._start_quotation_();
		sloth._start_quotation_();
		sloth._end_quotation_();

		assertEquals(111, sloth.fetch(here));
		assertEquals(sCELL, sloth.fetch(here + 3*sCELL));
		assertEquals(222, sloth.fetch(here + 4*sCELL));
		assertEquals(-1, sloth.user_get(Sloth.STATE));
		assertEquals(3, sloth.sp);
		assertEquals(here + sCELL, sloth.pop());
		assertEquals(0, sloth.pop());
		assertEquals(here + 2*sCELL, sloth.pop());
	}

	@Test
	public void test_end_quotation_compile_mode() {
		sloth.code("(QUOTATION)", 111);
		sloth.code("EXIT", 222);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 1);
		sloth._start_quotation_();
		sloth._end_quotation_();

		assertEquals(111, sloth.fetch(here));
		assertEquals(sCELL, sloth.fetch(here + sCELL));
		assertEquals(222, sloth.fetch(here + 2*sCELL));
		assertEquals(1, sloth.user_get(Sloth.STATE));
		assertEquals(0, sloth.sp);
	}

	@Test
	public void test_end_nested_quotation_compile_mode() {
		sloth.code("(QUOTATION)", 111);
		sloth.code("EXIT", 222);

		int here = sloth.here();
		sloth.user_set(Sloth.STATE, 1);
		sloth._start_quotation_();
		sloth._start_quotation_();
		sloth._end_quotation_();
		sloth._end_quotation_();

		assertEquals(111, sloth.fetch(here));
		assertEquals(sCELL, sloth.fetch(here + 3*sCELL));
		assertEquals(222, sloth.fetch(here + 4*sCELL));
		assertEquals(1, sloth.user_get(Sloth.STATE));
		assertEquals(0, sloth.sp);
	}

	/* -- User variable creation --------------------------- */

	@Test
	public void test_user_variable_creation() {
		int h;
		sloth.code("EXIT", 111);
		sloth.code("(LIT)", 222);
		sloth.user_variable("TEST-VAR", 8, 13);
		assertEquals(111, sloth.fetch(sloth.here() - sCELL));
		assertEquals(sloth.to_abs(8, sloth.u), sloth.fetch(sloth.here() - 2*sCELL));
		assertEquals(222, sloth.fetch(sloth.here() - 3*sCELL));
		assertEquals(13, sloth.fetch(sloth.to_abs(8, sloth.u)));
	}

	/* Source code preprocessing, interpreting & auditing commands */

	@Test
	public void test_file_position() {
		try {
			Path f = Files.createTempFile(null, null);
			RandomAccessFile raf = new RandomAccessFile(f.toFile(), "rw");
			int idx = sloth.op++;
			sloth.o.put(idx, raf);

			raf.writeBytes("abc");

			sloth.push(idx);
			sloth._file_position_();
			assertEquals(3, sloth.sp);
			assertEquals(0, sloth.pop());
			assertEquals(0, sloth.pop());
			assertEquals(3, sloth.pop());
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Test
	public void test_read_line() {
		int buf_idx = sloth.op++;
		sloth.o.put(buf_idx, ByteBuffer.allocate(16));
		int buf = sloth.to_abs(0, buf_idx);
		try {
			Path f = Files.createTempFile(null, null);
			RandomAccessFile raf = new RandomAccessFile(f.toFile(), "rw");
			int file_idx = sloth.op++;
			sloth.o.put(file_idx, raf);

			raf.writeBytes("abc");

			/* Test read at end of file */

			sloth.push(buf);
			sloth.push(16);
			sloth.push(file_idx);

			sloth._read_line_();

			assertEquals(3, sloth.sp);
			assertEquals(0, sloth.pop());
			assertEquals(0, sloth.pop());
			assertEquals(0, sloth.pop());

			/* Test correct read */

			raf.seek(0L);

			sloth.push(buf);
			sloth.push(16);
			sloth.push(file_idx);

			sloth._read_line_();
	
			assertEquals(3, sloth.sp);
			assertEquals(0, sloth.pop());
			assertNotEquals(0, sloth.pop());
			assertEquals(3, sloth.pop());
			assertEquals('a', sloth.c_fetch(buf + 0*suCHAR));
			assertEquals('b', sloth.c_fetch(buf + 1*suCHAR));
			assertEquals('c', sloth.c_fetch(buf + 2*suCHAR));
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	void my_accept(Sloth vm) { vm.push(15); }

	@Test
	public void test_refill() {
		int buf_idx = sloth.op++;
		sloth.o.put(buf_idx, ByteBuffer.allocate(16));
		int buf = sloth.to_abs(0, buf_idx);

		sloth.user_set(Sloth.SOURCE_ID, -1);
		sloth._refill_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.pop());

		int ibuf = sloth.user_get(Sloth.IBUF);
		sloth.code("ACCEPT", sloth.primitive((vm) -> my_accept(vm)));
		sloth.user_set(Sloth.SOURCE_ID, 0);
		sloth._refill_();
		assertEquals(ibuf, sloth.user_get(Sloth.IBUF));
		assertEquals(15, sloth.user_get(Sloth.ILEN));
		assertEquals(0, sloth.user_get(Sloth.IPOS));
	}

	@Test
	public void test_refill_file() {
		int buf_idx = sloth.op++;
		sloth.o.put(buf_idx, ByteBuffer.allocate(16));
		int buf = sloth.to_abs(0, buf_idx);

		try {
			Path f = Files.createTempFile(null, null);
			RandomAccessFile raf = new RandomAccessFile(f.toFile(), "rw");
			int file_idx = sloth.op++;
			sloth.o.put(file_idx, raf);

			sloth.user_set(Sloth.IBUF, buf);
			sloth.user_set(Sloth.IPOS, 0);
			sloth.user_set(Sloth.ILEN, 1024);

			raf.writeBytes("abc\ndefg");

			raf.seek(0);

			sloth.user_set(Sloth.SOURCE_ID, file_idx);

			sloth._refill_();

			assertEquals(1, sloth.sp);
			assertEquals(-1, sloth.pop());

			assertEquals(0, sloth.user_get(Sloth.IPOS));
			assertEquals(3, sloth.user_get(Sloth.ILEN));
			assertEquals("abc", sloth.toString(sloth.user_get(Sloth.IBUF), 3));

			sloth.push(file_idx);
			sloth._file_position_();
			sloth.pop();
			sloth.pop();
			assertEquals(4, sloth.pop());

			sloth._refill_();
			assertEquals(-1, sloth.pop());

			assertEquals(0, sloth.user_get(Sloth.IPOS));
			assertEquals(4, sloth.user_get(Sloth.ILEN));
			assertEquals("defg", sloth.toString(sloth.user_get(Sloth.IBUF), 4));

			sloth.push(file_idx);
			sloth._file_position_();
			sloth.pop();
			sloth.pop();
			assertEquals(8, sloth.pop());
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/* Portable temp file creation code */

	public String write_temp_file(String content) {
		try {
			Path tempFile = Files.createTempFile(null, null);
			tempFile.toFile().deleteOnExit(); // cleaned up when JVM exits
			Files.writeString(tempFile, content);
			return tempFile.toAbsolutePath().toString();
		} catch (IOException e) {
			e.printStackTrace();
			return "";
		}
	}
	
	int interpret_calls;
	
	public void noop_interpret(Sloth vm) {
		interpret_calls++;
	}

	@Test
	public void test_included_absolute_path() {
		// Set PATH_START and PATH_END pointing to an empty buffer
		int path_idx = sloth.op++;
		sloth.o.put(path_idx, ByteBuffer.allocate(256));
		int path = sloth.to_abs(0, path_idx);

		sloth.user_set(Sloth.PATH_START, path);
		sloth.user_set(Sloth.PATH_END, path);

		String tmppath = write_temp_file("line one\nline two");

		sloth.user_set(Sloth.INTERPRET, sloth.primitive((vm) -> noop_interpret(vm)));
		interpret_calls = 0;

		int saved_incl = sloth.user_get(Sloth.INCLUDED_FILES);

		int ibuf = sloth.user_get(Sloth.IBUF);
		int ipos = sloth.user_get(Sloth.IPOS);
		int ilen = sloth.user_get(Sloth.ILEN);
		int source = sloth.user_get(Sloth.SOURCE_ID);
		int source_pos = sloth.user_get(Sloth.SOURCE_POS);

		sloth.push(sloth.fromString(tmppath));
		sloth.push(tmppath.length());
		sloth._included_();

		assertEquals(0, sloth.sp);
		assertEquals(2, interpret_calls);

		assertEquals(source_pos, sloth.user_get(Sloth.SOURCE_POS));
		assertEquals(source, sloth.user_get(Sloth.SOURCE_ID));
		assertEquals(ilen, sloth.user_get(Sloth.ILEN));
		assertEquals(ipos, sloth.user_get(Sloth.IPOS));
		assertEquals(ibuf, sloth.user_get(Sloth.IBUF));

		/* A new entry must have been prepended to INCLUDED_FILES */
		int new_head = sloth.user_get(Sloth.INCLUDED_FILES);
		assertNotEquals(saved_incl, new_head);
		assertEquals(tmppath.length(), sloth.fetch(new_head + sCELL));
		assertEquals(tmppath, sloth.toString(new_head + 2*sCELL, tmppath.length()));
	}

	@Test
	public void test_included_file_not_found() {
		int path_idx = sloth.op++;
		sloth.o.put(path_idx, ByteBuffer.allocate(256));
		int path = sloth.to_abs(0, path_idx);

		sloth.user_set(Sloth.PATH_START, path);
		sloth.user_set(Sloth.PATH_END, path);

		int throw_prim = sloth.primitive((vm) -> vm._included_());		
		String missing = "sloth_test_no_such_file.4th";
		sloth.push(sloth.fromString(missing));
		sloth.push(missing.length());
		sloth.push(throw_prim);
		sloth._catch_();

		assertEquals(-38, sloth.pop());
		/* After throwing, the stack is restored to */
		/* the depth previous to the catch. */
		assertEquals(2, sloth.sp);
	}

	void included_relative_path_interpret(Sloth vm) {
		interpret_calls++;
		if (interpret_calls == 1) {
			vm._included_();	
		}
	}

	@Test
	public void test_included_relative_path() {
		sloth.user_set(Sloth.INTERPRET, sloth.primitive((vm) -> included_relative_path_interpret(vm)));
		interpret_calls = 0;

		String tmppath = write_temp_file("line one\nline two");

		// Split tmppath into directory and filename
		int sep = tmppath.lastIndexOf(System.getProperty("file.separator"));
		assertNotEquals(0, sep);
		int dirlen = sep + 1;

		// Reserve a string big enough for this tests
		sloth.user_set(Sloth.PATH_START, sloth.fromString("                                                  "));
		sloth.user_set(Sloth.PATH_END, sloth.user_get(Sloth.PATH_START));

		// I push the filename only first to be used by the
		// _include_ called by custom_included_interpret
		sloth.push(sloth.fromString(tmppath.substring(dirlen, tmppath.length())));
		sloth.push(tmppath.length() - dirlen);
		sloth.push(sloth.fromString(tmppath));
		sloth.push(tmppath.length());
		sloth._included_();

		assertEquals(0, sloth.sp);
		assertEquals(4, interpret_calls);
	}

	@Test
	public void test_included_root_path() {
		sloth.user_set(Sloth.INTERPRET, sloth.primitive((vm) -> noop_interpret(vm)));
		interpret_calls = 0;

		String tmppath = write_temp_file("line one\nline two");

		// Split tmppath into directory and filename
		int sep = tmppath.lastIndexOf(System.getProperty("file.separator"));
		assertNotEquals(0, sep);
		int dirlen = sep + 1;

		// Put the temp file's directory into PATHS as the root
		// PATH_START/PATH_END point to an empty path so the first two
		// strategies (absolute and relative-to-previous) both fail
		for (int i = 0; i < tmppath.length(); i++) {
			sloth.c_store(sloth.to_abs(Sloth.PATHS, sloth.u) + i*suCHAR, tmppath.charAt(i));
		}
		sloth.user_set(Sloth.ROOT_PATH_LENGTH, dirlen);
		sloth.user_set(Sloth.PATH_START, sloth.to_abs(Sloth.PATHS, sloth.u) + dirlen*suCHAR);
		sloth.user_set(Sloth.PATH_END, sloth.to_abs(Sloth.PATHS, sloth.u) + dirlen*suCHAR);
		int filename = sloth.to_abs(Sloth.PATHS, sloth.u) + dirlen*suCHAR;
		int filelen = tmppath.length() - dirlen;

		// Push only the filename (no directory)
		sloth.push(filename);
		sloth.push(filelen);
		sloth._included_();
	
		assertEquals(0, sloth.sp);
		assertEquals(2, interpret_calls);
	}

	// Next test is exact to previous but using set_root_path
	// function.
	@Test
	public void test_included_with_set_root_path() {
		sloth.user_set(Sloth.INTERPRET, sloth.primitive((vm) -> noop_interpret(vm)));
		interpret_calls = 0;

		String tmppath = write_temp_file("line one\nline two");

		// Split tmppath into directory and filename
		int sep = tmppath.lastIndexOf(System.getProperty("file.separator"));
		assertNotEquals(0, sep);
		int dirlen = sep + 1;

		sloth.set_root_path(tmppath.substring(0, dirlen));
		int filename = sloth.fromString(tmppath.substring(dirlen, tmppath.length()));
		int filelen = tmppath.length() - dirlen;

		// Push only the filename (no directory)
		sloth.push(filename);
		sloth.push(filelen);
		sloth._included_();
	
		assertEquals(0, sloth.sp);
		assertEquals(2, interpret_calls);
	}

	void refill_interpret(Sloth vm) {
		vm._refill_();
		vm.pop();
		interpret_calls++;
	}

	@Test
	public void test_included_and_refill() {
		sloth.user_set(Sloth.INTERPRET, sloth.primitive((vm) -> refill_interpret(vm)));
		interpret_calls = 0;

		String tmpfile = write_temp_file("line one\nline two\nline three\nline four");
		int tmppath = sloth.fromString(tmpfile);

		// Set PATH_START and PATH_END variables to an empty buffer
		int path_buf = sloth.fromString(" ".repeat(260));
		sloth.user_set(Sloth.PATH_START, path_buf);
		sloth.user_set(Sloth.PATH_END, path_buf);

		int saved_incl = sloth.user_get(Sloth.INCLUDED_FILES);		
	
		int ibuf = sloth.user_get(Sloth.IBUF);
		int ipos = sloth.user_get(Sloth.IPOS);
		int ilen = sloth.user_get(Sloth.ILEN);
		int source = sloth.user_get(Sloth.SOURCE_ID);
		int source_pos = sloth.user_get(Sloth.SOURCE_POS);

		sloth.push(tmppath);
		sloth.push(tmpfile.length());
		sloth._included_();

		assertEquals(0, sloth.sp);
		assertEquals(2, interpret_calls);

		assertEquals(source_pos, sloth.user_get(Sloth.SOURCE_POS));
		assertEquals(source, sloth.user_get(Sloth.SOURCE_ID));
		assertEquals(ilen, sloth.user_get(Sloth.ILEN));
		assertEquals(ipos, sloth.user_get(Sloth.IPOS));
		assertEquals(ibuf, sloth.user_get(Sloth.IBUF));

		// A new entry must have been prepended to INCLUDED_FILES
		int new_head = sloth.user_get(Sloth.INCLUDED_FILES);
		assertNotEquals(saved_incl, new_head);
		assertEquals(saved_incl, sloth.fetch(new_head));
		// link to prev
		assertEquals(tmpfile.length(), sloth.fetch(new_head + sCELL)); // name len
		assertEquals(tmpfile, sloth.toString(new_head + 2*sCELL, tmpfile.length())); // name
	}

	@Test
	public void test_add_to_included_files_list() {
	}

	// -- Input/Output and parsing -------------------------

	// Custom EMIT for testing — replaces the default printf one 
	char emitted_char = 0;
	void test_emit_(Sloth vm) { emitted_char = (char)vm.pop(); }

	@Test
	public void test_emit() {
		// Test the test_emit_ function
		emitted_char = 0;
		sloth.push('Z');
		test_emit_(sloth);
		assertEquals(0, sloth.sp);
		assertEquals('Z', emitted_char);

		// Ensure that our custom emit is being used
		sloth.primitive((vm) -> test_emit_(vm));
		sloth.push('Z');
		sloth.do_prim(-1);
		assertEquals(0, sloth.sp);
		assertEquals('Z', emitted_char);
	}

	// Custom KEY for testing
	int mock_key_char = 0;
	void test_key_(Sloth vm) { sloth.push(mock_key_char); }

	@Test
	public void test_key_pushes_char_onto_stack() {
		mock_key_char = 'Q';
		test_key_(sloth);
		assertEquals(1, sloth.sp);
		assertEquals('Q', sloth.pop());
		assertEquals(0, sloth.sp);
	
		// Ensure that our custom key is being used
		mock_key_char = 'Q';
		sloth.primitive((vm) -> test_key_(vm));
		sloth.do_prim(-1);
		assertEquals(1, sloth.sp);
		assertEquals('Q', sloth.pop());
		assertEquals(0, sloth.sp);
	}

	@Test
	public void test_key_handles_zero() {
		mock_key_char = 0;
		test_key_(sloth);
		assertEquals(0, sloth.pop());
	}

	@Test
	public void test_key_handles_max_uchar() {
		mock_key_char = 255;
		test_key_(sloth);
		assertEquals(255, sloth.pop());
	}

	@Test
	public void test_source_() {
		int ibuf = sloth.fromString("TEST");
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.ILEN, 4);
		sloth._source_();
		assertEquals(2, sloth.sp);
		assertEquals(4, sloth.pop());
		assertEquals(ibuf, sloth.pop());
	}

	@Test
	public void test_word_() {
		int ibuf = sloth.fromString("Hello world");
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 11);
		sloth.push(' ');
		sloth._word_();
		assertEquals(1, sloth.sp);
		int addr = sloth.pop();
		assertEquals(5, sloth.c_fetch(addr));
		assertEquals('H', sloth.c_fetch(addr + 1*suCHAR));
		assertEquals('e', sloth.c_fetch(addr + 2*suCHAR));
		assertEquals('l', sloth.c_fetch(addr + 3*suCHAR));
		assertEquals('l', sloth.c_fetch(addr + 4*suCHAR));
		assertEquals('o', sloth.c_fetch(addr + 5*suCHAR));

		sloth.push(' ');
		sloth._word_();
		assertEquals(1, sloth.sp);
		addr = sloth.pop();
		assertEquals(5, sloth.c_fetch(addr));
		assertEquals('w', sloth.c_fetch(addr + 1*suCHAR));
		assertEquals('o', sloth.c_fetch(addr + 2*suCHAR));
		assertEquals('r', sloth.c_fetch(addr + 3*suCHAR));
		assertEquals('l', sloth.c_fetch(addr + 4*suCHAR));
		assertEquals('d', sloth.c_fetch(addr + 5*suCHAR));

		sloth.push(' ');
		sloth._word_();
		assertEquals(1, sloth.sp);
		assertEquals(0, sloth.c_fetch(sloth.pop()));
	}

	// -- Outer interpreter --------------------------------

	@Test
	public void test_interpret_empty() {
		sloth.user_set(Sloth.IBUF, 0);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 0);
		sloth._interpret_();
		assertEquals(0, sloth.sp);
	}

	@Test
	public void test_interpret_character_literals_interpret_mode() {
		int ibuf = sloth.fromString("'a' 'b'");
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 7);
		sloth._interpret_();
		assertEquals(2, sloth.sp);
		assertEquals('b', sloth.pop());
		assertEquals('a', sloth.pop());
	}

	@Test
	public void test_interpret_character_literals_compile_mode() {
		int ibuf = sloth.fromString("'a' 'b'");
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 7);
		sloth.user_set(Sloth.STATE, 1);
		sloth.code("(LIT)", -2);
		int here = sloth.here();
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals('a', sloth.fetch(here + sCELL));
		assertEquals('b', sloth.fetch(here + 3*sCELL));
	}

	@Test 
	public void test_interpret_number_literals_interpret_mode() {
		int ibuf = sloth.fromString("1 0 37 -560");
		sloth.user_set(Sloth.BASE, 10);
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 11);
		sloth._interpret_();
		assertEquals(4, sloth.sp);
		assertEquals(-560, sloth.pop());
		assertEquals(37, sloth.pop());
		assertEquals(0, sloth.pop());
		assertEquals(1, sloth.pop());

		ibuf = sloth.fromString("%1111 #15 $f");
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 12);
		sloth._interpret_();
		assertEquals(3, sloth.sp);
		assertEquals(15, sloth.pop());
		assertEquals(15, sloth.pop());
		assertEquals(15, sloth.pop());
	}

	@Test
	public void test_interpret_number_literals_compile_mode() {
		int ibuf = sloth.fromString("1 0 37 -560");
		sloth.user_set(Sloth.BASE, 10);
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 11);
		sloth.user_set(Sloth.STATE, 1);
		sloth.code("(LIT)", -2);
		int here = sloth.here();
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(1, sloth.fetch(here + sCELL));
		assertEquals(0, sloth.fetch(here + 3*sCELL));
		assertEquals(37, sloth.fetch(here + 5*sCELL));
		assertEquals(-560, sloth.fetch(here + 7*sCELL));

		ibuf = sloth.fromString("%1111 #15 $f");
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 12);
		sloth.user_set(Sloth.STATE, 1);
		sloth.code("(LIT)", -2);
		here = sloth.here();
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(15, sloth.fetch(here + sCELL));
		assertEquals(15, sloth.fetch(here + 3*sCELL));
		assertEquals(15, sloth.fetch(here + 5*sCELL));
	}

	// TODO Tests for floating point literals
	
	int p1 = 0, p2 = 0;
	void primitive1(Sloth vm) { p1 = 1; }
	void primitive2(Sloth vm) { p2 = 1; }

	@Test
	public void test_interpret_() {
		int ibuf = sloth.fromString("11 PRIM1 13 PRIM2");
		sloth.user_set(Sloth.BASE, 10);
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 17);
		sloth.code("PRIM1", sloth.primitive((vm) -> primitive1(vm)));
		sloth.code("PRIM2", sloth.primitive((vm) -> primitive2(vm)));
		sloth._interpret_();
		assertEquals(2, sloth.sp);
		assertEquals(13, sloth.pop());
		assertEquals(11, sloth.pop());
		assertEquals(1, p1);
		assertEquals(1, p2);
	}

	@Test
	public void test_interpret_compile_mode() {
		int ibuf = sloth.fromString("11 PRIM1 13 PRIM2");
		sloth.user_set(Sloth.BASE, 10);
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 17);
		sloth.user_set(Sloth.STATE, 1);
		sloth.code("EXIT", -1);
		sloth.code("(LIT)", -2);
		sloth.code("PRIM1", -3);
		sloth.code("PRIM2", -4);
		int here = sloth.here();
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(-2, sloth.fetch(here + 0*sCELL));
		assertEquals(11, sloth.fetch(here + 1*sCELL));
		assertEquals(-3, sloth.fetch(here + 2*sCELL));
		assertEquals(-2, sloth.fetch(here + 3*sCELL));
		assertEquals(13, sloth.fetch(here + 4*sCELL));
		assertEquals(-4, sloth.fetch(here + 5*sCELL));
	}

	@Test
	public void test_interpret_immediate_words_compile_mode() {
		int ibuf = sloth.fromString("11 PRIM1 13 PRIM2");
		sloth.user_set(Sloth.BASE, 10);
		sloth.user_set(Sloth.IBUF, ibuf);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 17);
		sloth.user_set(Sloth.STATE, 1);
		sloth.code("PRIM1", sloth.primitive((vm) -> primitive1(vm)));
		sloth._immediate_();
		sloth.code("(LIT)", -2);
		sloth.code("PRIM2", -3);
		int here = sloth.here();
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(-2, sloth.fetch(here + 0*sCELL));
		assertEquals(11, sloth.fetch(here + 1*sCELL));
		assertEquals(-2, sloth.fetch(here + 2*sCELL));
		assertEquals(13, sloth.fetch(here + 3*sCELL));
		assertEquals(-3, sloth.fetch(here + 4*sCELL));
		assertEquals(1, p1);
	}

	// -- Defining words -----------------------------------

	@Test
	public void test_colon_() {
		String name = "TEST";
		int here = sloth.here();
		sloth.user_set(Sloth.IBUF, sloth.fromString(name));
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 11);
		sloth._colon_();
		assertEquals(0, sloth.sp);
		assertEquals(here, sloth.get_latest());
		assertEquals(sloth.here(), sloth.user_get(Sloth.LATESTXT));
		assertEquals(Sloth.HIDDEN, sloth.get_flags(sloth.get_latest()));
		assertEquals(1, sloth.user_get(Sloth.STATE));
	}

	@Test
	public void test_colon_no_name_() {
		int here = sloth.here();
		sloth._colon_no_name_();
		assertEquals(1, sloth.sp);
		assertEquals(sloth.here(), sloth.user_get(Sloth.LATESTXT));
		assertEquals(1, sloth.user_get(Sloth.STATE));
	}

	@Test
	public void test_semicolon_() {
		String name = "TEST";
		int here = sloth.here();
		sloth.user_set(Sloth.IBUF, sloth.fromString(name));
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 11);
		sloth._colon_();
		sloth.code("EXIT", -1);
		sloth._semicolon_();
		assertEquals(0, sloth.sp);
		assertEquals(0, sloth.user_get(Sloth.STATE));
	}

	@Test
	public void test_recurse_() {
		int here = sloth.here();
		sloth.user_set(Sloth.LATESTXT, 1111);
		sloth._recurse_();
		assertEquals(1111, sloth.fetch(here));
	}

	@Test
	public void test_immediate_() {
		int w = sloth.header("IMM-WORD");
		assertFalse(sloth.has_flag(w, Sloth.IMMEDIATE));
		sloth._immediate_();
		assertTrue(sloth.has_flag(w, Sloth.IMMEDIATE));
	}

	int postpone_p1;
	void postpone_primitive1(Sloth vm) { postpone_p1 = 1;	}

	@Test
	public void test_postpone_() {
		int ibuf1 = sloth.fromString(": TEST IMM-WORD ;");
		int ibuf2 = sloth.fromString(": TEST POSTPONE IMM-WORD ;");
		int ibuf3 = sloth.fromString("TEST");
		sloth.user_set(Sloth.IBUF, ibuf1);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 17);
		postpone_p1 = 0;
		sloth.code("EXIT", sloth.primitive((vm) -> vm._exit_()));
		sloth.code("(LIT)", sloth.primitive((vm) -> vm._lit_()));
		sloth.code("(RIP)", sloth.primitive((vm) -> vm._rip_()));
		sloth.code(":", sloth.primitive((vm) -> vm._colon_()));
		sloth.code(";", sloth.primitive((vm) -> vm._semicolon_()));
		sloth._immediate_();
		sloth.code("COMPILE,", sloth.primitive((vm) -> vm._compile_comma_()));
		sloth.code("POSTPONE", sloth.primitive((vm) -> vm._postpone_()));
		sloth._immediate_();
		sloth.code("IMM-WORD", sloth.primitive((vm) -> postpone_primitive1(vm)));
		sloth._immediate_();
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(1, postpone_p1);

		// Test that IMM-WORD is not executed when POSTPONEd
		sloth.user_set(Sloth.IBUF, ibuf2);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 26);
		postpone_p1 = 0;
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(0, postpone_p1);

		// Test that IMM-WORD was compiled when POSTPONEd
		sloth.user_set(Sloth.IBUF, ibuf3);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 4);
		sloth._interpret_();
		assertEquals(0, sloth.sp);
		assertEquals(1, postpone_p1);
	}

	@Test
	public void test_compile_comma_() {
		int here = sloth.here();
		sloth.push(123);
		sloth._compile_comma_();
		assertEquals(0, sloth.sp);
		assertEquals(123, sloth.fetch(here));
	}

	@Test
	public void test_create_() {
		int name = sloth.fromString("TEST");
		sloth.user_set(Sloth.IBUF, name);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 4);
		sloth.code("(RIP)", -2);
		sloth.code("EXIT", -1);
		int here = sloth.here();
		sloth._create_();
		assertEquals(0, sloth.sp);
		int xt = sloth.fetch(here + sCELL);
		assertEquals(-2, sloth.fetch(xt));
		assertEquals(4*sCELL, sloth.fetch(xt+sCELL));
		assertEquals(-1, sloth.fetch(xt+2*sCELL));
		assertEquals(-1, sloth.fetch(xt+3*sCELL));
	}

	@Test
	public void test_do_does_() {
		int name = sloth.fromString("TEST");
		sloth.user_set(Sloth.IBUF, name);
		sloth.user_set(Sloth.IPOS, 0);
		sloth.user_set(Sloth.ILEN, 4);
		sloth.code("(RIP)", -2);
		sloth.code("EXIT", -1);
		int here = sloth.here();
		sloth._create_();
		int xt = sloth.fetch(here + sCELL);
		assertEquals(-1, sloth.fetch(xt+2*sCELL));
		sloth.push(13);
		sloth._do_does_();
		assertEquals(13, sloth.fetch(xt+2*sCELL));
	}

	@Test
	public void test_does_() {
		sloth.code("(LIT)", -3);
		sloth.code("(DOES)", -2);
		sloth.code("EXIT", -1);
		int here = sloth.here();
		sloth._does_();
		assertEquals(0, sloth.sp);
		assertEquals(-3, sloth.fetch(here));
		assertEquals(here + 4*sCELL, sloth.fetch(here+sCELL));
		assertEquals(-2, sloth.fetch(here+2*sCELL));
		assertEquals(-1, sloth.fetch(here+3*sCELL));
	}

	@Test
	public void test_evaluate_() {
		int buf = sloth.fromString("11 12 +");
		sloth.user_set(Sloth.BASE, 10);
		sloth.push(buf);
		sloth.push(7);
		sloth.code("+", sloth.primitive((vm) -> vm._plus_()));
		sloth.user_set(Sloth.INTERPRET, sloth.primitive((vm) -> vm._interpret_()));
		sloth._evaluate_();
		assertEquals(1, sloth.sp);
		assertEquals(23, sloth.pop());
	}

	int p0;
	void primitive0(Sloth vm) { p0 = 1; }

	@Test
	public void test_execute_() {
		p0 = 0;
		sloth.push(sloth.primitive((vm) -> primitive0(vm)));
		sloth._execute_();
		assertEquals(0, sloth.sp);
		assertEquals(1, p0);
	}

	// -- Bootstrapping ------------------------------------

	@Test
	public void test_bootstrap() {
		sloth.bootstrap();

		assertNotEquals(0, sloth.find_word("EXIT"));
		assertNotEquals(0, sloth.find_word("DUP"));
		assertNotEquals(0, sloth.find_word("@"));
	}

	@Test
	public void test_bootstrap_user_area() {
		sloth.bootstrap();
		
		assertEquals(sloth.to_abs(Sloth.FORTH_WL), sloth.user_get(Sloth.CURRENT));
		assertEquals(2, sloth.user_get(Sloth.ORDER));
		assertEquals(0, sloth.user_get(Sloth.LOCALS_WORDLIST));
		assertEquals(sloth.to_abs(Sloth.FORTH_WL), sloth.user_get(Sloth.CONTEXT));
		assertEquals(sloth.to_abs(Sloth.INTERNAL_WL), sloth.user_get(Sloth.CONTEXT + sCELL));
		assertEquals(10, sloth.user_get(Sloth.BASE));
		assertEquals(0, sloth.user_get(Sloth.STATE));
		assertEquals(0, sloth.user_get(Sloth.IBUF));
		assertEquals(0, sloth.user_get(Sloth.IPOS));
		assertEquals(0, sloth.user_get(Sloth.ILEN));
		assertEquals(0, sloth.user_get(Sloth.SOURCE_ID));
		assertEquals(0, sloth.user_get(Sloth.SOURCE_POS));
		assertEquals(0, sloth.user_get(Sloth.LATESTXT));
		assertNotEquals(0, sloth.user_get(Sloth.INTERPRET));
		assertEquals(0, sloth.user_get(Sloth.ROOT_PATH_LENGTH));
		assertEquals(sloth.to_abs(Sloth.PATHS, sloth.u), sloth.user_get(Sloth.PATH_START));
		assertEquals(sloth.to_abs(Sloth.PATHS, sloth.u), sloth.user_get(Sloth.PATH_END));
		assertEquals(0, sloth.user_get(Sloth.PATHS));
		assertEquals(0, sloth.user_get(Sloth.INCLUDED_FILES));
	}
}
