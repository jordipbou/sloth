package io.github.jordipbou.Sloth;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.io.File;
import java.io.RandomAccessFile;
import java.io.IOException;
import java.util.function.Consumer;

public class Sloth {

	/* -- Virtual Machine constants ------------------------ */

	public static final int suCHAR = 2;
	public static final int sCELL = 4;
	public static final int CELL_BITS = 32;
	public static final int hCELL_MASK = 0xFFFF;
	public static final int hCELL_BITS = 16;
	public static final int sFCELL = 4;

	protected static final int STACK_SIZE = 64;
	protected static final int RETURN_STACK_SIZE = 64;
	protected static final int FLOAT_STACK_SIZE = 64;
	protected static final int OBJECT_STACK_SIZE = 64;

	/* -- Virtual Machine context definition --------------- */

	protected int d; // Index of the dictionary in m
	protected int u; // Index of the user area in m

	protected int s[]; // Data stack
	protected int sp; // Data stack pointer
	protected int r[]; // Return stack
	protected int rp; // Return stack pointer
	protected float f[]; // Floating point stack
	protected int fp; // Floating point stack pointer

	protected int ip; // Instruction pointer

	protected HashMap<Integer, Object> o; // Indexed map of objects
	protected int op; // Last index of previous HashMap

	protected ArrayList<Consumer<Sloth>> p; // Array of primitives

	/* This array was created by Gemini for the included */
	/* implementation, I must re-check it. */
	private ArrayList<java.io.RandomAccessFile> openFiles = new ArrayList<>();

	public static final int STACK_OVERFLOW = -3;
	public static final int STACK_UNDERFLOW = -4;
	public static final int RETURN_STACK_OVERFLOW = -5;
	public static final int RETURN_STACK_UNDERFLOW = -6;

	/* -- Displacement of counted string buffer from here -- */

	/* TODO CBuffer could be just another space of the normal */
	/* strings circular buffer. */
	public static final int SLOTH_CBUF = 64;

	/* -- Dictionary variables ----------------------------- */

	public static final int SLOTH_HERE = 0;
	public static final int SLOTH_INTERNAL_WL = 1*sCELL;
	public static final int SLOTH_FORTH_WL = 2*sCELL;

	/* -- User area variables and buffers ------------------ */
	
	public static final int SLOTH_CURRENT = 0*sCELL;
	public static final int SLOTH_ORDER = 1*sCELL;
	public static final int SLOTH_LOCALS_WORDLIST = 2*sCELL;
	public static final int SLOTH_CONTEXT = 3*sCELL;
	/* There are 16 CELLS reserved to search order */
	public static final int SLOTH_BASE = 19*sCELL;
	public static final int SLOTH_STATE = 20*sCELL;
	public static final int SLOTH_IBUF = 21*sCELL;
	public static final int SLOTH_IPOS = 22*sCELL;
	public static final int SLOTH_ILEN = 23*sCELL;
	public static final int SLOTH_SOURCE_ID = 24*sCELL;
	public static final int SLOTH_SOURCE_POS = 25*sCELL;
	public static final int SLOTH_LATESTXT = 26*sCELL;
	public static final int SLOTH_INTERPRET = 27*sCELL;
	
	public static final int SLOTH_ROOT_PATH_LENGTH = 28*sCELL;
	public static final int SLOTH_PATH_START = 29*sCELL;
	public static final int SLOTH_PATH_END = 30*sCELL;
	/* Continuous space to store path strings */
	public static final int SLOTH_PATHS = 31*sCELL;
	
	/* Space between SLOTH_PATHS and SLOTH_INCLUDED_FILES */
	/* reserved to store paths. */
	
	public static final int SLOTH_INCLUDED_FILES = 95*sCELL;
	
	public static final int SLOTH_LAST_USER_VAR = 96*sCELL;
	
	/* -- Flags for word status ---------------------------- */
	
	public static final int SLOTH_HIDDEN = 1;
	public static final int SLOTH_IMMEDIATE = 2;

	/* -- Context initialization -------------------------- */

	public Sloth() { this(524288, 1024); }

