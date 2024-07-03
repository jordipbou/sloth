package io.github.jordipbou;

import java.net.URL;
import java.io.File;
import java.util.List;
import java.nio.ByteBuffer;
import java.util.Hashtable;
import java.util.ArrayList;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.function.Consumer;
import java.io.FileNotFoundException;

import org.jline.terminal.Terminal;
import org.jline.terminal.TerminalBuilder;
import org.jline.utils.NonBlockingReader;
import org.jline.reader.LineReader;
import org.jline.reader.LineReaderBuilder;
import org.jline.reader.impl.DefaultParser;

public class Sloth {

	// --------------------------------------------------------------------------
	// ** Virtual machine *******************************************************
	// --------------------------------------------------------------------------

	// -- Debugging --

	int tr;

	// -- Parameter stack --

	int s[];
	int sp;

	public void push(int v) { s[sp++] = v; }
	public int pop() { return s[--sp]; }

	public void place(int a, int v) { s[sp - a - 1] = v; }
	public int pick(int a) { return s[sp - a - 1]; }

	public void swap() { int t = pick(0); place(0, pick(1)); place(1, t); }

	// -- Address stack --

	int r[];
	int rp;

	public void rpush(int v) { r[rp++] = v; }
	public int rpop() { return r[--rp]; }

	public void rplace(int a, int v) { r[rp - a - 1] = v; }
	public int rpick(int a) { return r[rp - a - 1]; }

	// -- Locals stack --

	int l[];
	int lp;

	public void lpush(int v) { l[lp++] = v; }
	public int lpop() { return l[--lp]; }

	public void lplace(int a, int v) { l[lp - a - 1] = v; }
	public int lpick(int a) { return l[lp - a - 1]; }

	// -- Memory --

	public int CELL = 4;
	public int CHAR = 2;

	ByteBuffer d;
	int dp;
	int tp;

	int state;

	public int fetch(int a) { return d.getInt(a); }
	public void store(int a, int v) { d.putInt(a, v); }

	public char cfetch(int a) { return d.getChar(a); }
	public void cstore(int a, char v) { d.putChar(a, v); }

	public int here() { if (state >= 0) return dp; else return tp; }
	public void allot(int v) { if (state >= 0) { dp += v; tp += v; } else tp += v; }
	public void align() { allot(((here() + (CELL - 1)) & ~(CELL - 1)) - here()); }
	public int aligned(int a) { return ((a + (CELL - 1)) & ~(CELL - 1)); }

	// -- Inner interpreter --

	int ip;

	ArrayList<Consumer<Sloth>> primitives;

	public int opcode() { int v = fetch(ip); ip += CELL; return v; }
	public void do_prim(int p) { primitives.get(-1 - p).accept(this); }
	// TODO: Think about tail call optimization, it failed on some cases (related to R> at the end)
	public void call(int q) { rpush(ip); ip = q; }
	public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	// Will execute until returning from current level, or an exception is thrown
	public void inner() {	int t = rp; while (t <= rp && ip >= 0) { trace(2); execute(opcode()); } }
	// Eval is equivalent to execute but its meant to be called from Java, as
	// it starts a nested inner interpreter
	public void eval(int q) { execute(q); inner(); }
	public void exit() { if (rp > 0) ip = rpop(); else ip = -1; }

	// -- Exceptions --

	public class SlothException extends RuntimeException {
		public int v;

		public SlothException(int v) { this.v = v; }
	}

	public void _throw(int v) { if (v != 0) { throw new SlothException(v); } }
	public void _catch(int q) {
		int tsp = sp;
		int trp = rp;
		try { 
			execute(q); 
			push(0);
		} catch(SlothException x) {
			sp = tsp;
			rp = trp;
			push(x.v);
		} catch(Exception e) {
			e.printStackTrace();
			sp = tsp;
			rp = trp;
			push(-1000);
		}
	}

	// --------------------------------------------------------------------------
	// -- Dictionary ------------------------------------------------------------
	// --------------------------------------------------------------------------

	int current, latest, latestxt;

	public static char HIDDEN = 1;
	public static char COLON = 2;
	public static char IMMEDIATE = 4;

	public int link(int w) { return fetch(w); }

	public int xt(int w) { return fetch(w + CELL); }
	public void xt(int w, int v) { store(w + CELL, v); }

	public int dt(int w) { return fetch(w + CELL + CELL); }
	public void dt(int w, int v) { store(w + CELL + CELL, v); }

