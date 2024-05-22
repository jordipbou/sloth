package io.github.jordipbou;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class SlothTest {
	@Test void innerInterpreterEXIT1() {
		Sloth x = new Sloth();
		
		x.ip = 0;
		x.exit();

		assertTrue(x.ip == -1);
	}

	@Test void innerInterpreterEXIT2() {
		Sloth x = new Sloth();

		x.rpush(11);
		x.ip = 13;
		x.exit();

		assertTrue(x.ip == 11);
		assertTrue(x.rp == 0);
	}

	// @Test void bootstrappedPrimitiveEXIT() {
	// 	Sloth x = new Sloth();
	// 	x.bootstrap();

	// 	x.ip = 0;
	// 	x.execute(Sloth.EXIT);

	// 	assertTrue(x.ip == -1);
	// }

	// @Test void fetchAndStore() {
	// 	Sloth x = new Sloth();

	// 	x.store(0, Sloth.EXIT);
	// 	x.store(Sloth.CELL, 11);

	// 	assertTrue(x.fetch(0) == Sloth.EXIT);
	// 	assertTrue(x.fetch(Sloth.CELL) == 11);
	// }

	// @Test void innerInterpreterToken() {
	// 	Sloth x = new Sloth();

	// 	x.store(0, Sloth.EXIT);
	// 	x.ip = 0;
	// 	assertTrue(x.token() == Sloth.EXIT);
	// }

	// @Test void innerInterpreterCompiledEXIT() {
	// 	Sloth x = new Sloth();
	// 	x.bootstrap();

	// 	// TODO I don't know why, but after bootstrapping the value is
	// 	// not correctly stored
	// 	x.store(0, 11);
	// 	assertTrue(x.fetch(0) == 11);

	// 	//x.store(0, Sloth.EXIT);

	// 	// x.ip = 0;
	// 	// x.inner();

	// 	// assertTrue(x.ip == -1);
	// }
}
