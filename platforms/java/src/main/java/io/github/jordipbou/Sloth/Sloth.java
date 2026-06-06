// Ideas:
// * linenumber in _include_ gets out of phase with real line
//   numbers if REFILL is called from inside Forth code.
//   Create a variable LINENUMBER in user area and update it
//   when doing a REFILL from a FILE. On opening a new file,
//   just set it to 0.
// * The main problem seems related to chars = 2 bytes in
//   ANS code. Ask claude to help there.

package io.github.jordipbou.Sloth;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.io.File;
import java.io.RandomAccessFile;
import java.io.IOException;
import java.util.function.Consumer;

public class Sloth {
	static final int KEY_BACKSPACE;
	static final int KEY_ENTER;
	
	static {
	    String os = System.getProperty("os.name").toLowerCase();
	    KEY_BACKSPACE = os.contains("win") ? 8 : 127;
			KEY_ENTER = os.contains("win") ? 13 : 10;  // \r on Windows, \n on Linux/Mac
 	}

	// -- Virtual Machine constants ------------------------

	public static final int suCHAR = 2;
	public static final int sCELL = 4;
	public static final int CELL_BITS = 32;
	public static final int hCELL_MASK = 0xFFFF;
	public static final int hCELL_BITS = 16;
	public static final int sFCELL = 8;
	public static final int sSFCELL = 4;
	public static final int sDFCELL = 8;

	protected static final int STACK_SIZE = 64;
	protected static final int RETURN_STACK_SIZE = 64;
	protected static final int FLOAT_STACK_SIZE = 64;
	protected static final int OBJECT_STACK_SIZE = 64;

	// -- Virtual Machine context definition ---------------

	protected ByteBuffer[] m; // "Continuous memory"

	protected int d; // Index of the dictionary in m
	protected int u; // Index of the user area in m
	protected int ts; // Index of temporal string area in m

	protected int s[]; // Data stack
	protected int sp; // Data stack pointer
	protected int r[]; // Return stack
	protected int rp; // Return stack pointer
	protected double f[]; // Floating point stack
	protected int fp; // Floating point stack pointer

	protected int ip; // Instruction pointer

	protected int ep; // Exception stack pointer

	protected Object[] o; // Indexed array of objects

	protected ArrayList<Consumer<Sloth>> p; // Array of primitives

	// This array was created by Gemini for the included
	// implementation, I must re-check it.
	private ArrayList<java.io.RandomAccessFile> openFiles = new ArrayList<>();

	public static final int STACK_OVERFLOW = -3;
	public static final int STACK_UNDERFLOW = -4;
	public static final int RETURN_STACK_OVERFLOW = -5;
	public static final int RETURN_STACK_UNDERFLOW = -6;

	// -- Displacement of counted string buffer from here --

	// TODO CBuffer could be just another space of the normal
	// strings circular buffer.
	public static final int CBUF = 64;

	// -- Dictionary variables -----------------------------

	public static final int HERE = 0;
	public static final int INTERNAL_WL = 1*sCELL;
	public static final int FORTH_WL = 2*sCELL;

	// -- User area variables and buffers ------------------
	
	public static final int CURRENT = 0*sCELL;
	public static final int ORDER = 1*sCELL;
	public static final int LOCALS_WORDLIST = 2*sCELL;
	public static final int CONTEXT = 3*sCELL;
	// There are 16 CELLS reserved to search order
	public static final int BASE = 19*sCELL;
	public static final int STATE = 20*sCELL;
	public static final int IBUF = 21*sCELL;
	public static final int IPOS = 22*sCELL;
	public static final int ILEN = 23*sCELL;
	public static final int SOURCE_ID = 24*sCELL;
	public static final int SOURCE_POS = 25*sCELL;
	public static final int LATESTXT = 26*sCELL;
	public static final int INTERPRET = 27*sCELL;
	
	public static final int ROOT_PATH_LENGTH = 28*sCELL;
	public static final int PATH_START = 29*sCELL;
	public static final int PATH_END = 30*sCELL;
	// Continuous space to store path strings, PATHS is just
	// an address inside user area, not a value, so it must
	// be used as to_abs(PATHS, u)
	public static final int PATHS = 31*sCELL;
	
	// Space between PATHS and INCLUDED_FILES
	// reserved to store paths.
	
	public static final int INCLUDED_FILES = 95*sCELL;
	
	public static final int LAST_USER_VAR = 96*sCELL;
	
	// -- Flags for word status ----------------------------
	
	public static final int HIDDEN = 1;
	public static final int IMMEDIATE = 2;

	// -- Context initialization --------------------------

	public Sloth() { this(524288, 1024, 1024); }
	public Sloth(int dsize, int usize) { this(dsize, usize, 1024); }
	public Sloth(int dsize, int usize, int osize) {
		s = new int[STACK_SIZE];
		sp = 0;
		r = new int[RETURN_STACK_SIZE];
		rp = 0;
		f = new double[FLOAT_STACK_SIZE];
		fp = 0;

		ip = -1;
		ep = 0;
		p = new ArrayList<Consumer<Sloth>>();

		// Objects array
		o = new Object[osize];

		// There are only 8 bits for memory blocks, so no
		// more than 256 simultaneous memory blocks will
		// exist.
		m = new ByteBuffer[256];
		d = putByteBuffer(ByteBuffer.allocate(dsize));
		u = putByteBuffer(ByteBuffer.allocate(usize));
		ts = putByteBuffer(ByteBuffer.allocate(2048));

		// Initialize HERE
		m[d].putInt(0*sCELL, 3*sCELL);
		// Initialize INTERNAL-WORDLIST
		m[d].putInt(1*sCELL, 0);
		// Initialize FORTH-WORDLIST (the default wordlist) */
		m[d].putInt(2*sCELL, 0);

		m[u].putInt(0*sCELL, to_abs(FORTH_WL)); // CURRENT
		m[u].putInt(1*sCELL, 2); // #ORDER
		m[u].putInt(2*sCELL, 0); // LOCALS-WORDLIST
		m[u].putInt(3*sCELL, to_abs(FORTH_WL)); // CONTEXT 0
		m[u].putInt(4*sCELL, to_abs(INTERNAL_WL)); // CONTEXT 
}

	// -- Data stack

	void push(int v) { s[sp++] = v; }
	int pop() { return s[--sp]; }
	int pick(int a) { return s[sp - a - 1]; }