	public int wl(int w) { return fetch(w + CELL + CELL + CELL); }
	public void wl(int w, int v) { store(w + CELL + CELL + CELL, v); }

	public char flags(int w) { return cfetch(w + CELL + CELL + CELL + CELL); }
	public void flags(int w, char v) { cstore(w + CELL + CELL + CELL + CELL, v); }
	public void set_flag(int w, char v) { flags(w, (char)(flags(w) | v)); }
	public void unset_flag(int w, char v) { flags(w, (char)(flags(w) & ~v)); }
	public boolean has_flag(int w, char v) { return (flags(w) & v) == v; }

	public void set_colon() { set_flag(latest, COLON); }
	public void set_immediate() { set_flag(latest, IMMEDIATE); }

	public char namelen(int w) { return cfetch(w + CELL + CELL + CELL + CELL + CHAR); }
	public int name(int w) { return w + CELL + CELL + CELL + CELL + CHAR + CHAR; }

	public void header(String s) { str_to_ibuf(s); push(ibuf); push(ilen); header(); }
	public void header() {
		int l = pop(), a = pop();
		align();
		int w = here();																				// Address of NT/XT
		comma(latest); latest = w;														// LINK
		comma(0);																							// XT
		comma(0);																							// DT
		comma(current);																				// WORDLIST
		ccomma((char)0);																			// FLAGS
		ccomma((char)l);																			// NAME LENGTH
		for (int i = 0; i < l; i++) ccomma(cfetch(a + (i * CHAR)));	// Name
		align();
		dt(latest, here());
	}

	// -- Finding words --

