// Virtual Forth computer and API
// jordipbou, 2024

package io.github.jordipbou;

import java.net.URL;
import java.io.File;
import java.util.List;
import java.util.ArrayList;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.function.Consumer;
import java.io.FileNotFoundException;

public class Sloth {
	// --------------------------------------------------------------------------

	// Constructor

	public Sloth() {
		s = new int[256];
		sp = 0;
		r = new int[256];
		rp = 0;
		o = new Object[256];
		op = 0;
		mem = ByteBuffer.allocateDirect(65536);
		omem = new Object[65536 >> 3];
		store(STATE, 0);
		here(0);
		there(MARGIN);
		ip = -1;
		primitives = new ArrayList<Consumer<Sloth>>();
	}

	// --------------------------------------------------------------------------

	// Data stack

	public int[] s;			// Data stack
	public int sp;			// Data stack pointer
	
	// Helpers to get the most/least significant bits from a long integer, 
	// used for working with double cell numbers which are a requirement for
	// ANS Forth (even when not implementing the optional Double-Number word
	// set).
	public long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	public long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }

	// Push/pop variants
	public void dpush(long v) { push(v); push(v >> 32); }
	public void push(long v) { push((int)lsp(v)); }
	public void push(int v) { s[sp++] = v; }
	public long dpop() { long b = lpop(); return msp(b << 32) + lsp(lpop()); }
	public long lpop() { return (long)s[--sp]; }
	public long upop() { return Integer.toUnsignedLong(s[--sp]); }
	public long udpop() { long b = upop(); return msp(b << 32) + lsp(upop()); }
	public int pop() { return s[--sp]; }

	// Pick from the stack
	public int P(int a) { return s[sp - a - 1]; }
	public void P(int a, int v) { s[sp - a - 1] = v; }

	public void dup() { push(P(0)); }
	public void over() { push(P(1)); }
	public void swap() { int a = pop(); int b = pop(); push(a); push(b); };

	// --------------------------------------------------------------------------

	// Return stack

	public int[] r;			// Return stack
	public int rp;			// Return stack pointer

	public void rpush(int v) { r[rp++] = v; }
	public int rpop() { return r[--rp]; }

	// --------------------------------------------------------------------------

	// Floating point stack

	public double[] f;	// Floating point stack
	public int fp;			// Floating point stack pointer

	public void fpush(double v) { f[fp++] = v; }
	public double fpop() { return f[--fp]; }

	public double fP(int a) { return f[fp - a - 1]; }
	public void fP(int a, double v) { f[fp - a - 1] = v; }

	// --------------------------------------------------------------------------

	// Object stack

	public Object[] o;	// Object stack
	public int op;			// Object stack pointer

	public void opush(Object v) { o[op++] = v; }
	public Object opop() { return o[--op]; }

	public Object oP(int a) { return o[op - a - 1]; }
	public void oP(int a, Object v) { o[op - a - 1] = v; }

	// --------------------------------------------------------------------------

	// Memory

	public static int CELL = 4;
	public static int CHAR = 2;

	public static int MARGIN = 512;

	public ByteBuffer mem;
	public Object[] omem;

	public int fetch(int a) { return mem.getInt(a); }
	public void store(int a, int v) { mem.putInt(a, v); }
	public char cfetch(int a) { return mem.getChar(a); }
	public void cstore(int a, char v) { mem.putChar(a, v); }

	public double ffetch(int a) { return mem.getDouble(a); }
	public void fstore(int a, double v) { mem.putDouble(a, v); }

	public Object ofetch(int a) { return omem[a >> 3]; }
	public void ostore(int a, Object v) { omem[a >> 3] = v; }

	public int here() { return fetch(HERE); }
	public void here(int v) { store(HERE, v); }

	public int there() { return fetch(THERE); }
	public void there(int v) { store(THERE, v); }

	public void allot(int v) { here(here() + v); }
	public void align() { here((here() + (CELL - 1)) & ~(CELL - 1)); }

	// If state is negative, everything is compiled to transient memory.
	public void comma(int v) { 
		if (fetch(STATE) >= 0) {
			store(here(), v); allot(CELL); 
		} else {
			store(there(), v); tallot(CELL);
		}
	}
	public void ccomma(char v) { 
		if (fetch(STATE) >= 0) {
			cstore(here(), v); allot(CHAR);
		} else {
			cstore(there(), v); tallot(CHAR);
		}
	}

	public void compile(int v) { comma(v); }
	public void literal(int v) { compile(doLIT); compile(v); }

	public int tallot(int v) { 
		if (there() < (here() + MARGIN)) there(here() + MARGIN);
		if ((there() + v) > mem.capacity()) there(here() + MARGIN);
		int x = there();
		there(there() + v);
		return x;
	}
	public void talign() { there((there() + (CELL - 1)) & ~(CELL - 1)); }

	public void str_to_transient(String str) {
		int t = tallot(str.length());
		push(t);
		for (int i = 0; i < str.length(); i++) cstore(t + (i * CHAR), str.charAt(i));
		push(str.length());
	}

	public String data_to_str(int a, int u) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < u; i++) sb.append(cfetch(a + (i * CHAR)));
		return sb.toString();
	}

	// --------------------------------------------------------------------------

	// Inner interpreter

	public int ip;	// Instruction pointer

	public List<Consumer<Sloth>> primitives;

	public int token() { int v = fetch(ip); ip += CELL; return v; }
	public boolean valid_ip() { return ip >= 0 && ip < mem.capacity(); }
	public boolean tail() { return !valid_ip() || fetch(ip) == EXIT; }
	public void do_prim(int p) { primitives.get(-1 - p).accept(this); }
	// NOTE: Tail call optimization has failed when 2R> was implemented directly in SLOTH.
	// It also fails with words that end on r> when executed in interpretation mode in the REPL.
	public void call(int q) { if (!tail()) rpush(ip); ip = q; }
	public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	public void inner() {	int t = rp; while (t <= rp && valid_ip()) { /* trace(); System.out.println(); */ execute(token()); } }
	public void eval(int q) { execute(q); inner(); }
	public void exit() { if (rp > 0) ip = rpop(); else ip = -1; }

	public void trace() {
		System.out.printf("[%d] ", fetch(STATE));
		System.out.printf("IP::%d ", ip);
		xt_to_name(fetch(ip)); type(); System.out.printf(" ");
		dump_stack();
	}

	public void xt_to_name(int v) {
		int w = latest();
		while (w != 0) {
			if (xt(w) == v) { push(name(w)); push(namelen(w)); return; }
			w = link(w);
		}
		push(0); push(0);
	}

	// **************************************************************************

	// Bootstrapping

	// --------------------------------------------------------------------------

	// Forth state

	public static int LATEST = 0*CELL;
	public static int LATESTXT = 1*CELL;
	public static int STATE = 2*CELL;
	public static int HERE = 3*CELL;
	public static int THERE = 4*CELL;
	public static int SOURCE_ID = 5*CELL; 
	public static int IPOS = 6*CELL;
	public static int BASE = 7*CELL;
	public static int CURRENT = 8*CELL;

	// Words implemented in Java that can be reassigned to Forth code

	public static int EMIT = 9*CELL;
	public static int KEY = 10*CELL;
	public static int ACCEPT = 11*CELL;
	public static int INTERPRET = 12*CELL;

	public static int LAST_VAR = 13*CELL;

	// --------------------------------------------------------------------------

	// Dictionary

	public static char HIDDEN = 1;
	public static char COLON = 2;
	public static char IMMEDIATE = 4;

	public int latest() { return fetch(LATEST); }
	public void latest(int v) { store(LATEST, v); }

	public int latestxt() { return fetch(LATESTXT); }
	public void latestxt(int v) { store(LATESTXT, v); }

	public int xt(int w) { return fetch(w + CELL); }
	public void xt(int w, int v) { store(w + CELL, v); }

	public int dt(int w) { return fetch(w + CELL + CELL); }
	public void dt(int w, int v) { store(w + CELL + CELL, v); }

	public int link(int w) { return fetch(w); }

	public int wordlist(int w) { return fetch(w + CELL + CELL + CELL); }
	public void wordlist(int w, int v) { store(w + CELL + CELL + CELL, v); }

	public char flags(int w) { return cfetch(w + CELL + CELL + CELL + CELL); }
	public void flags(int w, char v) { cstore(w + CELL + CELL + CELL + CELL, v); }
	public void set_flag(int w, char v) { flags(w, (char)(flags(w) | v)); }
	public void unset_flag(int w, char v) { flags(w, (char)(flags(w) & ~v)); }
	public boolean has_flag(int w, char v) { return (flags(w) & v) == v; }

	public char namelen(int w) { return cfetch(w + CELL + CELL + CELL + CELL + CHAR); }
	public int name(int w) { return w + CELL + CELL + CELL + CELL + CHAR + CHAR; }

	public void immediate() { set_flag(latest(), (char)(flags(latest()) | IMMEDIATE)); }

	public void header(String s) { str_to_transient(s); header(); }

	// TODO Add current wordlist to header
	public void header() {
		int l = pop();
		int a = pop();
		align();
		int w = here();		// Address of NT/XT
		comma(latest()); latest(w);	// LINK
		comma(0);										// XT
		comma(0);										// DT
		comma(fetch(CURRENT));			// WORDLIST
		ccomma((char)0);						// FLAGS
		ccomma((char)l);						// NAME LENGTH
		for (int i = 0; i < l; i++) ccomma(cfetch(a + (i * CHAR)));	// Name
		align();
		dt(latest(), here());
	}

	public boolean compare(int n, int l, int w) {
		if (l != namelen(w)) return false;
		for (int i = 0; i < l; i++) {
			char a = cfetch(n + (i * CHAR));
			char b = cfetch(name(w) + (i * CHAR));
			if (a >= 97 && a <= 122) a -= 32;
			if (b >= 97 && b <= 122) b-= 32;
			if (a != b) return false;
		}
		return true;
	}

	public void find_name() {
		int l = pop(); 
		int n = pop();
		int w = latest();
		while (w != 0) {
			if (compare(n, l, w) && !has_flag(w, HIDDEN)) break;
			w = link(w);
		}
		push(w);
	}

	// --------------------------------------------------------------------------

	// Input buffer

	public int ibuf;
	public int ilen;

	public String last_dir = "";
	public BufferedReader last_buffered_reader;

	public void key() {
		if (fetch(KEY) != 0) eval(fetch(KEY));
		else {
			try {
				push(System.in.read());
			} catch (IOException e) {
				push(0);
			}
		}
	}

	public void emit() {
		if (fetch(EMIT) != 0) eval(fetch(EMIT));
		else System.out.printf("%c", (char)pop());
	}

	public void accept() {
		if (fetch(ACCEPT) != 0) eval(fetch(ACCEPT));
		else if (fetch(KEY) == 0) {
			String s = System.console().readLine();
			int l = pop();
			int a = pop();
			for (int i = 0; i < s.length() && i < l; i++) 
				cstore(a + (i * CHAR), s.charAt(i));
			push(s.length());
		} else {
			int l = pop();
			int a = pop();
			int n = 0;
			while (l > 0) {
				key();
				char c = (char)pop();
				if (c == 10 || c == 13) { 
					push(n); return;
				} else if (c == 127) { 
					if (n > 0) {
						n--;
						l++;
						a--;
						push(8); emit();
						push(32); emit();
						push(8); emit();
					}
				} else {
					cstore(a, c);
					n++;
					a++;
					l--;
				}
			}
		}
	}

	public void type() {
		int l = pop();
		int a = pop();
		for (int i = 0; i < l; i++) { 
			push(cfetch(a + (i * CHAR))); emit(); 
		}
	}

	public void source_id() { push(SOURCE_ID); }
	public void refill() {
		switch (fetch(SOURCE_ID)) {
			case -1: push(-1); break;
			case 0: 
				ibuf = tallot(80);
				push(ibuf); 
				push(80); 
				eval(fetch(ACCEPT)); 
				ilen = pop();
				store(IPOS, 0);
				push(-1); 
				break;
			default: 
				try {
					str_to_transient(last_buffered_reader.readLine());
					ilen = pop();
					ibuf = pop();
					store(IPOS, 0);
					push(-1);
				} catch (Exception e) {
					push(0);
				}
		}
	}

	public void include(String filename) throws FileNotFoundException, IOException {
	 	File file = new File(filename);
		if (!file.exists()) {
			file = new File(last_dir + filename);
			if (!file.exists()) {
				throw new FileNotFoundException();
			}
		}

		String prevDir = last_dir;
		last_dir = file.getAbsolutePath().replace(file.getName(), "");

		BufferedReader prevReader = last_buffered_reader;
	 	last_buffered_reader = new BufferedReader(new InputStreamReader(new FileInputStream(file)));

		int last_source_id = fetch(SOURCE_ID);
		store(SOURCE_ID, 1);

	 	while (true) {
	 		String line = last_buffered_reader.readLine();
	 		if (line == null) break;
			System.out.println(line);
			str_to_transient(line);
			ilen = pop();
			ibuf = pop();
			store(IPOS, 0);
			try {
				interpret();
			} catch(Exception e) {
				System.out.printf("Exception on line: [%s]\n", line);
				e.printStackTrace();
				break;
			}
	 	}

		store(SOURCE_ID, last_source_id);
		last_buffered_reader.close();
		last_buffered_reader = prevReader;
		last_dir = prevDir;
	}

	// --------------------------------------------------------------------------

	// Parsing

	public void parse_name() {
		int ipos = fetch(IPOS);
		while (ipos < ilen && cfetch(ibuf + (ipos * CHAR)) < 33) ipos++;
		push(ibuf + (ipos * CHAR));
		while (ipos < ilen && cfetch(ibuf + (ipos * CHAR)) > 32) ipos++;
		push((ibuf + (ipos * CHAR) - P(0)) / CHAR);
		store(IPOS, ipos + (ipos < ilen ? 1 : 0));
	}

	// --------------------------------------------------------------------------

	// Definitions

	public void maybe_colon() { 
		parse_name();
		find_name(); 
		store(IPOS, pop() == 0 ? 1 : ilen); 
	}

	public void colon() {
		parse_name();
		header();
		xt(latest(), dt(latest()));
		latestxt(xt(latest()));
		set_flag(latest(), COLON);
		set_flag(latest(), HIDDEN);
		store(STATE, 1);
	}

	public void semicolon() { 
		compile(EXIT); 
		store(STATE, 0); 
		if (xt(latest()) == latestxt())	// Check that it's not a :NONAME definition
			unset_flag(latest(), HIDDEN); 
	};
	public void create() { parse_name(); header(); }
	public void does() { literal(here() + 16); compile(doDOES); compile(EXIT); }

	// public void noname() {
	// 	align();

	// 	// align();
	// 	// latestxt(here());
	// 	// comma(here() + 2*CELL);	// XT
	// 	// comma(here() + CELL);		// DT
	// 	// store(STATE, 1);
	// }

	// public void start_quotation() {
	// 	align();
	// 	postpone(doBLOCK);
	// 	push(here); comma(0); // >MARK
	// 	// align();
	// 	// latestxt(here());
	// 	// comma(here() + 2*CELL);	// XT
	// 	// comma(here() + CELL);		// DT
	// 	// store(STATE, 1);
	// }

	// public void end_quotation() {
	// 	postpone(EXIT);
	// 	// here over - swap !
	// 	int a = pop();
	// 	store(a, here() - a);
	// }

	// --------------------------------------------------------------------------

	// Outer interpreter and evaluation

	public void evaluate(String str) { str_to_transient(str); evaluate(); }
	public void evaluate() {
		int l = ilen;
		int b = ibuf;
		int p = fetch(IPOS);
		int source_id = fetch(SOURCE_ID);
		ilen = pop();
		ibuf = pop();
		store(IPOS, 0);
		store(SOURCE_ID, -1);
		interpret();
		store(SOURCE_ID, source_id);
		store(IPOS, p);
		ibuf = b;
		ilen = l;
	}

	// TODO Add recognizers to the interpreter directly here !!! (Why not?!)
	public void interpret() {
		if (fetch(INTERPRET) != 0) eval(fetch(INTERPRET));
		else {
			while (true) {
				parse_name(); 
				System.out.printf("TOKEN ["); over(); over(); type(); System.out.printf("]\n");
				if (s[sp - 1] == 0) { pop(); pop(); return; }
				int caddr = s[sp - 2]; int u = s[sp - 1];
				find_name(); int w = pop();
				if (w != 0) {
					System.out.printf("WORD FOUND W::%d XT(W)::%d DT(W)::%d\n", w, xt(w), dt(w));
					if (fetch(STATE) == 0 || has_flag(w, IMMEDIATE)) {
						System.out.println("INTERPRETING");
						if (!has_flag(w, COLON)) push(dt(w));
						if (xt(w) != 0) eval(xt(w));
					} else {
						System.out.println("COMPILING");
						if (!(has_flag(w, COLON))) { literal(dt(w)); } // comma(doLIT); comma(dt(w)); }
						if (xt(w) != 0) compile(xt(w)); // comma(xt(w));
					}
				} else {
					System.out.println("WORD NOT FOUND, CONVERTING TO NUMBER");
					StringBuffer sb = new StringBuffer();
					for (int i = 0; i < u; i++) sb.append(cfetch(caddr + (i * CHAR)));
					int n = (int)Long.parseLong(sb.toString(), fetch(BASE));
					if (fetch(STATE) == 0) push(n);
					else { comma(doLIT); comma(n); }
				}
			}
		}
	}

	// --------------------------------------------------------------------------

	// Include



	// --------------------------------------------------------------------------

	// Bootstrapping

	public static int EXIT;
	public static int doLIT;
	public static int doDOES;
	public static int doBLOCK;
	public static int COMPILE;
	public static int BRANCH;
	public static int zBRANCH;

	public void primitive(String s, Consumer<Sloth> c) {
		primitives.add(c);
		header(s);
		xt(latest(), 0 - primitives.size());
		set_flag(latest(), COLON);
	}

	public void bootstrap() {
		allot(LAST_VAR);
		store(LATEST, 0);
		store(STATE, 0);
		store(SOURCE_ID, 0);
		store(IPOS, 0);
		store(BASE, 10);
		store(CURRENT, 1);

		store(EMIT, 0);
		store(KEY, 0);
		store(ACCEPT, 0);
		store(INTERPRET, 0);

		// This primitives are used inside Java code. We to store
		// its xt on variables.
		primitive("EXIT", (vm) -> exit());
		EXIT = xt(latest());
		primitive("doLIT", (vm) -> push(token()));
		doLIT = xt(latest());
		primitive("doDOES", (vm) -> xt(latest(), pop()));
		doDOES = xt(latest());
		primitive("COMPILE,", (vm) -> comma(pop()));
		COMPILE = xt(latest());

		primitive("BRANCH", (vm) -> ip += token());
		BRANCH = xt(latest());
		primitive("?BRANCH", (vm) -> { if (pop() == 0) ip += token(); else ip += CELL; });
		zBRANCH = xt(latest());

		primitive("doBLOCK", (vm) -> { int v = token(); push(ip); ip += v - CELL; });
		doBLOCK = xt(latest());

		primitive("doSTRING", (vm) -> { int l = token(); push(ip); push(l); ip += (l + (CELL - 1)) & ~(CELL - 1);	});

		primitive("EXECUTE", (vm) -> execute(pop()));
		primitive("RECURSE", (vm) -> compile(latestxt()));
		primitive(":NONAME", (vm) -> { push(here()); latestxt(here()); store(STATE, 1); });
		primitive("BYE", (vm) -> System.exit(0));

		// Forth state ------------------------------------------------------------

		primitive("LATEST", (vm) -> push(LATEST));
		primitive("LATESTXT", (vm) -> push(LATESTXT));
		primitive("STATE", (vm) -> push(STATE));
		primitive("HERE", (vm) -> push(HERE));
		primitive("THERE", (vm) -> push(THERE));
		primitive("SOURCE-ID!", (vm) -> store(SOURCE_ID, pop()));
		primitive("SOURCE", (vm) -> { push(ibuf); push(ilen); });
		primitive(">IN", (vm) -> push(IPOS));
		primitive("BASE", (vm) -> push(BASE));
		primitive("CURRENT", (vm) -> push(CURRENT));

		primitive("(INTERPRET)", (vm) -> push(INTERPRET));

		// Return stack manipulation ----------------------------------------------

		primitive("RP@", (vm) -> push(rp));
		primitive("RP!", (vm) -> rp = pop());
		primitive(">R", (vm) -> rpush(pop()));
		primitive("R>", (vm) -> push(rpop()));

		// Data stack manipulation ------------------------------------------------

		primitive("SP@", (vm) -> push(sp));
		primitive("SP!", (vm) -> sp = pop());
		primitive("DROP", (vm) -> pop());
		primitive("DUP", (vm) -> dup());
		primitive("OVER", (vm) -> over());
		primitive("SWAP", (vm) -> swap());
		primitive("PICK", (vm) -> push(P(pop())));

		// Memory operations ------------------------------------------------------

		primitive("HERE", (vm) -> push(here()));
		primitive("ALLOT", (vm)-> allot(pop()));
		primitive("UNUSED", (vm) -> push(mem.capacity() - here()));
		primitive("@", (vm) -> push(fetch(pop())));
		primitive("!", (vm) -> { int a = pop(); int v = pop(); store(a, v); });
		primitive("C@", (vm) -> push(cfetch(pop())));
		primitive("C!", (vm) -> { int a = pop(); int v = pop(); cstore(a, (char)v); });
		primitive("CELLS", (vm) -> push(pop() * CELL));
		primitive("CHARS", (vm) -> push(pop() * CHAR));

		// Bit operations ---------------------------------------------------------

		primitive("0<", (vm) -> { int v = pop(); push(v < 0 ? -1 : 0); });
		primitive("AND", (vm) -> { int v = pop(); int w = pop(); push(w & v); });
		primitive("INVERT", (vm) -> { int v = pop(); push(~v); });
		primitive("RSHIFT", (vm) -> { int a = pop(); int b = pop(); push(b >>> a); });
		primitive("2/", (vm) -> { int a = pop(); push(a >> 1); });

		// Required Arithmetic operations -----------------------------------------

		primitive("-", (vm) -> { int a = pop(); int b = pop(); push(b - a); });
		primitive("*/MOD", (vm) -> { long n = lpop(); long d = lpop() * lpop(); push(d % n); push(d / n); });
		primitive("UM*", (vm) -> { long r = upop() * upop(); dpush(r); });
		primitive("UM/MOD", (vm) -> { long u = upop(); long d = dpop(); push(Long.remainderUnsigned(d, u)); push(Long.divideUnsigned(d, u)); });

		// Additional Arithmetic operations ---------------------------------------

		primitive("+", (vm) -> { int a = pop(); int b = pop(); push(b + a); });

		// Definitions ------------------------------------------------------------

		primitive("?:", (vm) -> maybe_colon());
		primitive(":", (vm) -> colon());
		primitive(";", (vm) -> semicolon()); immediate(); 
		primitive("CREATE", (vm) -> create());
		primitive("DOES>", (vm) -> does());	immediate();
		primitive("IMMEDIATE", (vm) -> immediate());
		primitive("'", (vm) -> { parse_name(); find_name(); push(xt(pop())); });
		primitive("POSTPONE", (vm) -> {
			parse_name();
			find_name();
			int w = pop();
			if (has_flag(w, IMMEDIATE)) {	compile(xt(w));	} 
			else { literal(xt(w)); compile(COMPILE);	}
		}); immediate();

		// primitive("IMMEDIATE-FLAG", (vm) -> push(IMMEDIATE));
		// primitive("COLON-FLAG", (vm) -> push(COLON));
		// primitive("HIDDEN-FLAG", (vm) -> push(HIDDEN));

		primitive("NT>LINK", (vm) -> link(pop()));
		primitive("NT>XT", (vm) -> xt(pop()));
		primitive("NT>DT", (vm) -> dt(pop()));
		primitive("NT>WORDLIST", (vm) -> wordlist(pop()));
		primitive("NT>FLAGS", (vm) -> flags(pop()));
		primitive("NT>NAME", (vm) -> { int v = pop(); push(name(v)); push(namelen(v)); });
		primitive("NAME>STRING", (vm) -> { int v = pop(); push(name(v)); push(namelen(v)); });

		primitive("IMMEDIATE?", (vm) -> has_flag(pop(), IMMEDIATE));
		primitive("COLON?", (vm) -> has_flag(pop(), COLON));
		primitive("HIDDEN?", (vm) -> has_flag(pop(), HIDDEN));

		// Evaluation -------------------------------------------------------------

		primitive("EVALUATE", (vm) -> evaluate());
		primitive("INCLUDED", (vm) -> { 
			int l = pop(); 
			int a = pop(); 

			// TODO: Manage exceptions
			try {
				include(data_to_str(a, l)); 
			} catch(Exception e) {
				e.printStackTrace();
			}
		});

		// Control structures -----------------------------------------------------

		// This two are only needed for DO/LOOP implementation, but maybe DO/LOOP should
		// be a primitive.
		primitive(">MARK", (vm) -> { push(here()); comma(0); });
		primitive(">>RESOLVE", (vm) -> store(pop(), here()));

		primitive("AHEAD", (vm) -> { compile(BRANCH); push(here()); comma(0); }); immediate();
		primitive("IF", (vm) -> { compile(zBRANCH); push(here()); comma(0); }); immediate();
		primitive("THEN", (vm) -> { int a = pop(); store(a, here() - a); }); immediate();

		primitive("BEGIN", (vm) -> push(here())); immediate();
		primitive("UNTIL", (vm) -> { compile(zBRANCH); comma(pop() - here()); }); immediate();
		primitive("AGAIN", (vm) -> { compile(BRANCH); comma(pop() - here()); }); immediate();

		// Quotations -------------------------------------------------------------

		primitive("[:", (vm) -> { compile(doBLOCK); push(here()); comma(0); }); immediate();
		primitive(";]", (vm) -> { compile(EXIT); int a = pop(); store(a, here() - a); }); immediate();

		// Input/output -----------------------------------------------------------

		// TODO These two must be deferred ?
		primitive("EMIT", (vm) -> emit());
		primitive("KEY", (vm) -> key());
		primitive("REFILL", (vm) -> refill());
		primitive("ACCEPT", (vm) -> accept());
		primitive("(ACCEPT)", (vm) -> push(ACCEPT));

		// Tools ------------------------------------------------------------------

		primitive(".S", (vm) -> dump_stack());

		// Error management -------------------------------------------------------
	
		// TODO CATCH THROW
	}

	// --------------------------------------------------------------------------

	// Utilities

	public void dump_stack() {
		// Temporary, should use EMIT/TYPE
		System.out.printf("<%d> ", sp);
		for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
	}

	// --------------------------------------------------------------------------

	// Simple REPL

	public static void main(String[] args) {
		Sloth x = new Sloth();
		x.bootstrap();

		try {
			URL url = Thread.currentThread().getContextClassLoader().getResource("sloth.4th");
			x.include(url.getFile());
		} catch(IOException e) {
		 	e.printStackTrace();
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
}
