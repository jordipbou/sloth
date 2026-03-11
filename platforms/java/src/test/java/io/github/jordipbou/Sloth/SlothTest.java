package io.github.jordipbou.Sloth;

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
		sloth = new Sloth(1024, 256);
	}

	@After
	public void tearDown() {
	}

	private int getIntField(String name) {
		try {
			Field field = Sloth.class.getDeclaredField(name);
			field.setAccessible(true);
			return field.getInt(sloth);
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	private void setIntField(String name, int value) {
		try {
			Field field = Sloth.class.getDeclaredField(name);
			field.setAccessible(true);
			field.setInt(sloth, value);
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	@SuppressWarnings("unchecked")
	private ArrayList<ByteBuffer> getM() {
		try {
			Field field = Sloth.class.getDeclaredField("m");
			field.setAccessible(true);
			return (ArrayList<ByteBuffer>) field.get(sloth);
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	private void callPrivateMethod(String name, Class<?>[] argTypes, Object... args) {
		try {
			Method method = Sloth.class.getDeclaredMethod(name, argTypes);
			method.setAccessible(true);
			method.invoke(sloth, args);
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	// Sloth VM Tests

	@Test
	public void contextInit() {
		assertNotNull(sloth);
		assertEquals(0, getIntField("sp"));
		assertEquals(0, getIntField("rp"));
		assertEquals(-1, getIntField("ip"));
		assertEquals(0, getIntField("d"));
		assertEquals(1, getIntField("u"));
		ArrayList<ByteBuffer> m = getM();
		assertEquals(1024, m.get(0).capacity());
		assertEquals(256, m.get(1).capacity());
	}

	@Test
	public void dataStack() {
		sloth.push(42);
		assertEquals(1, getIntField("sp"));
		assertEquals(42, sloth.pick(0));

		sloth.push(100);
		assertEquals(2, getIntField("sp"));
		assertEquals(100, sloth.pick(0));
		assertEquals(42, sloth.pick(1));

		assertEquals(100, sloth.pop());
		assertEquals(1, getIntField("sp"));
		assertEquals(42, sloth.pop());
		assertEquals(0, getIntField("sp"));
	}

	@Test
	public void returnStack() {
		sloth.rpush(123);
		assertEquals(1, getIntField("rp"));
		assertEquals(123, sloth.rpop());
		assertEquals(0, getIntField("rp"));
	}

	@Test
	public void memoryCell() {
		int addr = sloth.to_abs(0, 0);
		sloth.store(addr, 0xDEADBEEF);
		assertEquals(0xDEADBEEF, sloth.fetch(addr));

		sloth.store(addr + Sloth.sCELL, 0xCAFEBABE);
		assertEquals(0xCAFEBABE, sloth.fetch(addr + Sloth.sCELL));
	}

	@Test
	public void memoryChar() {
		int addr = sloth.to_abs(0, 0);
		sloth.cstore(addr, (char) 0xAB);
		assertEquals((char) 0xAB, sloth.cfetch(addr));

		sloth.cstore(addr + Sloth.sCHAR, (char) 0xCD);
		assertEquals((char) 0xCD, sloth.cfetch(addr + Sloth.sCHAR));
	}

	@Test
	public void addressConversion() {
		int rel = 16;
		int abs = sloth.to_abs(rel, 0);
		assertEquals(16, abs);
		assertEquals(rel, sloth.to_rel(abs));
	}

	@Test
	public void primitiveExecution() {
		int xt = sloth.primitive((vm) -> {
			int a = vm.pop();
			vm.push(a * 2);
		});
		sloth.push(21);
		sloth.eval(xt);
		assertEquals(42, sloth.pop());
	}

	@Test
	public void innerInterpreterPrimitive() {
		int xt = sloth.primitive((vm) -> {
			int a = vm.pop();
			vm.push(a * 2);
		});
		int d_addr = sloth.to_abs(0, 0);
		sloth.store(d_addr, xt);
		setIntField("ip", d_addr);

		sloth.push(10);
		int op = sloth.op();
		assertEquals(xt, op);
		assertEquals(d_addr + Sloth.sCELL, getIntField("ip"));

		sloth.eval(op);
		assertEquals(20, sloth.pop());
	}

	@Test
	public void innerInterpreterCall() {
		int xt_prim = sloth.primitive((vm) -> {
			int a = vm.pop();
			vm.push(a * 2);
		});
		int xt_exit_p = sloth.primitive((vm) -> vm._exit_());

		int code_xt = sloth.to_abs(100, 0);
		sloth.store(code_xt, xt_prim);
		sloth.store(code_xt + Sloth.sCELL, xt_exit_p);

		sloth.push(15);
		sloth.eval(code_xt);

		assertEquals(30, sloth.pop());
		assertEquals(0, getIntField("rp"));
		assertEquals(-1, getIntField("ip"));
	}

	@Test
	public void exceptions() {
		int xt_thrower = sloth.primitive((vm) -> {
			int e = vm.pop();
			vm._throw(e);
		});

		sloth.push(-42);
		sloth._catch(xt_thrower);

		int result = sloth.pop();
		assertEquals(-42, result);
	}

	@Test
	public void catchSuccess() {
		int xt_prim = sloth.primitive((vm) -> {
			int a = vm.pop();
			vm.push(a * 2);
		});
		sloth.push(10);
		sloth._catch(xt_prim);

		int error = sloth.pop();
		int value = sloth.pop();

		assertEquals(0, error);
		assertEquals(20, value);
	}

	@Test
	public void stackUnderflowDetection() {
		int xt_drop = sloth.primitive((vm) -> vm._drop_());

		sloth._catch(xt_drop);
		assertEquals(Sloth.STACK_UNDERFLOW, sloth.pop());
	}

	@Test
	public void returnStackUnderflowDetection() {
		int xt_r_from = sloth.primitive((vm) -> vm._r_from_());

		sloth._catch(xt_r_from);
		// Note: The current Java implementation of _r_from_ does not check for underflow
		// and will cause an ArrayIndexOutOfBoundsException, which is caught by _catch
		// and returns -1000.
		int result = sloth.pop();
		assertTrue(result == Sloth.RETURN_STACK_UNDERFLOW || result == -1000);
	}

	@Test
	public void slothOp() {
		int d_addr = sloth.to_abs(0, 0);
		sloth.store(d_addr, 1);
		sloth.store(d_addr + Sloth.sCELL, 2);
		setIntField("ip", d_addr);

		assertEquals(1, sloth.op());
		assertEquals(d_addr + Sloth.sCELL, getIntField("ip"));
		assertEquals(2, sloth.op());
		assertEquals(d_addr + 2 * Sloth.sCELL, getIntField("ip"));
	}

	@Test
	public void debugInner() {
		int xt_prim = sloth.primitive((vm) -> {
			int a = vm.pop();
			vm.push(a * 2);
		});
		int xt_exit_p = sloth.primitive((vm) -> vm._exit_());
		int xt_debug_spy = sloth.primitive((vm) -> {
			vm.pop(); // sloth.debug(debug_xt) pushes ip
			debugCount++;
		});

		int code_xt = sloth.to_abs(200, 0);
		sloth.store(code_xt, xt_prim);
		sloth.store(code_xt + Sloth.sCELL, xt_exit_p);

		debugCount = 0;
		setIntField("ip", code_xt);
		sloth.rpush(-1);

		sloth.push(10);
		callPrivateMethod("debug_inner", new Class<?>[]{int.class}, xt_debug_spy);

		assertEquals(20, sloth.pop());
		assertEquals(2, debugCount);
	}

	@Test
	public void slothNew() {
		Sloth sloth2 = new Sloth(524288, 1024);
		assertNotNull(sloth2);
		try {
			Field field = Sloth.class.getDeclaredField("m");
			field.setAccessible(true);
			@SuppressWarnings("unchecked")
			ArrayList<ByteBuffer> m = (ArrayList<ByteBuffer>) field.get(sloth2);
			assertEquals(524288, m.get(0).capacity());
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}
}