	public boolean compare_without_case(int n, int l, int w) {
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

	public void find() {
		int l = pop(), n = pop(), w = latest;
		while (w != 0) {
			if (compare_without_case(n, l, w) && !has_flag(w, HIDDEN)) break;
			w = link(w);
		}
		push(w);
	}

	// -- Input buffer and parsing --

	int ibuf, ilen, ipos, tok, tlen;

	public void token() {
		while (ipos < ilen && cfetch(ibuf + (ipos*CHAR)) <= 32) ipos++;
		tok = ibuf + (ipos*CHAR);
		push(tok);
		tlen = 0;
		while (ipos < ilen && cfetch(ibuf + (ipos*CHAR)) > 32) { ipos++; tlen++; }
		push(tlen);
	}

	public void to_char() {
		char c = (char)pop();
		ipos = ipos + 1;
		push(ibuf + (ipos*CHAR));
		while (ipos < ilen && cfetch(ibuf + (ipos*CHAR)) != c) ipos++;
		push(((ibuf + (ipos*CHAR)) - pick(0)) / CHAR);
		ipos++;
	}

	public void str_to_ibuf(String s) {
		for (int i = 0; i < s.length(); i++) cstore(ibuf + (i*CHAR), s.charAt(i));
		ilen = s.length();
		ipos = 0;
	}


	// -- Compilation --

	public int EXIT;
	public int NOOP;
	public int LIT;
	public int STRING;
	public int QUOTATION;
	public int BRANCH;
	public int zBRANCH;
	public int COMPILE;
	public int DOES;

	public void comma(int v) { store(here(), v); allot(CELL); }
	public void ccomma(char v) { cstore(here(), v); allot(CHAR); }

	public void compile(int v) { comma(v); }
	public void literal(int v) { comma(LIT); comma(v); }

	// NONAME adds an anonymous primitive that can be called by its xt (position on primitives list)
	public int noname(Consumer<Sloth> c) { primitives.add(c); return 0 - primitives.size(); }

	public void create() { token(); header(); }
	public void does() { literal(here() + 16); compile(DOES); compile(EXIT); }

	public int colon(String s, int xt) { header(s); xt(latest, xt); latestxt = xt; set_colon(); return xt;	}
	public int colon(String s, Consumer<Sloth> c) { return colon(s, noname(c)); }
	public void colon() { 
		token();
		header(); 
		xt(latest, dt(latest));
		latestxt = xt(latest);
		set_flag(latest, COLON);
		set_flag(latest, HIDDEN);
		state = 1;
	}

	public void semicolon() { 
		compile(EXIT); 
		state = 0;
		// Don't do this for nonames
		if (xt(latest) == latestxt) unset_flag(latest, HIDDEN); 
	};

	public void postpone() {
		token();
		find();
		int w = pop();
		if (has_flag(w, IMMEDIATE)) {	compile(xt(w));	} 
		else { literal(xt(w)); compile(COMPILE);	}
	}

	// -- Outer interpreter --

	public void interpret() {
		while (ilen > 0) {
			token();
			trace(1);
			if (tlen == 0) { pop(); pop(); break; }
			find(); int w = pop();
			if (w != 0) {
 				if (state == 0 || has_flag(w, IMMEDIATE)) {
 					if (!has_flag(w, COLON)) push(dt(w));
 					if (xt(w) != 0) eval(xt(w));
 				} else {
 					if (!has_flag(w, COLON)) literal(dt(w));
 					if (xt(w) != 0) compile(xt(w));
 				}
			} else {
 				StringBuilder sb = new StringBuilder();
 				for (int i = 0; i < tlen; i++) sb.append(cfetch(tok + (i*CHAR)));
 				try {
 					int n = Integer.parseInt(sb.toString(), 10);
 					if (state == 0) push(n);
 					else literal(n);
 				} catch (NumberFormatException e) {
 					// Throw word not found exception
 					e.printStackTrace();
 				}
			}
		}
		trace(0);
	}

	// -- Constructors --

	public Sloth(int ibufsize, int ssize, int rsize, int lsize, int dsize) {
		s = new int[ssize];
		r = new int[rsize];
		l = new int[lsize];
		d = ByteBuffer.allocateDirect(dsize);
		sp = rp = lp = state = 0;
		dp = ibufsize;
		tp = dsize / 2;
		ibuf = ipos = ilen = 0;
		primitives = new ArrayList<Consumer<Sloth>>();
		ip = -1;
		source_id = 0;
		tr = 3;
	}

	public Sloth() { this(1024, 256, 256, 16, 65536); }

	// -- Bootstrapping --

	public void start_string() {
		push('"'); to_char(); int l = pop(), a = pop();
		int old_state = state;
		if (state == 0) state = -1;
		compile(STRING);
		comma(l);
		push(here());
		push(l);
		for (int i = 0; i < l; i++) ccomma(cfetch(a + (i*CHAR)));
		align();
		state = old_state;
	}

	public void start_quotation() {
		state += state > 0 ? 1 : -1 ;
		if (state == -1) push(here() + (2*CELL)); 
		push(latestxt);
		compile(QUOTATION);
		push(here());
		comma(0);
	}

	public void end_quotation() {
		compile(EXIT);
		int a = pop();
		store(a, here() - a  - CELL);
		latestxt = pop();
		state += state < 0 ? 1 : -1 ;
	}

	int ix, jx, kx;	// Loop registers
	int lx; // Leave register

	public void ipush() { rpush(kx); kx = jx; jx = ix; lx = 0; }
	public void ipop() { lx = 0; ix = jx; jx = kx; kx = rpop(); }

	// // Algorithm for doloop taken from pForth (pf_inner.c case ID_PLUS_LOOP)
	// public void doloop() {
	// 	ipush(); 
	// 	int q = pop();
	// 	ix = pop();
	// 	int l = pop();
	// 	if (ix != l) {
	// 		for (int o = ix - l, d = 0;((o ^ (o + d)) & (o ^ d)) >= 0 && lx == 0;) {
	// 			eval(q);
	// 			if (lx == 0) { // Avoid pop if we're leaving
	// 				d = pop();
	// 				o = ix - l;
	// 				ix += d;
	// 			}
	// 		}
	// 	}
	// 	ipop(); 
	// }

	public void leave() { lx = -1; exit(); }
	public void qleave() { if (pop() != 0) leave(); }
	public void call_leave() { int q = pop(); if (pop() != 0) { eval(q); leave(); } }

	public void times() { ipush(); int q = pop(), l = pop(); for (ix = 0; ix < l && lx == 0; ix++) eval(q); ipop(); }

	public void loop() { ipush(); int q = pop(); while (lx == 0) eval(q); ipop(); }

	public void choose() { int f = pop(); int t = pop(); if (pop() != 0) eval(t); else eval(f); }
	public void when() { int q = pop(); if (pop() != 0) eval(q); }
	public void unless() { int q = pop(); if (pop() == 0) eval(q); }

	public void bootstrap() {
		// Reserve space for the input buffer
		allot(1024);

		// TODO: The basic primitives should be instantiated in the constructor.
		EXIT = colon("EXIT", (vm) -> exit());
		NOOP = colon("NOOP", (vm) -> { /* do nothing */ });
		LIT = colon("LIT", (vm) -> push(opcode()));
		STRING = colon("STRING", (vm) -> { int l = opcode(); push(ip); push(l); ip = aligned(ip + (l * CHAR)); });
		QUOTATION = colon("QUOTATION", (vm) -> { int l = opcode(); push(ip); ip += l; });
		BRANCH = colon("BRANCH", (vm) -> { int l = opcode(); ip += l - CELL; });
		zBRANCH = colon("?BRANCH", (vm) -> { int l = opcode(); if (pop() == 0) ip += l - CELL; });

		COMPILE = colon("COMPILE,", (vm) -> compile(pop()));

		colon("'", (vm) -> { token(); find(); push(xt(pop())); });

		colon("S\"", (vm) -> start_string()); set_immediate();
		colon("[", (vm) -> start_quotation()); set_immediate();
		colon("]", (vm) -> end_quotation()); set_immediate();
		colon("EXECUTE", (vm) -> eval(pop()));
		colon("RECURSE", (vm) -> compile(xt(latest))); set_immediate();

		colon("TRACE", (vm) -> push(tr));
		colon("TRACE!", (vm) -> { tr = pop(); });

		colon("BYE", (vm) -> System.exit(0));

		colon("THROW", (vm) -> _throw(pop()));
		colon("CATCH", (vm) -> _catch(pop()));

		colon("DROP", (vm) -> pop());
		colon("PICK", (vm) -> push(pick(pop())));
		colon("SWAP", (vm) -> swap());

		colon(">R", (vm) -> rpush(pop()));
		colon("R@", (vm) -> push(rpick(0)));
		colon("R>", (vm) -> push(rpop()));

		colon("@", (vm) -> push(fetch(pop())));
		colon("!", (vm) -> { int a = pop(); store(a, pop()); });
		colon("C@", (vm) -> push(cfetch(pop())));
		colon("C!", (vm) -> { int a = pop(); cstore(a, (char)pop()); });

		colon("HERE", (vm) -> push(here()));
		colon("ALLOT", (vm) -> allot(pop()));
		colon("ALIGN", (vm) -> align());
		colon("ALIGNED", (vm) -> push(aligned(pop())));

		colon(",", (vm) -> comma(pop()));
		colon("C,", (vm) -> ccomma((char)pop()));

		colon("CELLS", (vm) -> push(pop()*CELL));
		colon("CHARS", (vm) -> push(pop()*CHAR));

		colon("INVERT", (vm) -> place(0, ~pick(0)));

		colon("<", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 < x2 ? -1 : 0); });
		colon("=", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 == x2 ? -1 : 0); });
		colon(">", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 > x2 ? -1 : 0); });

		colon("INVERT", (vm) -> push(~pop()));
		colon("AND", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 & x2); });
		colon("OR", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 | x2); });
		colon("XOR", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 ^ x2); });

		colon("+", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 + x2); });
		colon("-", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 - x2); });
		colon("*", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 * x2); });
		colon("/", (vm) -> { int x2 = pop(); int x1 = pop(); push(x1 / x2); });

		colon("CHOOSE", (vm) -> { int q2 = pop(); int q1 = pop(); eval(pop() == 0 ? q2 : q1); });

		//colon("DOLOOP", (vm) -> doloop()); // Complex counted loop

		colon("TIMES", (vm) -> times()); // Counted loop
		colon("I", (vm) -> push(ix));
		colon("J", (vm) -> push(jx));
		colon("K", (vm) -> push(kx));
		colon("I!", (vm) -> { ix = pop(); });
		colon("LEAVE", (vm) -> leave());
		colon("?LEAVE", (vm) -> qleave());
		colon("?CALL/LEAVE", (vm) -> call_leave());

		colon("LOOP", (vm) -> loop()); // Infinite loop

		colon("WHILE", (vm) -> { if (pop() != 0) leave(); });

		colon("CHOOSE", (vm) -> choose());
		colon("WHEN", (vm) -> when());
		colon("UNLESS", (vm) -> unless());

		colon("CREATE", (vm) -> create());
		DOES = noname((vm) -> xt(latest, pop()));
		colon("DOES>", (vm) -> does()); set_immediate(); 

		colon(":", (vm) -> colon());
		colon(";", (vm) -> semicolon()); set_immediate();
		colon("IMMEDIATE", (vm) -> set_immediate());
		colon("POSTPONE", (vm) -> postpone()); set_immediate();

		colon("IBUF", (vm) -> push(ibuf));
		colon("IBUF!", (vm) -> { ibuf = pop(); });
		colon("IPOS", (vm) -> push(ipos));
		colon("IPOS!", (vm) -> { ipos = pop(); });
		colon("ILEN", (vm) -> push(ilen));
		colon("ILEN!", (vm) -> { ilen = pop(); });

		colon("TOKEN", (vm) -> token());
		colon("TO/CHAR", (vm) -> to_char());
		colon("FIND", (vm) -> find());

		colon("KEY", (vm) -> key());
		colon("EMIT", (vm) -> emit());

		colon(">L", (vm) -> lpush(pop()));
		colon("L>", (vm) -> push(lpop()));
		colon("L@", (vm) -> push(lpick(pop())));
		colon("L!", (vm) -> { int a = pop(); lplace(a, pop()); });
	}

	public int KEY = 0;

	public void key() {
		if (KEY != 0) eval(KEY);
		else try { push(System.in.read()); } catch (IOException e) { e.printStackTrace(); };
	}

	public int EMIT = 0;

	public void emit() {
		if (EMIT != 0) eval(EMIT);
		else System.out.printf("%c", (char)pop());
	}

	public String last_dir = "";
	public BufferedReader last_buffered_reader;

	int source_id;

	// public void refill() {
	// switch (fetch(SOURCE_ID)) {
	// 	case -1: push(-1); break;
	// 	case 0: 
	// 		ibuf = tallot(80);
	// 		push(ibuf); 
	// 		push(80); 
	// 		accept();
	// 		ilen = pop();
	// 		store(IPOS, 0);
	// 		push(-1); 
	// 		break;
	// 	default: 
	// 		try {
	// 			str_to_transient(last_buffered_reader.readLine());
	// 			ilen = pop();
	// 			ibuf = pop();
	// 			store(IPOS, 0);
	// 			push(-1);
	// 		} catch (Exception e) {
	// 			push(0);
	// 		}
	// 	}
	// }

	public void include(String filename) throws FileNotFoundException, IOException {
		// save previous input buffer, by writing after it
		int previbuf = ibuf;
		int previpos = ipos;
		int previlen = ilen;
		ibuf = ibuf + (ilen*CHAR);

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

		int last_source_id = source_id;
		source_id = 1;

	 	while (true) {
	 		String line = last_buffered_reader.readLine();
	 		if (line == null) break;
			str_to_ibuf(line);
			try {
				interpret();
			} catch(Exception e) {
				// TODO Exceptions should be managed by the inner interpreter
				System.out.printf("Exception on line: [%s]\n", line);
				e.printStackTrace();
				break;
			}
	 	}

		source_id = last_source_id;
		last_buffered_reader.close();
		last_buffered_reader = prevReader;
		last_dir = prevDir;
	}

	// -- Testing ---------------------------------------------------------------

	public void trace(int l) {
		if (l <= tr) {
			System.out.printf("%d:%d {%d} <%d> ", dp, tp, state, sp);
			for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
			if (ip >= 0) 
				System.out.printf(": [%d] %d ", ip, ip > 0 ? fetch(ip) : 0);
			for (int i = rp - 1; i >= 0; i--) 
				if (r[i] != -1)
					System.out.printf(": [%d] %d ", r[i], r[i] > 0 ? fetch(r[i]) : r[i]);
			if (tlen > 0) {
				System.out.printf(" **");
				for (int i = 0; i < tlen; i++)
					System.out.printf("%c", cfetch(tok + (i*CHAR)));
				System.out.printf("**");
			}
			if (ilen > 0) {
				System.out.printf(" <<", ibuf, ilen);
				for (int i = ipos; i < ilen; i++) 
					System.out.printf("%c", cfetch(ibuf + (i*CHAR)));
			}
			System.out.println();
		}
	}

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

		x.trace(3);

		try {
			Terminal terminal = TerminalBuilder.terminal();
			DefaultParser parser = new DefaultParser();
			parser.setEscapeChars(null);
			terminal.enterRawMode();
			NonBlockingReader reader = terminal.reader();

			x.KEY = x.noname((vm) -> { try { vm.push(reader.read()); } catch (IOException ex) { vm.push(-1); }; });

			LineReader lineReader = LineReaderBuilder.builder().terminal(terminal).parser(parser).build();

			while (true) {
				try {
					String line = lineReader.readLine();
					x.str_to_ibuf(line);
					x.interpret();
				} catch (java.io.IOError e) {
					e.printStackTrace();
				}
			} 
		} catch (IOException e) {
			e.printStackTrace();
		}

		// try {
		// 	BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
		// 	while (true) {
		// 		String line = reader.readLine();
		// 		x.str_to_ibuf(line);
		// 		x.interpret();
		// 	}
		// } catch(IOException e) {
		// 	e.printStackTrace();
		// }
	}
}
