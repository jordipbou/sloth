package io.github.jordipbou.Sloth;

import org.junit.Test;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.After;

import io.github.jordipbou.Sloth.Sloth;

public class SlothTest {
	private Sloth sloth;

	@Before
	public void setUp() {
		sloth = new Sloth(1024, 256);
	}

	@After
	public void tearDown() {
	}

	@Test
	public void dataStack() {
		sloth.push(42);
		assertEquals(42, sloth.pick(0));

		sloth.push(100);
		assertEquals(100, sloth.pick(0));
		assertEquals(42, sloth.pick(1));

		assertEquals(100, sloth.pop());
		assertEquals(42, sloth.pop());
	}

	@Test
	public void returnStack() {
		sloth.rpush(123);
		assertEquals(123, sloth.rpop());
	}
}
