package io.github.jordipbou.Sloth;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.io.IOException;
import java.util.function.Consumer;

public class Sloth {
	private static final int STACK_SIZE = 64;
	private static final int RETURN_STACK_SIZE = 64;
	private static final int FLOAT_STACK_SIZE = 64;

	public static final int sCHAR = 2;
	public static final int sCELL = 4;
	public static final int sFCELL = 4;

	public static final int STACK_OVERFLOW = -3;
	public static final int STACK_UNDERFLOW = -4;
	public static final int RETURN_STACK_OVERFLOW = -5;
	public static final int RETURN_STACK_UNDERFLOW = -6;

	private int s[];
	private int sp;
	private int r[];
	private int rp;
	private float f[];
	private int fp;

	private int ip;

	// Memories
	private ArrayList<ByteBuffer> m;

	private int d; // Index of the dictionary in m
	private int u; // Index of the user area in m

	ArrayList<Consumer<Sloth>> p;

	public Sloth(int dsize, int usize) {
		s = new int[STACK_SIZE];
		r = new int[RETURN_STACK_SIZE];
		f = new float[FLOAT_STACK_SIZE];
		ip = -1;
		p = new ArrayList<Consumer<Sloth>>();
		m = new ArrayList<ByteBuffer>();
		m.add(ByteBuffer.allocate(dsize));
		d = 0;
		m.add(ByteBuffer.allocate(usize));
		u = 1;
		m.add(ByteBuffer.allocate(1024));
	}

	// -- Data stack

	void push(int v) { s[sp++] = v; }
	int pop() { return s[--sp]; }
	int pick(int a) { return s[sp - a - 1]; }