	public Sloth(int dsize, int usize) {
		s = new int[STACK_SIZE];
		sp = 0;
		r = new int[RETURN_STACK_SIZE];
		rp = 0;
		f = new float[FLOAT_STACK_SIZE];
		fp = 0;

		ip = -1;
		p = new ArrayList<Consumer<Sloth>>();
		o = new HashMap<Integer, Object>();
		op = 0;
		o.put(op++, ByteBuffer.allocate(dsize));
		d = 0;
		o.put(op++, ByteBuffer.allocate(usize));
		u = 1;

		// Circular buffer for storing Java strings
		// that need to be converted to Forth strings
		o.put(op++, ByteBuffer.allocate(2048));

		// // Initialize HERE
		((ByteBuffer)(o.get(d))).putInt(0*sCELL, 3*sCELL);
		// Initialize INTERNAL-WORDLIST
		((ByteBuffer)(o.get(d))).putInt(1*sCELL, 0);
		// Initialize FORTH-WORDLIST (the default wordlist) */
		((ByteBuffer)(o.get(d))).putInt(2*sCELL, 0);


		((ByteBuffer)(o.get(u))).putInt(0*sCELL, to_abs(SLOTH_FORTH_WL, d)); // CURRENT
		((ByteBuffer)(o.get(u))).putInt(1*sCELL, 2); // #ORDER
		((ByteBuffer)(o.get(u))).putInt(2*sCELL, 0); // LOCALS-WORDLIST
		((ByteBuffer)(o.get(u))).putInt(3*sCELL, to_abs(SLOTH_FORTH_WL, d)); // CONTEXT 0
		((ByteBuffer)(o.get(u))).putInt(4*sCELL, to_abs(SLOTH_INTERNAL_WL, d)); // CONTEXT 
}

	// -- Data stack

	void push(int v) { s[sp++] = v; }
	int pop() { return s[--sp]; }
	int pick(int a) { return s[sp - a - 1]; }