	// Helpers for double numbers
	long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }
	/* Long pop -- takes a 32 bit CELL as a 64 bit value; */
	long lpop() { return (long)s[--sp]; }
	/* Unsigned long pop -- takes a signed 32 bit CELL as */
	/* an unsigned 64 bit value. */
	long upop() { return Integer.toUnsignedLong(s[--sp]); }
	/* Unsigned double pop -- takes two 32 bit CELLs as */
	/* a unsigned 64 bit value. */
	/* TODO: Seems to be of no use, check and remove if */
	/* necessary, or maintain it to be used by other classes. */
	long udpop() { long v = upop(); return msp(v << 32) + lsp(upop()); }
	void dpush(long v) { 
		push((int)lsp(v)); 
		push((int)(v >> 32)); 
	}
	long dpop() { 
		long v = lpop(); 
		return msp(v << 32) + lsp(lpop());
	}

	// -- Return stack

	void rpush(int v) { r[rp++] = v; }
	int rpop() { return r[--rp]; }
	int rpick(int a) { return r[rp - a - 1]; }

	// -- Float stack

	void f_push(double v) { f[fp++] = v; }
	double f_pop() { return f[--fp]; }
	double f_pick(int a) { return f[fp - a - 1]; }

	// -- Memory

	// The C implementation of SLOTH uses absolute addresses
	// that allow access to all the memory. That also allows
	// the use of several memory blocks (dictionaries).
	// To do that in Java, memory addresses consist of two parts:
	// <block index> - 8 bits
	// <byte address> - 24 bits

	// Convert between absolute and relative addresses

	int to_abs(int a, int b) { return (b << 24) + a; }
	int to_abs(int a) { return (d << 24) + a; }
	int to_rel(int a) { return a & 0x00FFFFFF; }
	ByteBuffer block(int a) { return m[a >> 24]; }

	void b_store(int a, byte v) { block(a).put(to_rel(a), v); }
	byte b_fetch(int a) { return block(a).get(to_rel(a)); }
	void c_store(int a, char v) { block(a).putChar(to_rel(a), v); }
	char c_fetch(int a) { return block(a).getChar(to_rel(a)); }
	void store(int a, int v) { block(a).putInt(to_rel(a), v); }
	int fetch(int a) { return block(a).getInt(to_rel(a)); }
	void f_store(int a, double v) { block(a).putDouble(to_rel(a), v); }
	double f_fetch(int a) { return block(a).getDouble(to_rel(a)); }
	void s_f_store(int a, float v) { block(a).putFloat(to_rel(a), v); }
	float s_f_fetch(int a) { return block(a).getFloat(to_rel(a)); }
	void d_f_store(int a, double v) { block(a).putDouble(to_rel(a), v); }
	double d_f_fetch(int a) { return block(a).getDouble(to_rel(a)); }

	// Helpers to use Java Strings with Sloth API

	// Memory block 2 is used as a transient memory to copy
	// Java Strings to it. No need to delete them, this will
	// act as a circular buffer and overwrite previous strings
	// as required.
	int fromString(String s) {
		if (m[ts].remaining() / suCHAR < s.length()) m[ts].rewind();
		int addr = to_abs(m[ts].position(), ts);
		for (int i = 0; i < s.length(); i++) {
			m[ts].putChar(s.charAt(i));
		}
		return addr;
	}

	String toString(int a, int l) {
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < l; i++) sb.append(c_fetch(a + i*suCHAR));
		return sb.toString();
	}

	// Helpers to use Java ByteBuffers as memory with Sloth API

	int putByteBuffer(ByteBuffer b) {
		for (int i = 0; i < 256; i++) {
			if (m[i] == null) {
				m[i] = b;
				return i;
			}
		}
		// No free slots
		return -1;
	}

	void removeByteBuffer(int i) {
		m[i] = null;
	}

	// Helpers to use Java Objects with Sloth API

	int putObject(Object obj) {
		// 0 is reserved because SOURCE_ID = 0 means user
		// input device and having a file with index 0 will
		// make it complicated.
		for (int i = 1; i < o.length; i++) {
			if (o[i] == null) {
				o[i] = obj;
				return i;
			}
		}
		// No free slots
		return -1;
	}

	void removeObject(int i) {
		o[i] = null;
	}

	// -- Inner interpreter

	int op() { int o = fetch(ip); ip += sCELL; return o; }
	double f_op() { double n = f_fetch(ip); ip += sFCELL;	return n; }
	protected void do_prim(int q) { p.get(-1 - q).accept(this); }
	protected void call(int q) { 
		if (ip >= 0 || rp > 0) rpush(ip); 
		ip = q; 
	}
	protected void execute(int q) { 
		if (q < 0) do_prim(q); 
		else call(q); 
	}
	protected void inner() { 
		int t = rp; 
		while (t <= rp && ip >= 0) {
			execute(op()); 
		}
	}
	public void eval(int q) { execute(q); if (q > 0) inner(); }