	// Helpers for double numbers
	long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }
	long lpop() { return (long)s[--sp]; }
	long upop() { return Integer.toUnsignedLong(s[--sp]); }
	long udpop() { long v = upop(); return msp(v << 32) + lsp(upop()); }
	void dpush(long v) { push((int)lsp(v)); push((int)(v >> 32)); }
	long dpop() { 
		long v = lpop(); 
		return msp(v << 32) + lsp(lpop());
	}

	// -- Float stack
	void fpush(float v) { f[fp++] = v; }
	float fpop() { return f[--fp]; }

	// -- Return stack

	void rpush(int v) { r[rp++] = v; }
	int rpop() { return r[--rp]; }
	int rpick(int a) { return r[rp - a - 1]; }

	// -- Memory

	// Memory addresses consist of two parts:
	// <block index> - 8 bits
	// <byte address> - 24 bits

	// Convert between absolute and relative addresses

	int to_abs(int a, int b) { return (b << 24) + a; }
	int to_rel(int a) { return a & 0x00FFFFFF; }
	ByteBuffer block(int a) { return m.get(a >> 24); }

	void cstore(int a, char v) { block(a).putChar(to_rel(a), v); }
	char cfetch(int a) { return block(a).getChar(to_rel(a)); }

	void store(int a, int v) { block(a).putInt(to_rel(a), v); }
	int fetch(int a) { return block(a).getInt(to_rel(a)); }

	void fstore(int a, float v) { block(a).putFloat(to_rel(a), v); }

	// Helpers to use Java Strings with Sloth API

	// Memory block 2 is used as a transient memory to copy
	// Java Strings to it. No need to delete them, this will
	// act as a circular buffer and overwrite previous strings
	// as required.
	int FromString(String s) {
		ByteBuffer b = m.get(2);
		if (b.remaining() / sCHAR < s.length()) b.rewind();
		int addr = to_abs(b.position(), 2);
		for (int i = 0; i < s.length(); i++) {
			b.putChar(s.charAt(i));
		}
		return addr;
	}

	String ToString(int a, int l) {
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < l; i++) sb.append(cfetch(a + i*sCHAR));
		return sb.toString();
	}

	// -- Inner interpreter

	int op() { int o = fetch(ip); ip += sCELL; return o; }
	void do_prim(int q) { p.get(-1 - q).accept(this); }
	void call(int q) { if (ip >= 0) rpush(ip); ip = q; }
	void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	void inner() { int t = rp; while (t <= rp && ip >= 0) execute(op()); }
	void eval(int q) { execute(q); if (q > 0) inner(); }

	// -- Tracing interpreter

	void debug(int debug_xt) { push(ip); eval(debug_xt); }
	void debug_inner(int debug_xt) {
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

	public void _throw(int v) {
		// TODO Remove	
		System.out.printf("EXCEPTION: %d\n", v);
		System.out.printf("BUFFER: %s\n", ToString(ua_get(IBUF), ua_get(ILEN)));
		System.out.printf("TOKEN: %s\n", ToString(ua_get(IBUF) + (ua_get(IPOS)*sCHAR), ua_get(ILEN) - ua_get(IPOS)));
		// --
		if (v != 0) throw new SlothException(v); 
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

	// In java this is just a synonim for store and get.
	void set(int a, int v) { store(a, v); }
	int get(int a) { return fetch(a); }

	void cset(int a, char v) { cstore(a, v); }
	char cget(int a) { return cfetch(a); }

	// User area set/get
	void ua_set(int rel_a, int v) { m.get(u).putInt(to_rel(rel_a), v); }
	int ua_get(int rel_a) { return m.get(u).getInt(to_rel(rel_a)); }

	// Memory management
	int here() { return get(HERE); }
	void allot(int v) { set(HERE, here() + v); }
	int aligned(int a) { return ((a + (sCELL - 1)) & ~(sCELL - 1)); }
	void align() { set(HERE, aligned(here())); }

	// Compilation
	void comma(int v) { store(here(), v); allot(sCELL); }
	void ccomma(char v) { cstore(here(), v); allot(sCHAR); }
	void fcomma(float v) { fstore(here(), v); allot(sFCELL); }
	void compile(int xt) { comma(xt); }
	void literal(int n) { 
		comma(get_xt(find_word("(LIT)")));
		comma(n);
	}
	void fliteral(float f) {
		comma(get_xt(find_word("(FLIT)")));
		fcomma(f);
	}

	// Headers
	int get_latest() { return fetch(ua_get(CURRENT)); }
	void set_latest(int w) { store(ua_get(CURRENT), w); }

	// Header structure:
	// Link (CELL) @ NT
	// XT (CELL) @ NT + sCELL
	// Wordlist_link (CELL) @ NT + 2*sCELL
	// Flags (CHAR) @ NT + 3*sCELL
	// Name_length (CHAR) @ NT + 3*sCELL + sCHAR
	// Name (CHAR*namelen) @ NT + 2*sCELL + 2*sCHAR

	int header(int n, int l) {
		align();
		int w = here();
		comma(get_latest());
		set_latest(w);
		comma(0);
		ccomma((char)0);
		ccomma((char)l);
		for (int i = 0; i < l; i++) ccomma(cfetch(n + i*sCHAR));
		align();
		store(w + sCELL, here());
		return w;
	}

	int get_link(int w) { return fetch(w); }
	int get_xt(int w) { return fetch(w + sCELL); }
	void set_xt(int w, int xt) { store(w + sCELL, xt); }
	char get_flags(int w) { return cfetch(w + 2*sCELL); }
	boolean has_flag(int w, int v) { return (get_flags(w) & v) == v; }
	char get_namelen(int w) { return cfetch(w + 2*sCELL + sCHAR); }
	int get_name_addr(int w) { return w + 2*sCELL + 2*sCHAR; }

	// Setting flags
	void set_flag(int w, char v) { 
		cstore(w + 2*sCELL, (char)(get_flags(w) | v)); 
	}
	void unset_flag(int w, char v) {
		cstore(w + 2*sCELL, (char)(get_flags(w) & ~v));
	}

	// -- Primitives ----------------------------------------
	void _exit_() { ip = (rp > 0) ? rpop() : -1; }
	void _lit_() { push(op()); }
	void _rip_() { int tip = ip; int o = op(); push(ip + o - sCELL); }
	void _branch_() { ip += op() - sCELL; }
	void _zbranch_() { ip += pop() == 0 ? (op() - sCELL) : sCELL; }
	void _string_() { 
		int l = op(); 
		push(ip); 
		push(l); 
		ip = aligned(ip + l + 1); 
	}
	void _c_string_() {
		char l = cfetch(ip);
		push(ip);
		ip = aligned(ip + l + 2);
	}
	// Quotations (not in ANS Forth yet)
	void _quotation_() { int d = op(); push(ip); ip += d; }
	void _start_quotation_() {
		int s = ua_get(STATE);
		ua_set(STATE, s <= 0 ? s - 1 : s + 1);
		if (ua_get(STATE) == -1) push(here() + 2*sCELL);
		push(ua_get(LATESTXT));
		compile(get_xt(find_word("(QUOTATION)")));
		push(here());
		comma(0);
		ua_set(LATESTXT, here());
	}
	void _end_quotation_() {
		int s = ua_get(STATE);
		int a = pop();
		compile(get_xt(find_word("EXIT")));
		store(a, here() - a - sCELL);
		ua_set(LATESTXT, pop());
		ua_set(STATE, s < 0 ? s + 1 : s - 1);
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

	// Parsing input

	// The region to store WORD generated counted strings
	// starts at "HERE" + CBUF
	void _word_() {
		char c = (char)pop();
		int ibuf = ua_get(IBUF);
		int ilen = ua_get(ILEN);
		int ipos = ua_get(IPOS);
		// First, ignore c until not c is found
		// As per the Forth standard, if the control character is the
		// space (hex 20) then control characters (< 32) may be treated
		// as delimiters.
		if (c == 32) {
			while (ipos < ilen && cfetch(ibuf + (ipos*sCHAR)) <= c) ipos++;
		} else {
			while (ipos < ilen && cfetch(ibuf + (ipos*sCHAR)) == c) ipos++;
		}
		int start = ibuf + (ipos*sCHAR);
		// Next, continue parsing until c is found again
		if (c == 32) {
			while (ipos < ilen && cfetch(ibuf + (ipos*sCHAR)) > c) ipos++;
		} else {
			while (ipos < ilen && cfetch(ibuf + (ipos*sCHAR)) != c) ipos++;
		}
		int end = ibuf + (ipos*sCHAR);
		// Now, copy the length and the string to the 
		// counted string buffer
		cstore(here() + CBUF, (char)((end - start) / sCHAR));
		for (int i = 0; i < ((end - start) / sCHAR); i++) {
			cstore(
				here() + CBUF + sCHAR + i*sCHAR, 
				cfetch(start + i*sCHAR));
		}
		push(here() + CBUF);
		// If not at the end of the input buffer,
		// skip c after the word, but don't treat it as part
		// of the counted string.
		if (ipos < ilen) ipos++;
		ua_set(IPOS, ipos);
	}

	// Finding words
	boolean compare_without_case(int a1, int u1, int a2, int u2) {
		if (u1 != u2) return false;
		for (int i = 0; i < u2; i++) {
			char a = cfetch(a1 + i*sCHAR);
			char b = cfetch(a2 + i*sCHAR);
			if (a >= 97 && a <= 122) a -= 32;
			if (b >= 97 && b <= 122) b -= 32;
			if (a != b) return false;
		}
		return true;
	}

	int search_word(int n, int l) {
		for (int i = -1; i < ua_get(ORDER); i++) {
			int wl = ua_get(CONTEXT + i*sCELL);
			if (wl != 0) {
				int w = fetch(wl);
				while (w > 0) {
					if (!has_flag(w, HIDDEN) 
					 && compare_without_case(
								get_name_addr(w), get_namelen(w),
								n, l)) {
						return w;
					}
					w = get_link(w);
				}
			}
		}
		return 0;
	}

	void _find_() {
		int cstring = pop();
		int w = search_word(cstring + sCHAR, cfetch(cstring));
		if (w == 0) { push(cstring); push(0); }
		else if (has_flag(w, IMMEDIATE)) { push(get_xt(w)); push(1); }
		else { push(get_xt(w)); push(-1); }
	}

	// Helper to find words from Java
	int find_word(String name) {
		// // Copy the string to a transient memory (3rd ByteBuffer array)
		// for (int i = 0; i < name.length(); i++) {
		// 	m.get(2).putChar(name.charAt(i));
		// }
		// return search_word(to_abs(0, 2), name.length());
		return search_word(FromString(name), name.length());
	}

	// -- Outer interpreter

	void _interpret_() {
		int flag;
		while (ua_get(IPOS) < ua_get(ILEN)) {
			push(32); _word_();
			int tok = pick(0) + sCHAR;
			int tlen = cfetch(pick(0));
			if (tlen == 0) { pop(); return; }
			_find_();
			if ((flag = pop()) != 0) {
				if (ua_get(STATE) == 0 || (ua_get(STATE) != 0 && flag == 1)) {
					eval(pop());
				} else {
					compile(pop());
				}
			} else {
				int temp_base = ua_get(BASE);
				pop();
				if (tlen == 3 
				 && cfetch(tok) == '\'' 
				 && cfetch(tok + 2*sCHAR) == '\'') {
					if (ua_get(STATE) == 0) {
						push(cfetch(tok + sCHAR));
					} else {
						literal(cfetch(tok + sCHAR));
					}
				} else {
					boolean is_double = false;
					if (cfetch(tok) == '#') {
						temp_base = 10;
						tlen--;
						tok++;
					} else if (cfetch(tok) == '$') {
						temp_base = 16;
						tlen--;
						tok++;
					} else if (cfetch(tok) == '%') {
						temp_base = 2;
						tlen--;
						tok++;
					} else if (cfetch(tok + tlen*sCHAR - sCHAR) == '.') {
						tlen--;
						is_double = true;
					}
					StringBuffer buf = new StringBuffer();
					for (int i = 0; i < tlen; i++) 
						buf.append(cfetch(tok +i*sCHAR));
					try {
						int n = Integer.parseInt(buf.toString(), temp_base);
						if (ua_get(STATE) == 0) {
							push(n);
							if (is_double) push(n < 0 ? -1 : 0);
						} else {
							literal(n);
							literal(n < 0 ? -1 : 0);
						}
					} catch(NumberFormatException e1) {
						try {
							float r = Float.parseFloat(buf.toString());
							if (ua_get(STATE) == 0) {
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
	void _unused_() { push(m.get(d).remaining()); }
	void _refill_() {
		// TODO
	}
	void _save_input_() {
		push(ua_get(SOURCE_POS));
		push(ua_get(SOURCE_ID));
		push(ua_get(IBUF));
		push(ua_get(IPOS));
		push(ua_get(ILEN));
		push(5);
	}
	void _restore_input_() {
		pop();
		ua_set(ILEN, pop());
		ua_set(IPOS, pop());
		ua_set(IBUF, pop());
		ua_set(SOURCE_ID, pop());
		ua_set(SOURCE_POS, pop());
		if (ua_get(SOURCE_ID) > 0) {
			// TODO Move file cursor and fill buffer
		}
		push(0);
	}
	void _included_() {
		// TODO 
	}
	void _move_() {
		int u = pop();
		int addr2 = pop();
		int addr1 = pop();
		if (addr1 >= addr2) {
			for (int i = 0; i < u; i++) 
				cstore(addr2 + i*sCHAR, cfetch(addr1 + i*sCHAR));
		} else {
			for (int i = u - 1; i >= 0; i--)
				cstore(addr2 + i*sCHAR, cfetch(addr1 + i*sCHAR));
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
	void _r_shift_() { int n = pop(); push(pop() >> n); }
	void _star_() { int n = pop(); push(pop() * n); }
	void _two_slash_() { push(pop() >> 1); }
	void _u_m_star_() { long u = upop() * upop(); dpush(u); }
	void _u_m_slash_mod_() { 
		long u = upop(); 
		long d = dpop();
		push((int)Long.remainderUnsigned(d, u));
		push((int)Long.divideUnsigned(d, u));
	}

	void _c_fetch_() { push(cfetch(pop())); }
	void _c_store_() { int a = pop(); cstore(a, (char)pop()); }
	void _fetch_() { push(fetch(pop())); }
	void _store_() { int a = pop(); store(a, pop()); }

	void _equals_() { int n = pop(); push(pop() == n ? -1 : 0); }
	void _less_than_() { int n = pop(); push(pop() < n ? -1 : 0); }

	// Defining routines
	void _colon_() {
		push(32); _word_();
		int tok = pick(0) + sCHAR;
		int tlen = cfetch(pop());
		header(tok, tlen);
		ua_set(LATESTXT, get_xt(get_latest()));
		set_flag(get_latest(), HIDDEN);
		ua_set(STATE, 1);
	}
	void _colon_no_name_() {
		push(here());
		ua_set(LATESTXT, here());
		ua_set(STATE, 1);
	}
	void _semicolon_() {
		compile(get_xt(find_word("EXIT")));
		ua_set(STATE, 0);
		// Don't change flags for nonames
		if (get_xt(get_latest()) == ua_get(LATESTXT)) {
			unset_flag(get_latest(), HIDDEN);
		}
	}
	void _recurse_() { compile(ua_get(LATESTXT)); }
	void _catch_() { _catch(pop()); }
	void _throw_() {
		int e = pop();
		if (e != 0) {
			if (e == -2) {
				// If it's ABORT" print the message
				int l = pop();
				int a = pop();
				StringBuffer buf = new StringBuffer();
				for (int i = 0; i < l; i++) buf.append(cfetch(a + i*sCHAR));
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
	void _chars_() { push(pop() * sCHAR); }
	void _compile_comma_() { compile(pop()); }
	void _create_name_() {
		int tlen = pop();
		header(pop(), tlen);
		compile(get_xt(find_word("(RIP")));
		compile(4*sCELL);
		compile(get_xt(find_word("EXIT")));
		compile(get_xt(find_word("EXIT")));
	}
	void _create_() {
		push(32); _word_();
		int c = pop();
		push(c + sCHAR);
		push(cfetch(c));
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

		int previbuf = ua_get(IBUF);
		int previpos = ua_get(IPOS);
		int previlen = ua_get(ILEN);

		int prevsourceid = ua_get(SOURCE_ID);

		ua_set(SOURCE_ID, -1);
		ua_set(IBUF, a);
		ua_set(IPOS, 0);
		ua_set(ILEN, l);

		_catch(ua_get(INTERPRET));

		ua_set(SOURCE_ID, prevsourceid);
		ua_set(IBUF, previbuf);
		ua_set(IPOS, previpos);

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
		int tok = pick(0) + sCHAR;
		int tlen = cfetch(pick(0));
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
	void _source_() { push(ua_get(IBUF)); push(ua_get(ILEN)); }

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
	void ua_variable(String name, int d, int v) {
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
		ua_set(CURRENT, to_abs(INTERNAL_WL, d));
		code("(LIT)", primitive((vm) -> _lit_()));

		// TODO Reorder user area variables to not need changing
		// between wordlists

		// User Area Variables
		// NOTE What is stored in (CURRENT) here will affect later
		// use of hedader
		ua_variable("(CURRENT)", CURRENT, to_abs(INTERNAL_WL, d));
		ua_variable("#ORDER", ORDER, 2);
		ua_variable("(LOCALS-WORDLIST)", LOCALS_WORDLIST, 0);
		ua_variable("CONTEXT", CONTEXT, to_abs(FORTH_WL, d));
		ua_set(CURRENT, to_abs(FORTH_WL, d));
		ua_variable("BASE", BASE, 10);
		ua_variable("STATE", STATE, 0);
		ua_set(CURRENT, to_abs(INTERNAL_WL, d));
		ua_variable("(IBUF)", IBUF, 0);
		ua_set(CURRENT, to_abs(FORTH_WL, d));
		ua_variable(">IN", IPOS, 0);
		ua_set(CURRENT, to_abs(INTERNAL_WL, d));
		ua_variable("(ILEN)", ILEN, 0);
		ua_variable("(SOURCE-ID)", SOURCE_ID, 0);
		ua_variable("(SOURCE-POS)", SOURCE_POS, 0);
		ua_variable("(LATESTXT)", LATESTXT, 0);
		ua_variable("(INTERPRET)", INTERPRET, 0);
		ua_variable("(SLOTH_ROOT_PATH_LENGTH)", ROOT_PATH_LENGTH, 0);
		ua_variable("(SLOTH_PATH_START)", PATH_START, to_abs(PATHS, u));
		ua_variable("(SLOTH_PATH_END)", PATH_END, to_abs(PATHS, u));
		ua_variable("(SLOTH_PATHS)", PATHS, 0);
		ua_variable("(INCLUDED-FILES)", INCLUDED_FILES, 0);

		// Primitives
		code("(RIP)", primitive((vm) -> _rip_()));
		code("(BRANCH)", primitive((vm) -> _branch_()));
		code("(?BRANCH)", primitive((vm) -> _zbranch_()));
		code("(STRING)", primitive((vm) -> _string_()));
		code("(CSTRING)", primitive((vm) -> _c_string_()));
		code("(QUOTATION)", primitive((vm) -> _quotation_()));
		code("(DOES)", primitive((vm) -> _do_does_()));
		code("(ENVIRONMENT)", primitive((vm) -> _environment_()));

		ua_set(CURRENT, to_abs(FORTH_WL, d));

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
		code(";", primitive((vm) -> _semicolon_()));

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
		ua_set(INTERPRET, primitive((vm) -> _interpret_()));
		code("(EMPTY-RETURN-STACK)", primitive((vm) -> _empty_rs_()));
	}

	void repl() {
		m.add(ByteBuffer.allocate(256));
		ua_set(IBUF, to_abs(0, m.size() - 1));
		ua_set(IPOS, 0);
		ua_set(ILEN, 80);
		eval(get_xt(find_word("QUIT")));
		// TODO Maybe remove the memory block?
	}

	void evaluate(String c) {
		m.add(ByteBuffer.allocate(256));
		m.get(m.size() - 1).clear();
		for(int i = 0; i < c.length(); i++) {
			m.get(m.size() - 1).putChar(c.charAt(i));
		}
		push(to_abs(0, m.size() - 1));
		push(c.length());
		_evaluate_();
	}
} 