	// Helpers for double numbers
	long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }
	/* Long pop -- takes a 32 bit CELL as a 64 bit value; */
	/* TODO: Remove it, its just (long)pop(); */
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

	void fpush(float v) { f[fp++] = v; }
	float fpop() { return f[--fp]; }

	// -- Memory

	// The C implementation of SLOTH uses absolute addresses
	// that allow access to all the memory. That also allows
	// the use of several memory blocks (dictionaries).
	// To do that in Java, memory addresses consist of two parts:
	// <block index> - 8 bits
	// <byte address> - 24 bits

	// Convert between absolute and relative addresses

	int to_abs(int a, int b) { return (b << 24) + a; }
	int to_rel(int a) { return a & 0x00FFFFFF; }
	ByteBuffer block(int a) { return (ByteBuffer)(o.get(a >> 24)); }

	void b_store(int a, byte v) { block(a).put(to_rel(a), v); }
	byte b_fetch(int a) { return block(a).get(to_rel(a)); }
	void c_store(int a, char v) { block(a).putChar(to_rel(a), v); }
	char c_fetch(int a) { return block(a).getChar(to_rel(a)); }
	void store(int a, int v) { block(a).putInt(to_rel(a), v); }
	int fetch(int a) { return block(a).getInt(to_rel(a)); }
	void f_store(int a, float v) { block(a).putFloat(to_rel(a), v); }
	float f_fetch(int a) { return block(a).getFloat(to_rel(a)); }

	// Helpers to use Java Strings with Sloth API

	// Memory block 2 is used as a transient memory to copy
	// Java Strings to it. No need to delete them, this will
	// act as a circular buffer and overwrite previous strings
	// as required.
	int FromString(String s) {
		ByteBuffer b = (ByteBuffer)(o.get(2));
		if (b.remaining() / suCHAR < s.length()) b.rewind();
		int addr = to_abs(b.position(), 2);
		for (int i = 0; i < s.length(); i++) {
			b.putChar(s.charAt(i));
		}
		return addr;
	}

	String ToString(int a, int l) {
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < l; i++) sb.append(c_fetch(a + i*suCHAR));
		return sb.toString();
	}

	// -- Inner interpreter

	int op() { int o = fetch(ip); ip += sCELL; return o; }
	protected void do_prim(int q) { p.get(-1 - q).accept(this); }
	protected void call(int q) { 
		if (ip >= 0 || rp > 0) rpush(ip); 
		ip = q; 
	}
	protected void execute(int q) { 
		if (q < 0) do_prim(q); 
		else call(q); 
	}
	protected void inner() { int t = rp; while (t <= rp && ip >= 0) execute(op()); }
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
	}

	public void _throw(int v) {
		if (v != 0) {
			System.out.printf("EXCEPTION: %d\n", v);
			System.out.printf("BUFFER: %s\n", ToString(user_get(IBUF), user_get(ILEN)));
			System.out.printf("TOKEN: %s\n", ToString(user_get(IBUF) + (user_get(IPOS)*suCHAR), user_get(ILEN) - user_get(IPOS)));
			throw new SlothException(v); 
		}
	}

	// == Forth Kernel

	// Constants 
	
	// Displacement of counted string buffer from "here"
	private static final int CBUF = 64;
	
	// Relative addresses of variables accessed both from C
	// and Forth.

	// Start of dictionary variables
	
	private static final int HERE = 0;
	private static final int INTERNAL_WL = 1*sCELL;
	private static final int FORTH_WL = 2*sCELL;
	
	// User area variables
	
	private static final int CURRENT = 0*sCELL;
	private static final int ORDER = 1*sCELL;
	private static final int LOCALS_WORDLIST = 2*sCELL;
	private static final int CONTEXT = 3*sCELL;
	// Here there are 16 CELLS reserved to search order
	private static final int BASE = 19*sCELL;
	private static final int STATE = 20*sCELL;
	private static final int IBUF = 21*sCELL;
	private static final int IPOS = 22*sCELL;
	private static final int ILEN = 23*sCELL;
	private static final int SOURCE_ID = 24*sCELL;
	private static final int SOURCE_POS = 25*sCELL;
	private static final int LATESTXT = 26*sCELL;
	private static final int INTERPRET = 27*sCELL;

	private static final int ROOT_PATH_LENGTH = 28*sCELL;
	private static final int PATH_START = 29*sCELL;
	private static final int PATH_END = 30*sCELL;
	// Continuous space to store path strings
	private static final int PATHS = 31*sCELL;
	
	// Space between SLOTH_PATHS and SLOTH_INCLUDED_FILES
	// reserved to store path strings.
	
	private static final int INCLUDED_FILES = 95*sCELL;
	
	private static final int LAST_USER_VAR = 96*sCELL;
	
	// Word statuses
	
	private static final char HIDDEN = 1;
	private static final char IMMEDIATE = 2;

	// -- Helpers

	// Dictionary set/get
	void set(int a, int v) { store(a, v); }
	int get(int a) { return fetch(a); }

	void c_set(int a, char v) { c_store(a, v); }
	char c_get(int a) { return c_fetch(a); }

	// User area set/get.
	void user_set(int rel_a, int v) { 
		((ByteBuffer)(o.get(u))).putInt(to_rel(rel_a), v); 
	}
	int user_get(int rel_a) { 
		return ((ByteBuffer)(o.get(u))).getInt(to_rel(rel_a)); 
	}

	// Memory management
	int here() { return get(HERE); }
	void allot(int v) { set(HERE, here() + v); }
	int aligned(int a) { return ((a + (sCELL - 1)) & ~(sCELL - 1)); }
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
	void fliteral(float f) {
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
	// Wordlist_link (CELL) @ NT + 2*sCELL
	// Flags (CHAR) @ NT + 3*sCELL
	// Name_length (CHAR) @ NT + 3*sCELL + suCHAR
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

	// -- Primitives ----------------------------------------

	void _exit_() { ip = (rp > 0) ? rpop() : -1; }
	void _lit_() { push(op()); }
	void _rip_() { int tip = ip; int o = op(); push(tip + o - sCELL); }
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
		ip = aligned(ip + l*suCHAR + suCHAR);
	}
	// Quotations (not in ANS Forth yet)
	void _quotation_() { int d = op(); push(ip); ip += d; }
	void _start_quotation_() {
		int s = user_get(STATE);
		user_set(STATE, s <= 0 ? s - 1 : s + 1);
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
		case 12: push(FLOAT_STACK_SIZE); break; // FLOATING-STACK
		// Obsolescent queries (required for tests)
		case 100: push(-1); break;
		// Non standard queries
		case -1: push(5); break;
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
			RandomAccessFile file = (RandomAccessFile)(o.get(pop()));
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
			RandomAccessFile file = (RandomAccessFile)(o.get(pop()));	
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
						// b_store(caddr + i, (byte)buf.charAt(i));
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
				user_set(SLOTH_SOURCE_POS, (int)dpop());

				/* REFILL can only be called after INCLUDE/INCLUDED */
				/* that means that the line buffer of _included_ will */
				/* be used and its size is known and fixed. */
				push(user_get(SLOTH_IBUF));
				push(1024);
				push(source_id);
				_read_line_();

				int ior = pop();
				int flag = pop();

				if (flag != 0 && ior == 0) {
					user_set(SLOTH_ILEN, pop());
					user_set(SLOTH_IPOS, 0);
					push(-1);
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
	void _included_() {
		int l = pop();
		int a = pop();

		int previbuf = user_get(IBUF);
		int previpos = user_get(IPOS);
		int previlen = user_get(ILEN);

		int prevsourceid = user_get(SOURCE_ID);

		int prevstart = user_get(PATH_START);
		int prevend = user_get(PATH_END);

		int pathstart = prevstart;
		int pathend = prevend;

		String filename = ToString(a, l);

		// Copy filename to pathend
		for (int i = 0; i < l; i++) {
			c_set(pathend + i*suCHAR, c_get(a + i*suCHAR));
		}
		c_set(pathend + l*suCHAR, (char)0);

		java.io.RandomAccessFile f = null;
		String fullPathString = "";

		try {
			fullPathString = filename;
			f = new java.io.RandomAccessFile(fullPathString, "r");
			pathstart = pathend;
			pathend = pathend + l*suCHAR;
		} catch (Exception e1) {
			try {
				int totalLen = (pathend + l*suCHAR - pathstart)/suCHAR;
				fullPathString = ToString(pathstart, totalLen);
				f = new java.io.RandomAccessFile(fullPathString, "r");
				pathend = pathend + l*suCHAR;
			} catch (Exception e2) {
				try {
					int rootLen = user_get(ROOT_PATH_LENGTH);
					int paths_addr = to_abs(PATHS, u);
					// Copy root path to pathend
					for (int i = 0; i < rootLen; i++) {
						c_set(pathend + i*suCHAR, c_get(paths_addr + i*suCHAR));
					}
					// Copy filename after root path
					for (int i = 0; i < l; i++) {
						c_set(pathend + rootLen*suCHAR + i*suCHAR, c_get(a + i*suCHAR));
					}
					c_set(pathend + (rootLen + l)*suCHAR, (char)0);
					fullPathString = ToString(pathend, rootLen + l);
					f = new java.io.RandomAccessFile(fullPathString, "r");
					pathstart = pathend;
					pathend = pathend + (rootLen + l)*suCHAR;
				} catch (Exception e3) {
					// Failed to open file
				}
			}
		}

		if (f != null) {
			// Remove filename from path...
			while (pathend > pathstart) {
				char c = c_get(pathend - suCHAR);
				if (c == '/' || c == '\\') {
					break;
				}
				pathend -= suCHAR;
			}
			// ...and store for nested includes.
			user_set(PATH_START, pathstart);
			user_set(PATH_END, pathend);

			int interpret_xt = user_get(INTERPRET);

			openFiles.add(f);
			int source_id = openFiles.size();
			user_set(SOURCE_ID, source_id);

			// Add path+filename to INCLUDED FILES
			int h = here();
			comma(user_get(INCLUDED_FILES));
			user_set(INCLUDED_FILES, h);
			comma(l);
			for (int i = 0; i < l; i++) {
				c_comma(c_get(a + i*suCHAR));
			}
			_align_();

			try {
				user_set(SOURCE_POS, (int)f.getFilePointer());
				int linenumber = 0;
				String line;
				while ((line = f.readLine()) != null) {
					// Create temporary block for line
					ByteBuffer b = ByteBuffer.allocate(Math.max(1024, line.length() + 1) * suCHAR);
					for (int i = 0; i < line.length(); i++) {
						b.putChar(line.charAt(i));
					}
					o.put(op++, b);
					// int block_idx = m.size() - 1;
					int block_idx = op - 1;

					user_set(IBUF, to_abs(0, block_idx));
					user_set(IPOS, 0);
					user_set(ILEN, line.length());

					_catch(interpret_xt);
					int e = pop();
					if (e != 0) {
						System.out.println("File: " + fullPathString);
						System.out.printf("Line (%d): %s\n", linenumber, line);
						_throw(e);
					}
					user_set(SOURCE_POS, (int)f.getFilePointer());
					linenumber++;
					o.remove(op - 1);
				}
			} catch (java.io.IOException e) {
				// Handle IO error if needed
			} finally {
				user_set(SOURCE_ID, prevsourceid);
				try {
					f.close();
				} catch (java.io.IOException e) {}
				openFiles.remove(openFiles.size() - 1);
			}
		}

		// Restore previous path
		user_set(PATH_START, prevstart);
		user_set(PATH_END, prevend);

		// Restore previous input buffer
		user_set(IBUF, previbuf);
		user_set(IPOS, previpos);
		user_set(ILEN, previlen);

		if (f == null) {
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
		int w = search_word(cstring + suCHAR, c_fetch(cstring));
		if (w == 0) { push(cstring); push(0); }
		else if (has_flag(w, IMMEDIATE)) { push(get_xt(w)); push(1); }
		else { push(get_xt(w)); push(-1); }
	}

	// Helper to find words from Java
	public int find_word(String name) {
		return search_word(FromString(name), name.length());
	}

	// -- Outer interpreter

	void _interpret_() {
		int flag;
		while (user_get(IPOS) < user_get(ILEN)) {
			push(32); _word_();
			int tok = pick(0) + suCHAR;
			int tlen = c_fetch(pick(0));
			if (tlen == 0) { pop(); return; }
			_find_();
			if ((flag = pop()) != 0) {
				if (user_get(STATE) == 0 || (user_get(STATE) != 0 && flag == 1)) {
					eval(pop());
				} else {
					compile(pop());
				}
			} else {
				int temp_base = user_get(BASE);
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
								fpush(r);
							} else {
								fliteral(r);
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
	void _unused_() { 
		push(((ByteBuffer)(o.get(d))).capacity() - here()); 
	}

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
			if (e == -2) {
				// If it's ABORT" print the message
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
	void _swap_() { int a = pop(); int b = pop(); push(a); push(b); }

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

		_catch(user_get(INTERPRET));

		user_set(SOURCE_ID, prevsourceid);
		user_set(IBUF, previbuf);
		user_set(IPOS, previpos);
		user_set(ILEN, previlen);

		int e = pop();
		if (e != 0) _throw(e);
	}
	void _execute_() { eval(pop()); }
	// Debug is not ANS
	void _debug_() {
		int post_xt = pop();
		int inner_xt = pop();
		int pre_xt = pop();
		int q = pop();
		debug(pre_xt);
		execute(q);
		if (q > 0) debug_inner(inner_xt);
		debug(post_xt);
	}
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

	// Helpers for bootstrapping

	int primitive(Consumer<Sloth> c) {
		p.add(c);
		return 0 - p.size();
	}
	int code(String name, int xt) {
		int w = header(FromString(name), name.length());
		set_xt(w, xt);
		return xt;
	}
	void user_variable(String name, int d, int v) {
		int w = header(FromString(name), name.length());
		set_xt(w, here());
		literal(to_abs(0, u) + d);
		compile(get_xt(find_word("EXIT")));
		store(to_abs(0, u) + d, v);
	}
	void _empty_rs_() { rp = 0; }
	// In C there is _ints_ and _self_, but I don't need it here.

	// Bootstrapping

	void bootstrap_kernel() {
		// Initialization of dictionary
		store(to_abs(0, d), to_abs(sCELL, d)); /* HERE */
		comma(0); /* INTERNAL-WORDLIST */
		comma(0); /* FORTH-WORDLIST */
		comma(0); /* INCLUDED FILES LINKED LIST */
		// Initialization of user area
		store(to_abs(0*sCELL, u), to_abs(FORTH_WL, d)); // CURRENT
		store(to_abs(1*sCELL, u), 2); // #ORDER
		store(to_abs(2*sCELL, u), 0); // LOCALS-WORDLIST
		store(to_abs(3*sCELL, u), to_abs(FORTH_WL, d)); // CONTEXT 0
		store(to_abs(4*sCELL, u), to_abs(INTERNAL_WL, d)); // CONTEXT 1
		// Basic primitives
		// EXIT and (LIT) must be defined before using 
		// user_area_variable
		code("EXIT", primitive((vm) -> _exit_()));
		user_set(CURRENT, to_abs(INTERNAL_WL, d));
		code("(LIT)", primitive((vm) -> _lit_()));

		// TODO Reorder user area variables to not need changing
		// between wordlists

		// User Area Variables
		// NOTE What is stored in (CURRENT) here will affect later
		// use of hedader
		user_variable("(CURRENT)", CURRENT, to_abs(FORTH_WL, d));
		user_variable("#ORDER", ORDER, 2);
		user_variable("(LOCALS-WORDLIST)", LOCALS_WORDLIST, 0);
		user_variable("CONTEXT", CONTEXT, to_abs(FORTH_WL, d));
		user_variable("BASE", BASE, 10);
		user_variable("STATE", STATE, 0);
		user_variable("(IBUF)", IBUF, 0);
		user_variable(">IN", IPOS, 0);
		user_variable("(ILEN)", ILEN, 0);
		user_variable("(SOURCE-ID)", SOURCE_ID, 0);
		user_variable("(SOURCE-POS)", SOURCE_POS, 0);
		user_variable("(LATESTXT)", LATESTXT, 0);
		user_variable("(INTERPRET)", INTERPRET, 0);
		user_variable("(SLOTH_ROOT_PATH_LENGTH)", ROOT_PATH_LENGTH, 0);
		user_variable("(SLOTH_PATH_START)", PATH_START, to_abs(PATHS, u));
		user_variable("(SLOTH_PATH_END)", PATH_END, to_abs(PATHS, u));
		user_variable("(SLOTH_PATHS)", PATHS, 0);
		user_variable("(INCLUDED-FILES)", INCLUDED_FILES, 0);

		// Primitives
		code("(RIP)", primitive((vm) -> _rip_()));
		code("(BRANCH)", primitive((vm) -> _branch_()));
		code("(?BRANCH)", primitive((vm) -> _zbranch_()));
		code("(STRING)", primitive((vm) -> _string_()));
		code("(CSTRING)", primitive((vm) -> _c_string_()));
		code("(QUOTATION)", primitive((vm) -> _quotation_()));
		code("(DOES)", primitive((vm) -> _do_does_()));
		code("(ENVIRONMENT)", primitive((vm) -> _environment_()));

		user_set(CURRENT, to_abs(FORTH_WL, d));

		code("[:", primitive((vm) -> _start_quotation_()));
		code(";]", primitive((vm) -> _end_quotation_()));

		code("UNUSED", primitive((vm) -> _unused_()));
		code("BYE", primitive((vm) -> _bye_()));

		code("REFILL", primitive((vm) -> _refill_()));
		code("SAVE-INPUT", primitive((vm) -> _save_input_()));
		code("RESTORE-INPUT", primitive((vm) -> _restore_input_()));
		code("INCLUDED", primitive((vm) -> _included_()));

		code("MOVE", primitive((vm) -> _move_()));

		code("EMIT", primitive((vm) -> _emit_()));
		code("KEY", primitive((vm) -> _key_()));

		code("AND", primitive((vm) -> _and_()));
		code("INVERT", primitive((vm) -> _invert_()));
		code("LSHIFT", primitive((vm) -> _l_shift_()));
		code("-", primitive((vm) -> _minus_()));
		code("+", primitive((vm) -> _plus_()));
		code("RSHIFT", primitive((vm) -> _r_shift_()));
		code("*", primitive((vm) -> _star_()));
		code("2/", primitive((vm) -> _two_slash_()));
		code("UM*", primitive((vm) -> _u_m_star_()));
		code("UM/MOD", primitive((vm) -> _u_m_slash_mod_()));

		code("C@", primitive((vm) -> _c_fetch_()));
		code("C!", primitive((vm) -> _c_store_()));
		code("@", primitive((vm) -> _fetch_()));
		code("!", primitive((vm) -> _store_()));

		// In C there is INT@ and INT!, no sense here neither

		code("=", primitive((vm) -> _equals_()));
		code("<", primitive((vm) -> _less_than_()));

		code(":", primitive((vm) -> _colon_()));
		code(":NONAME", primitive((vm) -> _colon_no_name_()));
		code(";", primitive((vm) -> _semicolon_())); _immediate_();

		code("CATCH", primitive((vm) -> _catch_()));
		code("THROW", primitive((vm) -> _throw_()));

		code("DROP", primitive((vm) -> _drop_()));
		code("DUP", primitive((vm) -> _dup_()));
		code("OVER", primitive((vm) -> _over_()));
		code(">R", primitive((vm) -> _to_r_()));
		code("R>", primitive((vm) -> _r_from_()));
		code("SWAP", primitive((vm) -> _swap_()));

		code("RECURSE", primitive((vm) -> _recurse_())); _immediate_();

		code("ALLOT", primitive((vm) -> _allot_()));
		code("CELLS", primitive((vm) -> _cells_()));
		code("CHARS", primitive((vm) -> _chars_()));
		code("COMPILE,", primitive((vm) -> _compile_comma_()));
		code("CREATE-NAME", primitive((vm) -> _create_name_()));
		code("CREATE", primitive((vm) -> _create_()));
		code("DOES>", primitive((vm) -> _does_()));
		code("EVALUATE", primitive((vm) -> _evaluate_()));
		code("EXECUTE", primitive((vm) -> _execute_()));
		/* NON ANS */ code("DEBUG", primitive((vm) -> _debug_()));
		code("HERE", primitive((vm) -> _here_()));
		code("IMMEDIATE", primitive((vm) -> _immediate_()));
		code("POSTPONE", primitive((vm) -> _postpone_()));
		code("SOURCE", primitive((vm) -> _source_()));
		code("WORD", primitive((vm) -> _word_()));
		code("FIND", primitive((vm) -> _find_()));

		code("DICT", primitive((vm) -> push(to_abs(d, 0))));
		code("USER", primitive((vm) -> push(to_abs(u, 0))));
		user_set(INTERPRET, primitive((vm) -> _interpret_()));
		code("(EMPTY-RETURN-STACK)", primitive((vm) -> _empty_rs_()));
	}

	void repl() {
		o.put(op++, ByteBuffer.allocate(256));
		user_set(IBUF, to_abs(0, op - 1));
		user_set(IPOS, 0);
		user_set(ILEN, 80);
		eval(get_xt(find_word("QUIT")));
		// TODO Maybe remove the memory block?
	}

	void evaluate(String c) {
		ByteBuffer buf = ByteBuffer.allocate(256);
		buf.clear();
		o.put(op++, buf);
		for(int i = 0; i < c.length(); i++) {
			buf.putChar(c.charAt(i));
		}
		push(to_abs(0, op - 1));
		push(c.length());
		_evaluate_();
	}

	int include(String f) {
		push(FromString(f));
		push(f.length());
		_catch(get_xt(find_word("INCLUDED")));
		return pop();
	}
} 