// -- Tracing interpreter

	protected void debug(int debug_xt) { push(ip); eval(debug_xt); }
	protected void debug_inner(int debug_xt) {
		int t = rp;
		while (t <= rp && ip >= 0) {
			debug(debug_xt);
			execute(op());
		}
	}
	protected void _debug_() {
		int post_xt = pop(); 
		int inner_xt = pop();
		int pre_xt = pop();
		int q = pop();
		debug(pre_xt);
		execute(q);
		if (q > 0) debug_inner(inner_xt);
		debug(post_xt);
	}


	// -- Exceptions

	// This implementation of Forth exceptions allows its use
	// both from Forth code and from Java code.
	// It just reuses the Java exception system and implements
	// an Sloth exception as a RuntimeException,
	// not needing to declare it on every method.

	public class SlothException extends RuntimeException {
		public int value;

		public SlothException(int v) { value = v; }
	}

	public void _catch(int q) {
		int tsp = sp;
		int trp = rp;
		int tip = ip;
		ep = ep + 1;
		try {
			eval(q);
			push(0);
		} catch(SlothException x) {
			sp = tsp;
			rp = trp;
			ip = tip;
			push(x.value);
		} catch(Exception e) {
			// Here we catch every possible exception (not just the
			// SlothExceptions) to not break the interactive 
			// environment. Its up to the user to decide if the
			// exception is sever enough to restart the system.
			e.printStackTrace();
			sp = tsp;
			rp = trp;
			push(-1000);
		}
		ep = ep - 1;
	}

	public void _throw(int v) {
		if (v != 0) {
			// Print only if there is no exception frame
			// in the exception stack.
			if (ep == 0) {
				System.out.printf("EXCEPTION: %d\n", v);
				System.out.printf("BUFFER: %s\n", toString(user_get(IBUF), user_get(ILEN)));
				System.out.printf("TOKEN: %s\n", toString(user_get(IBUF) + (user_get(IPOS)*suCHAR), user_get(ILEN) - user_get(IPOS)));
			}
			throw new SlothException(v); 
		}
	}

	// == Forth Kernel
	
	// -- Helpers

	// Dictionary set/get
	void set(int a, int v) { store(a, v); }
	int get(int a) { return fetch(a); }

	void c_set(int a, char v) { c_store(a, v); }
	char c_get(int a) { return c_fetch(a); }

	// User area set/get.
	void user_set(int rel_a, int v) { 
		m[u].putInt(to_rel(rel_a), v);
	}
	int user_get(int rel_a) { 
		return m[u].getInt(to_rel(rel_a));
	}

	// Memory management
	int here() { return get(HERE); }
	void allot(int v) { set(HERE, here() + v); }
	int aligned(int a, int sz) { return ((a + (sz - 1)) & ~(sz - 1)); }
	int aligned(int a) { return aligned(a, sCELL); }
	void _align_() { set(HERE, aligned(here())); }

	// Compilation
	void comma(int v) { store(here(), v); allot(sCELL); }
	void c_comma(char v) { c_store(here(), v); allot(suCHAR); }
	void f_comma(float v) { f_store(here(), v); allot(sFCELL); }
	void compile(int xt) { comma(xt); }
	void literal(int n) { 
		comma(get_xt(find_word("(LIT)")));
		comma(n);
	}
	void f_literal(float f) {
		comma(get_xt(find_word("(FLIT)")));
		f_comma(f);
	}

	// Headers
	int get_latest() { return fetch(user_get(CURRENT)); }
	void set_latest(int w) { store(user_get(CURRENT), w); }
	int get_link(int w) { return fetch(w); }
	int get_xt(int w) { return fetch(w + sCELL); }
	void set_xt(int w, int xt) { store(w + sCELL, xt); }
	char get_flags(int w) { return c_fetch(w + 2*sCELL); }
	void set_flags(int w, int v) { c_store(w + 2*sCELL, (char)v); }
	boolean has_flag(int w, int v) { return (get_flags(w) & v) == v; }
	void set_flag(int w, int v) { 
		c_store(w + 2*sCELL, (char)(get_flags(w) | v)); 
	}
	void unset_flag(int w, int v) {
		c_store(w + 2*sCELL, (char)(get_flags(w) & ~v));
	}
	char get_namelen(int w) { return c_fetch(w + 2*sCELL + suCHAR); }
	int get_name_addr(int w) { return w + 2*sCELL + 2*suCHAR; }

	// Header structure:
	// Link (CELL) @ NT
	// XT (CELL) @ NT + sCELL
	// Flags (CHAR) @ NT + 2*sCELL
	// Name_length (CHAR) @ NT + 2*sCELL + suCHAR
	// Name (CHAR*namelen) @ NT + 2*sCELL + 2*suCHAR

	int header(int n, int l) {
		_align_();
		int w = here();
		comma(get_latest());
		set_latest(w);
		comma(0);
		c_comma((char)0);
		c_comma((char)l);
		for (int i = 0; i < l; i++) c_comma(c_fetch(n + i*suCHAR));
		_align_();
		store(w + sCELL, here());
		return w;
	}

	int header(String n) { return header(fromString(n), n.length()); }

	// -- Primitives ----------------------------------------

	void _exit_() { ip = (rp > 0) ? rpop() : -1; }
	void _lit_() { push(op()); }
	void _rip_() { int tip = ip; int o = op(); push(tip + o - sCELL); }
	void _f_lit_() { f_push(f_op()); }
	void _branch_() { ip += op() ; }
	void _zbranch_() { ip += pop() == 0 ? op() : sCELL ; }
	void _string_() { 
		int l = op(); 
		push(ip); 
		push(l); 
		ip = aligned(ip + (l + 1) * suCHAR); 
	}
	void _c_string_() {
		char l = c_fetch(ip);
		push(ip);
		ip = aligned(ip + (l + 2) * suCHAR);
	}
	// Quotations (not in ANS Forth yet)
	void _quotation_() { int d = op(); push(ip); ip += d; }
	void _start_quotation_() {
		int state = user_get(STATE);
		user_set(STATE, state <= 0 ? state - 1 : state + 1);
		if (user_get(STATE) == -1) push(here() + 2*sCELL);
		push(user_get(LATESTXT));
		compile(get_xt(find_word("(QUOTATION)")));
		push(here());
		comma(0);
		user_set(LATESTXT, here());
	}
	void _end_quotation_() {
		int s = user_get(STATE);
		int a = pop();
		compile(get_xt(find_word("EXIT")));
		store(a, here() - a - sCELL);
		user_set(LATESTXT, pop());
		user_set(STATE, s < 0 ? s + 1 : s - 1);
	}

	// Environment queries
	void _environment_() {
		switch (pop()) {
		case 0: push(64); break; // /COUNTED-STRING
		case 1: break; // TODO /HOLD
		case 2: break; // TODO /PAD
		case 3: push(8); break; // ADDRESS-UNIT-BITS
		case 4: push((-3 / 2 == -2) ? -1 : 0); break; // FLOORED
		case 5: push(Character.MAX_VALUE); break; // MAX-CHAR
		case 6: break; // TODO MAX-D
		case 7: break; // TODO MAX-N
		case 8: break; // TODO MAX-U
		case 9: break; // TODO MAX-UD
		case 10: push(RETURN_STACK_SIZE); break; // RETURN-STACK-CELLS
		case 11: push(STACK_SIZE); break; // STACK-CELLS
		case 12: push(-1); break; // push(FLOAT_STACK_SIZE); break; // FLOATING-STACK
		// Obsolescent queries (required for tests)
		case 100: push(-1); break;
		// Non standard queries
		case -2: push(KEY_ENTER); break;
		case -3: push(KEY_BACKSPACE); break;
		}
	}

	// -- Input/output and parsing operations ------------

	// The region to store WORD generated counted strings
	// starts at "HERE" + CBUF
	public void _word_() {
		char c = (char)pop();
		int ibuf = user_get(IBUF);
		int ilen = user_get(ILEN);
		int ipos = user_get(IPOS);
		// First, ignore c until not c is found
		// As per the Forth standard, if the control character is the
		// space (hex 20) then control characters (< 32) may be treated
		// as delimiters.
		if (c == 32) {
			while (ipos < ilen && c_fetch(ibuf + (ipos*suCHAR)) <= c) ipos++;
		} else {
			while (ipos < ilen && c_fetch(ibuf + (ipos*suCHAR)) == c) ipos++;
		}
		int start = ibuf + (ipos*suCHAR);
		// Next, continue parsing until c is found again
		if (c == 32) {
			while (ipos < ilen && c_fetch(ibuf + (ipos*suCHAR)) > c) ipos++;
		} else {
			while (ipos < ilen && c_fetch(ibuf + (ipos*suCHAR)) != c) ipos++;
		}
		int end = ibuf + (ipos*suCHAR);
		// Now, copy the length and the string to the 
		// counted string buffer
		c_store(here() + CBUF, (char)((end - start) / suCHAR));
		for (int i = 0; i < ((end - start) / suCHAR); i++) {
			c_store(
				here() + CBUF + suCHAR + i*suCHAR, 
				c_fetch(start + i*suCHAR));
		}
		push(here() + CBUF);
		// If not at the end of the input buffer,
		// skip c after the word, but don't treat it as part
		// of the counted string.
		if (ipos < ilen) ipos++;
		user_set(IPOS, ipos);
	}

	public void _file_position_() {
		try {
			RandomAccessFile file = (RandomAccessFile)(o[pop()]);
			if (file != null) {
				try {
					long pos = file.getFilePointer();
					dpush(pos);
					push(0);
				} catch (IOException e) {
					dpush(0);
					push(-37);
				}
			} else {
				dpush(0);
				push(-37);
			}
		} catch (ClassCastException e) {
			// TODO
			e.printStackTrace();
		}
	}

	// As this implementation uses 2 bytes characters, it
	// reads a byte stream from file but stores it as 2 bytes
	// characters in memory.
	public void _read_line_() {
		try {
			RandomAccessFile file = (RandomAccessFile)(o[pop()]);	
			int u1 = pop();
			int caddr = pop();
			try {
				if (file.getFilePointer() >= file.length()) {
					push(0);
					push(0);
					push(0);
				} else {
					String buf = file.readLine();
					int i;
					for (i = 0; i < buf.length() && i < u1; i++) {
						c_store(caddr + i*suCHAR, buf.charAt(i));
					}
					push(i);
					push(-1);
					push(0);
				}
			} catch (IOException e) {
				// TODO
				e.printStackTrace();
			}
		} catch (ClassCastException e) {
			// TODO
			e.printStackTrace();
		}
	}

	public void _refill_() {
		int source_id = user_get(SOURCE_ID);
		switch (source_id) {
			case -1:
				push(0);
				break;
			case 0:
				push(user_get(IBUF));
				push(80);
				eval(get_xt(find_word("ACCEPT")));
				user_set(ILEN, pop());
				user_set(IPOS, 0);
				push(-1);
				break;
			default:
				/* File position is stored to go back to it when */
				/* exiting of nesting includes */
				push(source_id);
				_file_position_();
				pop();
				user_set(SOURCE_POS, (int)dpop());

				/* REFILL can only be called after INCLUDE/INCLUDED */
				/* that means that the line buffer of _included_ will */
				/* be used and its size is known and fixed. */
				push(user_get(IBUF));
				push(1024);
				push(source_id);
				_read_line_();

				int ior = pop();
				int flag = pop();

				if (flag != 0 && ior == 0) {
					user_set(ILEN, pop());
					user_set(IPOS, 0);
					push(-1);
					// Uncomment to debug
					// TODO Add current linenumber as some type of 
					// variable to be able to access it later
					// System.out.printf("REFILL: %s ", toString(user_get(IBUF), user_get(ILEN)));
					// System.out.printf("<%d> ", sp);
					// for (int i = 0; i < sp; i++) {
					// 	System.out.printf("%d ", s[i]);
					// }
					// System.out.printf("\n");
					// ---

				} else {
					pop();
					push(0);
				}
				break;
		}
	}

	void _save_input_() {
		push(user_get(SOURCE_POS));
		push(user_get(SOURCE_ID));
		push(user_get(IBUF));
		push(user_get(IPOS));
		push(user_get(ILEN));
		push(5);
	}
	
	void _restore_input_() {
		pop();
		user_set(ILEN, pop());
		user_set(IPOS, pop());
		user_set(IBUF, pop());
		user_set(SOURCE_ID, pop());
		user_set(SOURCE_POS, pop());
		if (user_get(SOURCE_ID) > 0) {
			int sourceId = user_get(SOURCE_ID);
			if (sourceId <= openFiles.size()) {
				java.io.RandomAccessFile raf = openFiles.get(sourceId - 1);
				try {
					raf.seek(user_get(SOURCE_POS));
					String line = raf.readLine();
					if (line != null) {
						int ibuf = user_get(IBUF);
						ByteBuffer b = block(ibuf);
						int rel = to_rel(ibuf);
						for (int i = 0; i < line.length(); i++) {
							b.putChar(rel + (i * suCHAR), line.charAt(i));
						}
					}
				} catch (java.io.IOException e) {}
			}
		}
		push(0);
	}

	protected void save_input_and_path() {
		_save_input_();
		push(user_get(PATH_START));
		push(user_get(PATH_END));
		for (int i = 0; i < 8; i++) _to_r_();
	}
	
	protected void restore_input_and_path() {
		for (int i = 0; i < 8; i++) _r_from_();
		user_set(PATH_END, pop());
		user_set(PATH_START, pop());
		_restore_input_();
		pop();
	}

	protected RandomAccessFile open_included_file(String name) throws IOException {
		// Variables for working with path, initialized to
		// reuse current path if possible.
		int pathstart = user_get(PATH_START);
		int pathend = user_get(PATH_END);

		RandomAccessFile f;
		String updated_path = "";
		try {
			// Absolute or relative to current directory
			f = new RandomAccessFile(name, "r");
			updated_path = name;
			pathstart = pathend;
		} catch (IOException e) {
			try {
				// Try relative to last included directory
				updated_path = name;
				f = 
					new RandomAccessFile(
						toString(
							pathstart, 
							(pathend - pathstart)/suCHAR
						).concat(name)
					, "r");
				pathend = pathend + name.length()*suCHAR;
			} catch (IOException ie) {
				try {
					// Try relative to ROOT path
					updated_path = 
						toString(
							to_abs(PATHS, u), 
							user_get(ROOT_PATH_LENGTH)
						).concat(name);
					f = new RandomAccessFile(updated_path, "r");
				} catch (IOException ie2) {
					throw ie2;
				}
			}
		}

		// Update path to allow opening files that their paths
		// are relative to a previous opened file (in a nested way).

		// Remove the filename from the updated path string
		updated_path = 
			updated_path.substring(
				0, 
				Math.max(
					updated_path.lastIndexOf('/'), 
					updated_path.lastIndexOf('\\')
				) + 1);

		for (int i = 0; i < updated_path.length(); i++) {
			c_store(pathend + i*suCHAR, updated_path.charAt(i));
		}
		pathend = pathend + updated_path.length()*suCHAR;

		user_set(PATH_START, pathstart);
		user_set(PATH_END, pathend);

		return f;
	}

	protected void add_to_included_files_list(String name) {
			/* TODO Check if this file has been included before, */
			/* and in that case don't add it to the linked list. */
			int here = here();
			comma(user_get(INCLUDED_FILES));
			user_set(INCLUDED_FILES, here);
			comma(name.length());
			for (int i = 0; i < name.length(); i++) {
				c_comma(name.charAt(i));
			}
			_align_();
	}
	
	/* TODO INCLUDED can not be implemented in ANS Forth because */
	/* its needed to include ans.4th itself. */
	/* But this function is very complex and that will make it */
	/* error prone when porting to another language. Try to */
	/* simplify it and use READ-LINE, etc. from Forth */
	
	/* INCLUDED is a complex function because it tries to find */
	/* the indicated file in several directories. */
	/* It first tries to open it as an absolute path/current */
	/* directory. If its not possible to open it, it reuses the */
	/* last path from the previous opened file. */
	public void _included_() {
		int l = pop();
		int a = pop();
		String name = toString(a, l);

		save_input_and_path();

		try {
			RandomAccessFile raf = open_included_file(name);

			int linenumber = 0;
			add_to_included_files_list(name);

			user_set(SOURCE_ID, putObject(raf));
			int buf_idx = putByteBuffer(ByteBuffer.allocate(1024*suCHAR));
			user_set(IBUF, to_abs(0, buf_idx));
			user_set(IPOS, 0);
			user_set(ILEN, 1024);

			do {
				_refill_();
				if (pop() == 0) break;
				// Uncomment to debug included files
				// System.out.printf("INCLUDED: [%d] %s\n", linenumber, toString(user_get(IBUF), user_get(ILEN)));
				// System.out.printf("<%d> ", sp);
				// for (int i = 0; i < sp; i++) {
				// 	System.out.printf("%d ", s[i]);
				// }
				// System.out.printf("\n");
				// ---
				_catch(user_get(INTERPRET));
				int e = pop();
				if (e != 0) {
					int pathstart = user_get(PATH_START);
					int pathend = user_get(PATH_END);
					String path = toString(pathstart, (pathstart - pathend)/suCHAR);
					System.out.printf("File: %s\n", path);
					System.out.printf("Line (%d): %s\n", linenumber, toString(user_get(IBUF), user_get(ILEN)));
					_throw(e);
				}
				linenumber++;
			} while(true);

			raf.close();

			removeByteBuffer(buf_idx);
			removeObject(user_get(SOURCE_ID));

			restore_input_and_path();
		} catch (IOException e) {
			restore_input_and_path();
			_throw(-38);
		}
	}

	// Finding words
	protected boolean compare(int a1, int u1, int a2, int u2) {
		if (u1 != u2) return false;
		for (int i = 0; i < u2; i++) {
			char a = c_fetch(a1 + i*suCHAR);
			char b = c_fetch(a2 + i*suCHAR);
			if (a >= 97 && a <= 122) a -= 32;
			if (b >= 97 && b <= 122) b -= 32;
			if (a != b) return false;
		}
		return true;
	}

	protected int search_word(int n, int l) {
		for (int i = -1; i < user_get(ORDER); i++) {
			int wl = user_get(CONTEXT + i*sCELL);
			if (wl != 0) {
				int w = fetch(wl);
				while (w > 0) {
					if (!has_flag(w, HIDDEN) 
					 && compare(get_name_addr(w), get_namelen(w),	n, l)) {
						return w;
					}
					w = get_link(w);
				}
			}
		}
		return 0;
	}

	public void _find_() {
		int cstring = pop();
		try {
			int w = search_word(cstring + suCHAR, c_fetch(cstring));
			if (w == 0) { push(cstring); push(0); }
			else if (has_flag(w, IMMEDIATE)) { push(get_xt(w)); push(1); }
			else { push(get_xt(w)); push(-1); }
		} catch (Exception e) {
			System.out.printf("WORD SEARCHED:%s:\n", toString(cstring + suCHAR, c_fetch(cstring)));
		}
	}

	// Helper to find words from Java
	public int find_word(String name) {
		return search_word(fromString(name), name.length());
	}

	// -- Outer interpreter

	void _interpret_() {
		int flag;
		while (user_get(IPOS) < user_get(ILEN)) {
			push(32); _word_();
			int tok = pick(0) + suCHAR;
			int tlen = c_fetch(pick(0));
			if (tlen == 0) { pop(); break; } // return; }
			// Debugging
			// System.out.printf("TOKEN: [%d] {%s}\n", tlen, toString(tok, tlen));
			// \Debugging
			_find_();
			if ((flag = pop()) != 0) {
				if (user_get(STATE) == 0 || (user_get(STATE) != 0 && flag == 1)) {
					eval(pop());
				} else {
					compile(pop());
				}
			} else {
				pop();
				if (tlen == 3 
				 && c_fetch(tok) == '\'' 
				 && c_fetch(tok + 2*suCHAR) == '\'') {
					if (user_get(STATE) == 0) {
						push(c_fetch(tok + suCHAR));
					} else {
						literal(c_fetch(tok + suCHAR));
					}
				} else {
					boolean is_double = false;
					int temp_base = user_get(BASE);
					if (c_fetch(tok) == '#') {
						temp_base = 10;
						tlen--;
						tok += suCHAR;
					} else if (c_fetch(tok) == '$') {
						temp_base = 16;
						tlen--;
						tok += suCHAR;
					} else if (c_fetch(tok) == '%') {
						temp_base = 2;
						tlen--;
						tok += suCHAR;
					} else if (c_fetch(tok + tlen*suCHAR - suCHAR) == '.') {
						tlen--;
						is_double = true;
					}
					StringBuffer buf = new StringBuffer();
					for (int i = 0; i < tlen; i++) 
						buf.append(c_fetch(tok +i*suCHAR));
					try {
						long n = Long.parseLong(buf.toString(), temp_base);
						if (user_get(STATE) == 0) {
							push((int)n);
							if (is_double) push(n < 0 ? -1 : 0);
						} else {
							literal((int)n);
							if (is_double) literal(n < 0 ? -1 : 0);
						}
					} catch(NumberFormatException e1) {
						try {
							float r = Float.parseFloat(buf.toString());
							if (user_get(STATE) == 0) {
								f_push(r);
							} else {
								f_literal(r);
							}
						} catch(NumberFormatException e2) {
							_throw(-13);
						}
					}
				}
			}
		}
	}

	// -- Require words to bootstrap

	void _bye_() { System.out.println(); System.exit(0); }
	void _unused_() { push(m[d].capacity() - here()); }
	void _move_() {
		int u = pop();
		int addr2 = pop();
		int addr1 = pop();
		if (addr1 >= addr2) {
			for (int i = 0; i < u; i++) 
				b_store(addr2 + i, b_fetch(addr1 + i));
		} else {
			for (int i = u - 1; i >= 0; i--)
				b_store(addr2 + i, b_fetch(addr1 + i));
		}
	}

	void _emit_() { System.out.printf("%c", (char)pop()); }
	void _key_() { 
		try { push(System.in.read()); } 
		catch(IOException e) { e.printStackTrace(); }
	}

	void _and_() { int v = pop(); push(pop() & v); }
	void _invert_() { push(~pop()); }
	void _l_shift_() { int n = pop(); push(pop() << n); }
	void _minus_() { int n = pop(); push(pop() - n); }
	void _plus_() { int n = pop(); push(pop() + n); }
	void _r_shift_() { int n = pop(); push(pop() >>> n); }
	void _star_() { int n = pop(); push(pop() * n); }
	void _two_slash_() { push(pop() >> 1); }
	void _u_m_star_() { long u = upop() * upop(); dpush(u); }
	void _u_m_slash_mod_() { 
		long u = upop(); 
		long d = dpop();
		push((int)Long.remainderUnsigned(d, u));
		push((int)Long.divideUnsigned(d, u));
	}

	void _c_fetch_() { push(c_fetch(pop())); }
	void _c_store_() { int a = pop(); c_store(a, (char)pop()); }
	void _fetch_() { push(fetch(pop())); }
	void _store_() { int a = pop(); store(a, pop()); }

	void _equals_() { int n = pop(); push(pop() == n ? -1 : 0); }
	void _less_than_() { int n = pop(); push(pop() < n ? -1 : 0); }

	// Defining routines
	void _colon_() {
		push(32); _word_();
		int tok = pick(0) + suCHAR;
		int tlen = c_fetch(pop());
		header(tok, tlen);
		user_set(LATESTXT, get_xt(get_latest()));
		set_flag(get_latest(), HIDDEN);
		user_set(STATE, 1);
	}
	void _colon_no_name_() {
		push(here());
		user_set(LATESTXT, here());
		user_set(STATE, 1);
	}
	void _semicolon_() {
		compile(get_xt(find_word("EXIT")));
		user_set(STATE, 0);
		// Don't change flags for nonames
		if (get_xt(get_latest()) == user_get(LATESTXT)) {
			unset_flag(get_latest(), HIDDEN);
		}
	}
	void _recurse_() { compile(user_get(LATESTXT)); }
	void _catch_() { _catch(pop()); }
	void _throw_() {
		int e = pop();
		if (e != 0) {
			if (e == -2 && ep == 0) {
				// If it's ABORT" print the message only if there
				// is no exception frame on the exception stack.
				int l = pop();
				int a = pop();
				StringBuffer buf = new StringBuffer();
				for (int i = 0; i < l; i++) buf.append(c_fetch(a + i*suCHAR));
				System.out.printf("Error: %s\n", buf.toString());
			}
		}
		_throw(e);
	}

	// Manipulating stack items
	void _drop_() { 
		if (sp <= 0) _throw(STACK_UNDERFLOW); 
		else pop(); }
	void _dup_() { 
		if (sp == 0) _throw(STACK_UNDERFLOW);
		else if (sp == STACK_SIZE) _throw(STACK_OVERFLOW);
		else push(pick(0));
	}
	void _over_() { push(pick(1)); }
	void _to_r_() { rpush(pop()); }
	void _r_from_() { push(rpop()); }
	void _swap_() { 
		if (sp < 2) throw new SlothException(-4);
		int a = pop(); int b = pop(); push(a); push(b); 
	}

	// Constructing compiler and interpreter system extensions
	void _allot_() { allot(pop()); }
	void _cells_() { push(pop() * sCELL); }
	void _chars_() { push(pop() * suCHAR); }
	void _compile_comma_() { compile(pop()); }
	void _create_name_() {
		int tlen = pop();
		header(pop(), tlen);
		compile(get_xt(find_word("(RIP)")));
		compile(4*sCELL);
		compile(get_xt(find_word("EXIT")));
		compile(get_xt(find_word("EXIT")));
	}
	void _create_() {
		push(32); _word_();
		int c = pop();
		push(c + suCHAR);
		push(c_fetch(c));
		_create_name_();
	}
	void do_does(int a) { store(get_xt(get_latest()) + 2*sCELL, a); }
	void _do_does_() { do_does(pop()); }
	void _does_() { 
		literal(here() + 4*sCELL);
		compile(get_xt(find_word("(DOES)")));
		compile(get_xt(find_word("EXIT")));
	}
	void _evaluate_() {
		int l = pop();
		int a = pop();

		int previbuf = user_get(IBUF);
		int previpos = user_get(IPOS);
		int previlen = user_get(ILEN);

		int prevsourceid = user_get(SOURCE_ID);

		user_set(SOURCE_ID, -1);
		user_set(IBUF, a);
		user_set(IPOS, 0);
		user_set(ILEN, l);

		// To ensure that the input buffer is restored correctly
		// even in case of a throw, I catch any possible throw
		// here and rethrow it after restoring the input buffer.
		_catch(user_get(INTERPRET));

		user_set(SOURCE_ID, prevsourceid);
		user_set(IBUF, previbuf);
		user_set(IPOS, previpos);
		user_set(ILEN, previlen);

		int e = pop();
		if (e != 0) _throw(e);
	}
	void _execute_() { eval(pop()); }
	void _here_() { push(here()); }
	void _immediate_() { set_flag(get_latest(), IMMEDIATE); }
	void _postpone_() {
		push(32); _word_();
		int tok = pick(0) + suCHAR;
		int tlen = c_fetch(pick(0));
		if (tlen == 0) { pop(); return; }
		_find_();
		int i = pop();
		int xt = pop();
		if (i == 0) { 
			return;
		} else if (i == -1) {
			literal(xt);
			compile(get_xt(find_word("COMPILE,")));
		} else {
			compile(xt);
		}
	}
	void _source_() { push(user_get(IBUF)); push(user_get(ILEN)); }

	// -- Floating point word set --------------------------
	
	// Constructing compiler and interpreter system extensions
	
	public void _f_align_() { set(HERE, aligned(get(HERE), sFCELL)); }
	public void _f_aligned_() { push(aligned(pop(), sFCELL)); }
	public void _s_f_aligned_() {	push(aligned(pop(), sSFCELL)); }
	public void _d_f_aligned_() { push(aligned(pop(), sDFCELL)); }
	
	public void _floats_() { push(pop() * sFCELL); }
	public void _s_floats_() { push(pop() * sSFCELL); }
	public void _d_floats_() { push(pop() * sDFCELL); }

	// Manipulating stack items

	public void _f_depth_() { push(fp); }
	public void _f_drop_() { f_pop(); }
	public void _f_dup_() { f_push(f_pick(0)); }
	public void _f_over_() { f_push(f_pick(1)); }
	public void _f_rot_() { 
		double c = f_pop();
		double b = f_pop();
		double a = f_pop();
		f_push(b);
		f_push(c);
		f_push(a);
	}
	public void _f_swap_() { 
		double b = f_pop();
		double a = f_pop();
		f_push(b);
		f_push(a);
	}
	
	// Comparison operations
	
	public void _f_less_than_() { 
		double b = f_pop();
		double a = f_pop();
		push(a < b ? -1 : 0);
	}
	public void _f_zero_less_than_() { 
		push(f_pop() < 0.0 ? -1 : 0); 
	}
	public void _f_zero_equals_() {
		push(f_pop() == 0.0 ? -1 : 0);
	}

	// Memory-stack transfer operations
	
	public void _f_fetch_() { f_push(f_fetch(pop())); }
	public void _f_store_() {	f_store(pop(), f_pop()); }
	public void _s_f_fetch_() { f_push(s_f_fetch(pop())); }
	public void _s_f_store_() { s_f_store(pop(), (float)f_pop()); }
	public void _d_f_fetch_() { f_push(d_f_fetch(pop())); }
	public void _d_f_store_() { d_f_store(pop(), f_pop()); }

	// Number-type conversion operators

	public void _d_to_f_() { f_push((double)dpop()); }
	public void _f_to_d_() { dpush((long)f_pop()); }

	// Arithmetic and logical operations

	public void _f_abs_() { f_push(Math.abs(f_pop())); }
	public void _f_plus_() { f_push(f_pop() + f_pop()); }
	public void _f_minus_() { 
		double b = f_pop();
		f_push(f_pop() - b);
	}
	public void _f_star_() { f_push(f_pop() * f_pop()); }
	public void _f_star_star_() {
		double b = f_pop();
		f_push(Math.pow(f_pop(), b));
	}
	public void _f_slash_() {
		double b = f_pop();
		f_push(f_pop() / b);
	}
	public void _floor_() { f_push(Math.floor(f_pop())); }
	public void _f_round_() { f_push(Math.rint(f_pop())); }
	public void _f_max_() { 
		double b = f_pop();
		double a = f_pop();
		f_push(a > b ? a : b);
	}
	public void _f_min_() { 
		double b = f_pop();
		double a = f_pop();
		f_push(a < b ? a : b);
	}
	public void _f_negate_() { f_push(-f_pop()); }
	public void _f_proximate_() {
    double r3 = f_pop();
    double r2 = f_pop();
    double r1 = f_pop();

    if (Double.isNaN(r3)) {
        // r3 is NaN: not proximate
        push(0);
    } else if (r3 > 0.0) {
        // r3 positive: absolute epsilon comparison
        push(Math.abs(r1 - r2) < r3 ? -1 : 0);
    } else if (r3 < 0.0) {
        // r3 negative: relative epsilon comparison
        push(Math.abs(r1 - r2) < (Math.abs(r3) * (Math.abs(r1) + Math.abs(r2))) ? -1 : 0);
    } else {
        // r3 == 0.0: exact bitwise comparison of 
				// IEEE 754 representation.
        // Must use doubleToRawLongBits, 
				// not doubleToLongBits,
        // because doubleToLongBits canonicalizes 
				// all NaNs to one value,
        // which would make distinct NaN bit 
				// patterns compare as equal.
        push(Double.doubleToRawLongBits(r1) == Double.doubleToRawLongBits(r2) ? -1 : 0);
    }
	}
	public void _f_sqrt_() { f_push(Math.sqrt(f_pop())); }
	public void _f_l_n_() { f_push(Math.log(f_pop())); }
	public void _f_exp_() { f_push(Math.exp(f_pop())); }
	public void _f_exp_m_one_() { f_push(Math.exp(f_pop()) - 1.0); }
	public void _f_log_ten_() { f_push(Math.log10(f_pop())); }
	public void _f_l_n_p_one_() { f_push(Math.log(f_pop() + 1.0)); }
	public void _f_a_log_() { f_push(Math.pow(10.0, f_pop())); }

	// Trigonometric functions

	public void _f_sine_() { f_push(Math.sin(f_pop())); }
	public void _f_a_sine_() { f_push(Math.asin(f_pop())); }
	public void _f_cos_() { f_push(Math.cos(f_pop()));	}
	public void _f_a_cos_() { f_push(Math.acos(f_pop())); }
	public void _f_sine_cos_() {
		double r = f_pop();
		f_push(Math.sin(r));
		f_push(Math.cos(r));
	}
	public void _f_tan_() { f_push(Math.tan(f_pop())); }
	public void _f_a_tan_() { f_push(Math.atan(f_pop())); }
	public void _f_atan2_() { 
		double b = f_pop();
		f_push(Math.atan2(f_pop(), b));
	}

	// Hyperbolic functions

	public void _f_sin_h_() { f_push(Math.sinh(f_pop())); }
	public void _f_cos_h_() { f_push(Math.cosh(f_pop())); }
	public void _f_tan_h_() { f_push(Math.tanh(f_pop())); }
	// There are no asinh/acosh functions in Math.
	public void _f_a_sine_h_() {
		double r = f_pop();
		if (r == 0) {
			f_push(0.0);
		} else if (r > 0) {
			f_push(Math.log(r + Math.sqrt(r * r + 1.0)));
		} else {
			f_push(-Math.log(-r + Math.sqrt(r * r + 1.0)));
		}
	}
	public void _f_a_cos_h_() {
		double r = f_pop();
		if (r < 1.0) {
			/* undefined, push NaN */
			f_push(Double.NaN);
		} else {
			f_push(Math.log(r + Math.sqrt(r * r - 1.0)));
		}
	}

	// String/numeric conversion
	
	public void _to_float_() {
		int tlen = pop();
		int tok = pop();
		String s = toString(tok, tlen);
		try {
			// A string with trailing spaces must fail
			if (s.endsWith(" ")) { throw new NumberFormatException(); }
			f_push(Double.parseDouble(s));
			push(-1);
		} catch (NumberFormatException e) {
			// Double.parseDouble will not convert an empty
			// string or a string of blanks to a 0.0, but
			// the Forth standard requires it.
			if (s.trim().isEmpty()) {
				push(-1);
				f_push(0.0);
			} else {
				push(0);
			}
		}
	}

	// -- Helpers for bootstrapping -------------------------

	int primitive(Consumer<Sloth> c) {
		p.add(c);
		return 0 - p.size();
	}
	int code(String name, int xt) {
		int w = header(name);
		set_xt(w, xt);
		return xt;
	}
	void user_variable(String name, int d, int v) {
		int w = header(name);
		set_xt(w, here());
		literal(to_abs(0, u) + d);
		compile(get_xt(find_word("EXIT")));
		store(to_abs(0, u) + d, v);
	}
	void _empty_rs_() { rp = 0; }
	// In C there is _ints_ and _self_, but I don't need it here.

	// Bootstrapping

	void bootstrap() {
		// Basic primitives
		code("EXIT", primitive((vm) -> vm._exit_()));
		code("(LIT)", primitive((vm) -> vm._lit_()));
		code("(RIP)", primitive((vm) -> vm._rip_()));
		code("(BRANCH)", primitive((vm) -> vm._branch_()));
		code("(?BRANCH)", primitive((vm) -> vm._zbranch_()));
		// Define user variables
		user_variable("(CURRENT)", CURRENT, to_abs(FORTH_WL));
		user_variable("#ORDER", ORDER, 2);
		user_variable("(LOCALS-WORDLIST)", LOCALS_WORDLIST, 0);
		user_variable("CONTEXT", CONTEXT, to_abs(FORTH_WL));
		user_variable("BASE", BASE, 10);
		user_variable("STATE", STATE, 0);
		user_variable("(IBUF)", IBUF, 0);
		user_variable(">IN", IPOS, 0);
		user_variable("(ILEN)", ILEN, 0);
		user_variable("(SOURCE-ID)", SOURCE_ID, 0);
		user_variable("(SOURCE-POS)", SOURCE_POS, 0);
		user_variable("(LATESTXT)", LATESTXT, 0);
		user_variable("(INTERPRET)", INTERPRET, primitive((vm) -> vm._interpret_()));

		user_variable("(SLOTH_ROOT_PATH_LENGTH)", ROOT_PATH_LENGTH, 0);
		user_variable("(SLOTH_PATH_START)", PATH_START, to_abs(PATHS, u));
		user_variable("(SLOTH_PATH_END)", PATH_END, to_abs(PATHS, u));
		user_variable("(SLOTH_PATHS)", PATHS, 0);

		user_variable("(INCLUDED-FILES)", INCLUDED_FILES, 0);

		// Data and return stack
		code("DROP", primitive((vm) -> vm._drop_()));
		code("DUP", primitive((vm) -> vm._dup_()));
		code("OVER", primitive((vm) -> vm._over_()));
		code(">R", primitive((vm) -> vm._to_r_()));
		code("R>", primitive((vm) -> vm._r_from_()));
		code("SWAP", primitive((vm) -> vm._swap_()));

		// Memory
		code("C@", primitive((vm) -> vm._c_fetch_()));
		code("C!", primitive((vm) -> vm._c_store_()));
		code("@", primitive((vm) -> vm._fetch_()));
		code("!", primitive((vm) -> vm._store_()));

		code("CELLS", primitive((vm) -> vm._cells_()));
		code("CHARS", primitive((vm) -> vm._chars_()));

		code("HERE", primitive((vm) -> vm._here_()));
		code("ALIGN", primitive((vm) -> vm._align_()));
		code("ALLOT", primitive((vm) -> vm._allot_()));
		code("UNUSED", primitive((vm) -> vm._unused_()));

		// Exceptions
		code("CATCH", primitive((vm) -> vm._catch_()));
		code("THROW", primitive((vm) -> vm._throw_()));

		// Arithmetic and logical operations
		code("INVERT", primitive((vm) -> vm._invert_()));
		code("AND", primitive((vm) -> vm._and_()));
		code("LSHIFT", primitive((vm) -> vm._l_shift_()));
		code("-", primitive((vm) -> vm._minus_()));
		code("+", primitive((vm) -> vm._plus_()));
		code("RSHIFT", primitive((vm) -> vm._r_shift_()));
		code("*", primitive((vm) -> vm._star_()));
		code("2/", primitive((vm) -> vm._two_slash_()));
		code("UM*", primitive((vm) -> vm._u_m_star_()));
		code("UM/MOD", primitive((vm) -> vm._u_m_slash_mod_()));

		// Comparison operations
		code("=", primitive((vm) -> vm._equals_()));
		code("<", primitive((vm) -> vm._less_than_()));

		// Strings
		code("(STRING)", primitive((vm) -> vm._string_()));
		code("(CSTRING)", primitive((vm) -> vm._c_string_()));
		code("MOVE", primitive((vm) -> vm._move_()));

		// Input/output and parsing operations
		code("EMIT", primitive((vm) -> vm._emit_()));
		code("KEY", primitive((vm) -> vm._key_()));
		code("SOURCE", primitive((vm) -> vm._source_()));
		code("WORD", primitive((vm) -> vm._word_()));
		code("REFILL", primitive((vm) -> vm._refill_()));
		code("SAVE-INPUT", primitive((vm) -> vm._save_input_()));
		code("RESTORE-INPUT", primitive((vm) -> vm._restore_input_()));
		code("INCLUDED", primitive((vm) -> vm._included_()));

		// Finding words
		code("FIND", primitive((vm) -> vm._find_()));

		// Quotations
		code("(QUOTATION)", primitive((vm) -> vm._quotation_()));
		code("[:", primitive((vm) -> vm._start_quotation_())); _immediate_();
		code(";]", primitive((vm) -> vm._end_quotation_())); _immediate_();

		// End work session
		code("BYE", primitive((vm) -> vm._bye_()));

		// Defining words
		code(":", primitive((vm) -> vm._colon_()));
		code(":NONAME", primitive((vm) -> vm._colon_no_name_()));
		code(";", primitive((vm) -> vm._semicolon_())); _immediate_();
		code("RECURSE", primitive((vm) -> vm._recurse_())); _immediate_();
		code("IMMEDIATE", primitive((vm) -> vm._immediate_()));
		code("POSTPONE", primitive((vm) -> vm._postpone_())); _immediate_();

		code("COMPILE,", primitive((vm) -> vm._compile_comma_()));
		code("CREATE", primitive((vm) -> vm._create_()));
		code("(DOES)", primitive((vm) -> vm._do_does_()));
		code("DOES>", primitive((vm) -> vm._does_())); _immediate_();

		// Executing
		code("EVALUATE", primitive((vm) -> vm._evaluate_()));
		code("EXECUTE", primitive((vm) -> vm._execute_()));

		// Environment queries
		code("(ENVIRONMENT)", primitive((vm) -> vm._environment_()));

		// Primitives I don't like too much
		code("DICT", primitive((vm) -> vm.push(to_abs(0))));
		code("(EMPTY-RETURN-STACK)", primitive((vm) -> vm.rp = 0));
	}

	void repl() {
		int block = putByteBuffer(ByteBuffer.allocate(80*suCHAR));
		user_set(IBUF, to_abs(0, block));
		user_set(IPOS, 0);
		user_set(ILEN, 80);
		eval(get_xt(find_word("QUIT")));
		removeByteBuffer(block);
	}

	void evaluate(String c) {
		ByteBuffer buf = ByteBuffer.allocate(256);
		buf.clear();
		int bidx = putByteBuffer(buf);
		for(int i = 0; i < c.length(); i++) {
			buf.putChar(c.charAt(i));
		}
		push(to_abs(0, bidx));
		push(c.length());
		_evaluate_();
		removeByteBuffer(bidx);
	}

	int include(String f) {
		push(fromString(f));
		push(f.length());
		_catch(get_xt(find_word("INCLUDED")));
		return pop();
	}

	// --

	void set_root_path(String path) {
		int paths = to_abs(PATHS, u);
		for (int i = 0; i < path.length(); i++) {
			c_store(paths + i*suCHAR, path.charAt(i));	
		}
		user_set(ROOT_PATH_LENGTH, path.length());
		user_set(PATH_START, paths + path.length()*suCHAR);
		user_set(PATH_END, paths + path.length()*suCHAR);
	}
}
