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
import java.nio.ByteBuffer;

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
		assertEquals(32767, sloth.m.get(0).capacity());
		assertEquals(1024, sloth.m.get(1).capacity());
		assertEquals(3*sCELL, sloth.m.get(0).getInt(0*sCELL));
		assertEquals(0, sloth.m.get(0).getInt(1*sCELL));
		assertEquals(0, sloth.m.get(0).getInt(2*sCELL));
		assertEquals(0, sloth.m.get(0).getInt(3*sCELL));
		assertEquals(2*sCELL, sloth.m.get(1).getInt(0*sCELL));
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
		assertEquals(sloth.m.get(0).capacity() - sloth.here(), sloth.pop());
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
		assertEquals(sloth.fetch(sloth.to_abs(0, 0)), sloth.pop());
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
		assertEquals(13, sloth.get_link(sloth.to_abs(100, 0)));
	}

	@Test
	public void test_get_set_xt() {
		sloth.set_xt(sloth.to_abs(100, 0), 17);
		assertEquals(17, sloth.get_xt(sloth.to_abs(100, 0)));
	}

	@Test
	public void test_get_set_flags() {
		sloth.set_flags(sloth.to_abs(100, 0), 33);
		assertEquals(33, sloth.get_flags(sloth.to_abs(100, 0)));
	}

	@Test
	public void test_has_set_unset_flag() {
		sloth.set_flags(sloth.to_abs(100, 0), 0);
		assertFalse(sloth.has_flag(sloth.to_abs(100, 0), 2));
		sloth.set_flag(sloth.to_abs(100, 0), 2);
		assertTrue(sloth.has_flag(sloth.to_abs(100, 0), 2));
		sloth.unset_flag(sloth.to_abs(100, 0), 2);
		assertFalse(sloth.has_flag(sloth.to_abs(100, 0), 2));
	}

	@Test
	public void test_headers() {
		String name = "NEW-WORD";
		int name_addr = sloth.FromString(name);
		int w = sloth.header(name_addr, name.length());
		assertEquals(sloth.aligned(w + 2*sCELL + 2*suCHAR + name.length()*suCHAR), sloth.here());
		assertEquals(sloth.aligned(sloth.here()), sloth.here());
		assertEquals(3*sCELL, sloth.get_latest());
		assertEquals(sloth.here(), sloth.fetch(4*sCELL));
		assertEquals(0, sloth.c_fetch(5*sCELL));
		assertEquals(name, sloth.ToString(5*sCELL + 2*suCHAR, sloth.c_fetch(5*sCELL + suCHAR)));
	}

	@Test
	public void test_name_and_len() {
		String name = "TEST-WORD";
		int name_addr = sloth.FromString(name);
		int w = sloth.header(name_addr, name.length());
		assertEquals(name.length(), sloth.get_namelen(w));
		assertEquals(name, sloth.ToString(sloth.get_name_addr(w), sloth.get_namelen(w)));
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
		assertEquals("MY-PRIMITIVE", sloth.ToString(sloth.get_name_addr(word), sloth.get_namelen(word)));
	}

	/* -- Inner interpreter -------------------------------- */
	
	@Test
	public void test_op() {
		sloth.set(3*sCELL, 11);
		sloth.ip = sloth.to_abs(3*sCELL, 0);
		assertEquals(11, sloth.op());
		assertEquals(sloth.to_abs(4*sCELL, 0), sloth.ip);
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
		sloth.execute(sloth.to_abs(3*sCELL, 0));
		assertEquals(0, sloth.rp);
		assertEquals(sloth.to_abs(3*sCELL, 0), sloth.ip);
	}

	void my_exit(Sloth vm) { sloth.ip=  -1; }

	@Test
	public void test_inner() {
		sloth.set(3*sCELL, sloth.primitive((vm) -> my_primitive(vm)));
		sloth.set(4*sCELL, sloth.primitive((vm) -> my_exit(vm)));
		sloth.ip = sloth.to_abs(3*sCELL, 0);
		my_var = 11;
		sloth.inner();
		assertEquals(17, my_var);
		assertEquals(-1, sloth.ip);
	}

	@Test
	public void test_eval() {
		sloth.set(3*sCELL, sloth.primitive((vm) -> my_primitive(vm)));
		sloth.set(4*sCELL, sloth.primitive((vm) -> my_exit(vm)));

		sloth.ip = sloth.to_abs(3*sCELL, 0);
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
		sloth.ip = sloth.to_abs(4*sCELL, 0);
		sloth.debug_inner(-3);
		assertEquals(23, my_var);
		assertEquals(1, sloth.sp);
		assertEquals(sloth.to_abs(4*sCELL, 0), sloth.s[0]);
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

		sloth.push(sloth.to_abs(3*sCELL, 0));
		sloth.push(-2);
		sloth.push(-3);
		sloth.push(-4);
		
		sloth._debug_();

		assertEquals(13, pre_var);
		assertEquals(19, inner_var);
		assertEquals(27, post_var);
	}
}
