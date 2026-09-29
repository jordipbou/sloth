package io.github.jordipbou.Sloth;

import java.nio.ByteBuffer;

/* Java port of platforms/c/memory.c */

public class Memory {

	/* Allocated memory is handed out as a fresh ByteBuffer, so its
	 * address is (index << 24) + 0: aligned and independent of HERE.
	 * That caps live allocations at 127 (the index shares the sign
	 * bit of the encoded CELL) and each allocation at 16 MB (the
	 * offset is masked to 24 bits by Sloth.to_rel). */

	public static void bootstrap(Sloth x) {

		/* -- Memory-Allocation word set ------------------------- */

		x.code("ALLOCATE", x.primitive(Memory::_allocate_));
		x.code("FREE", x.primitive(Memory::_free_));
		x.code("RESIZE", x.primitive(Memory::_resize_));

		/* -- Special memory access words proposal --------------- */

		x.code("B@", x.primitive(Memory::_b_fetch_));
		x.code("B!", x.primitive(Memory::_b_store_));
		x.code("W@", x.primitive(Memory::_w_fetch_));
		x.code("W!", x.primitive(Memory::_w_store_));
		x.code("L@", x.primitive(Memory::_l_fetch_));
		x.code("L!", x.primitive(Memory::_l_store_));
		x.code("X@", x.primitive(Memory::_x_fetch_));
		x.code("X!", x.primitive(Memory::_x_store_));
	}

	static void _allocate_(Sloth x) {
		if (!x.check_data_stack(1, 2)) return;
		int u = x.pop();
		ByteBuffer b = null;
		if (u >= 0) {
			try {
				b = ByteBuffer.allocate(u);
			} catch (IllegalArgumentException e) {
				b = null;
			} catch (OutOfMemoryError e) {
				b = null;
			}
		}
		int idx = (b == null) ? -1 : x.putByteBuffer(b);
		if (idx < 0) {
			x.push(0);
			x.push(-59);
		} else {
			x.push(x.to_abs(0, idx));
			x.push(0);
		}
	}

	static void _free_(Sloth x) {
		if (!x.check_data_stack(1, 1)) return;
		x.removeByteBuffer(x.pop() >> 24);
		x.push(0);
	}

	static void _resize_(Sloth x) {
		if (!x.check_data_stack(2, 2)) return;
		int u = x.pop();
		int addr = x.pop();
		if (u < 0) {
			x.push(addr);
			x.push(-61);
			return;
		}
		ByteBuffer old = x.block(addr);
		int old_idx = addr >> 24;
		int n = Math.min(old.capacity(), u);
		ByteBuffer nb = null;
		try {
			nb = ByteBuffer.allocate(u);
		} catch (IllegalArgumentException e) {
			nb = null;
		} catch (OutOfMemoryError e) {
			nb = null;
		}
		if (nb == null) {
			x.push(addr);
			x.push(-61);
			return;
		}
		byte[] tmp = new byte[n];
		old.get(0, tmp, 0, n);
		nb.put(0, tmp, 0, n);
		int new_idx = x.putByteBuffer(nb);
		if (new_idx < 0) {
			x.push(addr);
			x.push(-61);
			return;
		}
		x.removeByteBuffer(old_idx);
		x.push(x.to_abs(0, new_idx));
		x.push(0);
	}

	/* -- Special memory access words proposal ------------------- */

	static void _b_fetch_(Sloth x) {
		if (!x.check_data_stack(1, 1)) return;
		int a = x.pop();
		x.push(x.block(a).get(x.to_rel(a)));
	}

	static void _b_store_(Sloth x) {
		if (!x.check_data_stack(2, 0)) return;
		int a = x.pop();
		byte v = (byte)x.pop();
		x.block(a).put(x.to_rel(a), v);
	}

	static void _w_fetch_(Sloth x) {
		if (!x.check_data_stack(1, 1)) return;
		int a = x.pop();
		x.push(x.block(a).getShort(x.to_rel(a)));
	}

	static void _w_store_(Sloth x) {
		if (!x.check_data_stack(2, 0)) return;
		int a = x.pop();
		short v = (short)x.pop();
		x.block(a).putShort(x.to_rel(a), v);
	}

	static void _l_fetch_(Sloth x) {
		if (!x.check_data_stack(1, 1)) return;
		int a = x.pop();
		x.push(x.block(a).getInt(x.to_rel(a)));
	}

	static void _l_store_(Sloth x) {
		if (!x.check_data_stack(2, 0)) return;
		int a = x.pop();
		int v = x.pop();
		x.block(a).putInt(x.to_rel(a), v);
	}

	static void _x_fetch_(Sloth x) {
		if (!x.check_data_stack(1, 1)) return;
		int a = x.pop();
		x.push((int)x.block(a).getLong(x.to_rel(a)));
	}

	static void _x_store_(Sloth x) {
		if (!x.check_data_stack(2, 0)) return;
		int a = x.pop();
		long v = x.pop();
		x.block(a).putLong(x.to_rel(a), v);
	}
}
