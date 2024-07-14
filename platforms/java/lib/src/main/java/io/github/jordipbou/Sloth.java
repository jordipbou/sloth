package io.github.jordipbou;

import java.net.URL;
import java.io.File;
import java.util.HashMap;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.function.Consumer;
import java.io.FileNotFoundException;

public class Sloth {
	// --------------------------------------------------------------------------
	// -- Virtual Machine -------------------------------------------------------
	// --------------------------------------------------------------------------

	// The virtual machine is a typical two stack Forth machine, but it knows
	// nothing of the language implemented on top of it. It does not need
	// bootstrapping and can load an image file and execute directly from it.

	// -- Debugging -------------------------------------------------------------

	// The tr register is used to set the level of debugging, helpful during
	// the development of the virtual machine and the bootstrapping of Sloth.

	int tr;

	// -- Parameter stack -------------------------------------------------------

	// The parameter stack is implemented with a Java array. Its size is defined
	// on the constructor.

	int s[];
	int sp;

	public void push(int v) { s[sp++] = v; }
	public int pop() { return s[--sp]; }

	public void place(int a, int v) { s[sp - a - 1] = v; }
	public int pick(int a) { return s[sp - a - 1]; }

	// Swap is a very basic function needed inside Java code to define functions
	// that interact with the Sloth environment.
	public void swap() { int t = pick(0); place(0, pick(1)); place(1, t); }

	// -- Address stack ---------------------------------------------------------

	int r[];
	int rp;

	public void rpush(int v) { r[rp++] = v; }
	public int rpop() { return r[--rp]; }

	public void rplace(int a, int v) { r[rp - a - 1] = v; }
	public int rpick(int a) { return r[rp - a - 1]; }

	// -- Locals stack ----------------------------------------------------------

	int l[];
	int lp;

	public void lpush(int v) { l[lp++] = v; }
	public int lpop() { return l[--lp]; }

	public void lplace(int a, int v) { l[lp - a - 1] = v; }
	public int lpick(int a) { return l[lp - a - 1]; }

	// -- Floating point stack --------------------------------------------------

	float f[];
	int fp;

	public void fpush(float v) { f[fp++] = v; }
	public float fpop() { return f[--fp]; }

	// -- Objects stack ---------------------------------------------------------

	// Being a Java implementation of Sloth, it needs access to Java objects to
	// be really useful as an scripting language for Java applications.

	Object o[];
	int op;

	public void opush(Object v) { o[op++] = v; }
	public Object opop() { return o[--op]; }

	// -- Memory ----------------------------------------------------------------

	public int CELL = 4;
	public int CHAR = 2;
	public int BYTE = 1;

	ByteBuffer m;
	int mp;

	public int fetch(int a) { return m.getInt(a); }
	public void store(int a, int v) { m.putInt(a, v); }

	public char cfetch(int a) { return m.getChar(a); }
	public void cstore(int a, char v) { m.putChar(a, v); }

	public int here() { return mp; }
	public int allot(int v) { int t = mp; mp += v; return t; }
	public void align() { allot(((here() + (CELL - 1)) & ~(CELL - 1)) - here()); }
	public int aligned(int a) { return ((a + (CELL - 1)) & ~(CELL - 1)); }

	int tp, tx;

	public int there() { return tp; }
	public int tallot(int v) { tp -= v; return tp; }
	public void tfree() { tp = tx; }

	// -- Objects memory --------------------------------------------------------

	// As Java objects can not be stored as an integer (or long), an hashtable
	// of number->object is created to map memory addresses inside Sloth to
	// Java objects.

	HashMap<Integer, Object> om;

	public Object ofetch(int a) { return om.get(a); }
	public void ostore(int a, Object v) { om.put(a, v); }

	// -- Inner interpreter -----------------------------------------------------

	int ip;

	ArrayList<Consumer<Sloth>> primitives;

	// NONAME adds an anonymous primitive that can be called by its xt (position on primitives list)
	public int noname(Consumer<Sloth> c) { primitives.add(c); return 0 - primitives.size(); }

	public int opcode() { int v = fetch(ip); ip += CELL; return v; }
	public void do_prim(int p) { primitives.get(-1 - p).accept(this); }
	// TODO: Test tail call optimization, it failed in some cases (related to R> at the end)
	public void call(int q) { rpush(ip); ip = q; }

	public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	public void exit() { if (rp > 0) ip = rpop(); else ip = -1; }

	// The interaction between Sloth and Java is complicated, and this has been
	// designed to allow calling from Sloth to Java primitives and from that
	// primitives to Sloth code again without breaking anything. To be able to
	// do that, the inner interpreter only exists to the level of the return
	// address in which it started. If the return address pointer goes below (by
	// calling exit) it just returns from the function to Java code again.
	// This is really helpful to create combinators that call Sloth code from
	// Java.
	public void inner() {	int t = rp; while (t <= rp && ip >= 0) { trace(3); execute(opcode()); } }

	// Eval allows calling Sloth code from Java.
	public void eval(int q) { execute(q); inner(); }

	// -- Exceptions ------------------------------------------------------------

	// This implementation of Forth exceptions allow its use both in Sloth and
	// inside Java code.

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

	// -- Save/load image -------------------------------------------------------

	// TODO Useful to create turnkey applications ?

	// --------------------------------------------------------------------------
	// -- Dictionary ------------------------------------------------------------
	// --------------------------------------------------------------------------

	// This dictionary implementation does not need to know how the virtual
	// machine works, it just needs access to fetch/store/cfetch/cstore/here/allot
	// to work on the memory of the virtual machine.

	int current = 1, latest = 0, latestxt = 0, latestwid = 1;

	// ORDER is the xt of an executable order (its executed to search one
	// or more wordlists). Just by modifying it from SLOTH its possible to reuse
	// the find function in Java without having to implement a search order.
	int ORDER = 0;

	public static char HIDDEN = 1;
	public static char COLON = 2;
	public static char IMMEDIATE = 4;
	public static char INSTANTANEOUS = 8;

	public void comma(int v) { store(here(), v); allot(CELL); }
	public void ccomma(char v) { cstore(here(), v); allot(CHAR); }

	public void compile(int v) { comma(v); }

	public int link(int w) { return fetch(w); }

	public int xt(int w) { return fetch(w + CELL); }
	public void xt(int w, int v) { store(w + CELL, v); }

	public int dt(int w) { return fetch(w + CELL + CELL); }
	public void dt(int w, int v) { store(w + CELL + CELL, v); }

	public int get_wl(int w) { return fetch(w + CELL + CELL + CELL); }
	public void set_wl(int w, int v) { store(w + CELL + CELL + CELL, v); }

	public char flags(int w) { return cfetch(w + CELL + CELL + CELL + CELL); }
	public void flags(int w, char v) { cstore(w + CELL + CELL + CELL + CELL, v); }
	public void set_flag(int w, char v) { flags(w, (char)(flags(w) | v)); }
	public void unset_flag(int w, char v) { flags(w, (char)(flags(w) & ~v)); }
	public boolean has_flag(int w, char v) { return (flags(w) & v) == v; }

	public void set_colon() { set_flag(latest, COLON); }
	public void set_immediate() { set_flag(latest, IMMEDIATE); }
	public void set_instantaneous() { set_flag(latest, INSTANTANEOUS); }

	public char namelen(int w) { return cfetch(w + CELL + CELL + CELL + CELL + CHAR); }
	public int name(int w) { return w + CELL + CELL + CELL + CELL + CHAR + CHAR; }

	public void start_header() {
		align();
		int w = here();	comma(latest); latest = w;	// LINK (and NT address)
		comma(0);																		// XT
		comma(0);																		// DT
		comma(current);															// WORDLIST
		ccomma((char)0);														// Flags
	}

	// Having two different functions to compile the header name, one based
	// on a Java String and one on Sloth string (c-addr/u) allows not needing
	// to use an input buffer to copy the Java String to it on the bootstrapping
	// process. At first, that was the implementation used and as >IN was a 
	// problem as had to be bootstrapped and was needed for bootstrapping itself.
	public void header_name(String s) {
		ccomma((char)s.length());
		for (int i = 0; i < s.length(); i++) ccomma(s.charAt(i));
	}

	public void header_name() { 
		int l = pop(), a = pop();
		ccomma((char)l);
		for (int i = 0; i < l; i++) ccomma(cfetch(a + (i * CHAR)));
	}

	public void end_header() {
		align();
		dt(latest, here());
	}

	public void header(String s) { start_header(); header_name(s); end_header(); }
	public void header() { start_header(); header_name(); end_header(); }

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

	public void find_name_in() {
		int wid = pop(), l = pop(), a = pop(), w = latest;
		try {
			while (w != 0) {
				if (get_wl(w) == wid && compare_without_case(a, l, w) && !has_flag(w, HIDDEN))
					break;
				w = link(w);
			}
			push(w);
		} catch(Exception e) {
			System.out.printf("WORD THAT GETS ERROR: %d\n", w);
			e.printStackTrace();
			System.out.printf("WORDS:\n");
			w = latest;
			while (w != 0) {
				System.out.printf("%d %d ", w, link(w));
				for (int i = 0; i < namelen(w); i++)
					System.out.printf("%c", cfetch(name(w) + i*CHAR));
				System.out.printf(" %d\n", get_wl(w));
				w = link(w);
			}
		}
	}

	public void find_name() { eval(ORDER); }

	// --------------------------------------------------------------------------
	// -- Bootstrapping ---------------------------------------------------------
	// --------------------------------------------------------------------------

	// With the implementation of a basic virtual machine and the implementation
	// of the dicionary on top of it, we have enough structure to start the
	// bootstrapping process. More functions will be added as required to be
	// able to bootstrap a full Forth environment from source code.

	// This helper function will create a named variable in the Forth environemnt.

	public int variable(String s) { header(s); allot(CELL); return dt(latest); }

	// The next function helps with the creation of colon definitions from Java
	// code.
	// This words will be primitives in the Forth environment and can be called
	// both from Forth and from Java (with eval)

	public int colon(String s, Consumer<Sloth> c) { return colon(s, noname(c)); }

	public int colon(String s, int xt) {
		header(s);
		xt(latest, xt);
		latestxt = xt;
		set_colon();
		return xt;
	}

	// Now, all the required primitives and the required variables will be
	// created.

	// This method has to be called after creating an Sloth instance except if
	// an already bootstrapped image is loaded to the virtual machine.

	public void bootstrap() {
		// First word can not be at 0, as it will not be found, so lets reserve
		// two cells.
		allot(2*CELL);

		// This are the primitives that need to be known from Java to be able to
		// compile them to Sloth code.
		EXIT = colon("EXIT", (vm) -> exit());

		NOOP = colon("NOOP", (vm) -> { /* do nothing */ });

		LIT = colon("LIT", (vm) -> push(opcode()));

		STRING = colon("STRING", (vm) -> { 
			int l = opcode(); 
			push(ip); 
			push(l); 
			ip = aligned(ip + (l * CHAR)); 
		});

		QUOTATION = colon("QUOTATION", (vm) -> { 
			int l = opcode(); 
			push(ip); 
			ip += l; 
		});

		BRANCH = colon("BRANCH", (vm) -> { 
			int l = opcode(); 
			ip += l - CELL; 
		});

		zBRANCH = colon("?BRANCH", (vm) -> { 
			int l = opcode(); 
			if (pop() == 0) ip += l - CELL; 
		});

		COMPILE = colon("COMPILE,", (vm) -> compile(pop()));

		// Default search order just searches on wordlist 1
		ORDER = noname((vm) -> { push(1); find_name_in(); });

		IPOS = variable(">IN"); store(IPOS, 0);
		STATE = variable("STATE"); store(STATE, 0);
		BASE = variable("BASE"); store(BASE, 10);

		// --
		colon("TRACE", (vm) -> push(tr));
		colon("TRACE!", (vm) -> { tr = pop(); });
		colon("BYE", (vm) -> System.exit(0));

		// -- Stacks --
		colon("DROP", (vm) -> pop());
		colon("PICK", (vm) -> push(pick(pop())));
		colon("OVER", (vm) -> push(pick(1)));
		colon("SWAP", (vm) -> swap());

		colon(">R", (vm) -> rpush(pop()));
		colon("R@", (vm) -> push(rpick(0)));
		colon("R>", (vm) -> push(rpop()));

		colon("DEPTH", (vm) -> push(sp));

		// -- Arithmetic --
		colon("+", (vm) -> { int a = pop(); int b = pop(); push(b + a); });
		colon("-", (vm) -> { int a = pop(); int b = pop(); push(b - a); });
		colon("*", (vm) -> { int a = pop(); int b = pop(); push(b * a); });
		colon("/", (vm) -> { int a = pop(); int b = pop(); push(b / a); });
		colon("MOD", (vm) -> { int a = pop(); int b = pop(); push(b % a); });

		// -- Comparison --
		colon("<", (vm) -> { int a = pop(); int b = pop(); push(b < a ? -1 : 0); });
		colon("=", (vm) -> { int a = pop(); int b = pop(); push(b == a ? -1 : 0); });
		colon(">", (vm) -> { int a = pop(); int b = pop(); push(b > a ? -1 : 0); });

		// -- Bits --
		colon("AND", (vm) -> { int a = pop(); int b = pop(); push(b & a); });
		colon("OR", (vm) -> { int a = pop(); int b = pop(); push(b | a); });
		colon("XOR", (vm) -> { int a = pop(); int b = pop(); push(b ^ a); });
		colon("INVERT", (vm) -> push(~pop()));

		// -- Definitions and execution --
		colon(":", (vm) -> colon());
		colon(";", (vm) -> semicolon()); set_immediate();
		colon("POSTPONE", (vm) -> postpone()); set_immediate();
		colon("IMMEDIATE", (vm) -> set_immediate()); 
		colon("[:", (vm) -> start_quotation()); set_immediate();
		colon(";]", (vm) -> end_quotation()); set_immediate();
		colon("EXECUTE", (vm) -> eval(pop()));
		colon("RECURSE", (vm) -> compile(xt(latest))); set_immediate();
		colon("CREATE", (vm) -> create());
		DOES = noname((vm) -> xt(latest, pop()));
		colon("DOES>", (vm) -> does()); set_immediate(); 
		colon("[", (vm) -> store(STATE, 0)); set_immediate();
		colon("]", (vm) -> store(STATE, 1));
		colon("]]", (vm) -> store(STATE, 2)); set_immediate();
		// TODO An internal variable could be used to store previous state and
		// restore it when doing ]] [[
		colon("[[", (vm) -> store(STATE, 1)); set_instantaneous();

		// -- Exceptions --
		colon("THROW", (vm) -> _throw(pop()));
		colon("CATCH", (vm) -> _catch(pop()));

		// -- Memory access and compilation --
		colon("@", (vm) -> push(fetch(pop())));
		colon("!", (vm) -> { int a = pop(); store(a, pop()); });
		colon("C@", (vm) -> push(cfetch(pop())));
		colon("C!", (vm) -> { int a = pop(); cstore(a, (char)pop()); });

		colon("HERE", (vm) -> push(here()));
		colon("ALLOT", (vm) -> allot(pop()));
		colon("ALIGN", (vm) -> align());
		colon("ALIGNED", (vm) -> push(aligned(pop())));

		colon("THERE", (vm) -> push(there()));
		colon("TALLOT", (vm) -> push(tallot(pop())));
		colon("TFREE", (vm) -> tfree());

		colon(",", (vm) -> comma(pop()));
		colon("C,", (vm) -> ccomma((char)pop()));

		colon("CELLS", (vm) -> push(pop()*CELL));
		colon("CHARS", (vm) -> push(pop()*CHAR));

		// -- Input buffer and parsing --
		colon("SOURCE", (vm) -> { push(ibuf); push(ilen); });
		colon("SOURCE!", (vm) -> { int l = pop(); int a = pop(); ibuf = a; ilen = l; });

		colon("REFILL", (vm) -> refill());

		colon("PARSE-NAME", (vm) -> parse_name());
		colon("PARSE", (vm) -> parse());

		colon("INCLUDED", (vm) -> included());

		// -- Search and dictionary helpers --
		colon("FIND-NAME", (vm) -> find_name());
		colon("FIND-NAME-IN", (vm) -> find_name_in());

		colon("SET-ORDER-XT", (vm) -> { ORDER = pop(); });

		colon("WORDLIST", (vm) -> push(++latestwid));
		colon("SET-CURRENT", (vm) -> { current = pop(); });
		colon("GET-CURRENT", (vm) -> push(current));

		colon("NT>XT", (vm) -> push(xt(pop())));
		colon("IMMEDIATE?", (vm) -> push(has_flag(pop(), IMMEDIATE) ? -1 : 0));

		// -- Combinators --
		colon("CHOOSE", (vm) -> choose());
		colon("WHEN", (vm) -> when());
		colon("UNLESS", (vm) -> unless());

		colon("TIMES", (vm) -> times());
		colon("I", (vm) -> push(ix));
		colon("LEAVE", (vm) -> leave());
		colon("DOLOOP", (vm) -> doloop());

		// -- Strings --
		colon("S\"", (vm) -> start_string()); set_immediate();

		// -- Input/Output --
		colon("KEY", (vm) -> key());
		colon("EMIT", (vm) -> emit());
		colon("ACCEPT", (vm) -> accept());
	}

	// -- Forth primitives ------------------------------------------------------

	// This variables store the XT (or DT for variables) that need to be used
	// from Java.

	int EXIT; 
	int NOOP; 
	int LIT; 
	int STRING; 
	int QUOTATION; 
	int BRANCH; 
	int zBRANCH; 
	int DOES;
	int COMPILE;
	int STATE;

	// -- Quotations and definitions --

	// level stores the current nesting of quotations. If a quotation is
	// created on the REPL, then it has to return to an interpretation state
	// on close but if a quotation is created inside a colon definition (or
	// inside another quotation) then it must keep current state.

	int level;

	public void start_quotation() {
		level++;
		if (level == 1) store(STATE, 1);
		push(latestxt);
		compile(QUOTATION);
		push(here());
		comma(0);
		latestxt = here();
	}

	public void end_quotation() {
		compile(EXIT);
		int a = pop();
		store(a, here() - a  - CELL);
		latestxt = pop();
		level--;
		if (level == 0) store(STATE, 0);
	}

	public void literal(int v) { comma(LIT); comma(v); }

	public void create() { parse_name(); header(); }
	public void does() { literal(here() + 16); compile(DOES); compile(EXIT); }

	public void colon() { 
		parse_name();
		header(); 
		xt(latest, dt(latest));
		latestxt = xt(latest);
		set_flag(latest, COLON);
		set_flag(latest, HIDDEN);
		store(STATE, 1);
		level = 1;
	}

	public void semicolon() { 
		compile(EXIT); 
		store(STATE, 0);
		level = 0;
		// Don't do this for nonames
		if (xt(latest) == latestxt) unset_flag(latest, HIDDEN); 
	};

	public void postpone() {
		parse_name();
		find_name();
		int w = pop();
		if (has_flag(w, IMMEDIATE)) {	compile(xt(w));	} 
		else { literal(xt(w)); compile(COMPILE);	}
	}

	// -- Input buffer and parsing --

	int ibuf, ilen, IPOS, tok, tlen;

	public void set_ipos(int v) { store(IPOS, v); }
	public int get_ipos() { return fetch(IPOS); }
	public void inc_ipos() { set_ipos(get_ipos() + 1); }

	public void parse_name() {
		while (get_ipos() < ilen && cfetch(ibuf + (get_ipos()*CHAR)) <= 32) inc_ipos();
		tok = ibuf + (get_ipos()*CHAR);
		push(tok);
		tlen = 0;
		while (get_ipos() < ilen && cfetch(ibuf + (get_ipos()*CHAR)) > 32) { inc_ipos(); tlen++; }
		push(tlen);
		if (get_ipos() < ilen) inc_ipos();
	}

	public void parse() {
		char c = (char)pop();
		push(ibuf + (get_ipos()*CHAR));
		while (get_ipos() < ilen && cfetch(ibuf + (get_ipos()*CHAR)) != c) inc_ipos();
		push(((ibuf + (get_ipos()*CHAR)) - pick(0)) / CHAR);
		if (get_ipos() < ilen) inc_ipos();
	}

	// -- Conversion between Java Strings and Forth c-addr/u strings --

	public void str_to_transient(String s) {
		int a = tallot(s.length()*CHAR);
		str_to_data(s, a);
		ibuf = a;
		ilen = s.length();
		set_ipos(0);
	}

	public void str_to_data(String s, int a) {
		for (int i = 0; i < s.length(); i++) cstore(a + (i*CHAR), s.charAt(i));
	}

	public String data_to_str(int a, int l) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < l; i++) sb.append(cfetch(a + i*CHAR));
		return sb.toString();
	}

	// -- String definition --

	public void start_string() {
		push('"'); parse(); int l = pop(), a = pop();
		if (fetch(STATE) != 0) {
			compile(STRING);
			comma(l);
			for (int i = 0; i < l; i++) ccomma(cfetch(a + (i*CHAR)));
			align();
		} else {
			int d = tallot(l*CHAR);
			push(d); push(l);
			for (int i = 0; i < l; i++) cstore(d + i*CHAR, cfetch(a + (i*CHAR)));
		}
	}

	// -- Input/output --

	public int KEY = 0;

	public void key() {
		if (KEY != 0) eval(KEY);
		else try { push(System.in.read()); } catch (IOException e) { e.printStackTrace(); };
	}

	public int ACCEPT = 0;

	public void accept() {
		if (ACCEPT != 0) eval(ACCEPT);
		else {
			try {
				BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
				String line = reader.readLine();
				str_to_transient(line);
				push(line.length());
			} catch(IOException e) {
				// TODO Should this throw a Sloth exception?
				e.printStackTrace();
			}
		}
	}

	public int EMIT = 0;

	public void emit() {
		if (EMIT != 0) eval(EMIT);
		else System.out.printf("%c", (char)pop());
	}

	public void type() {
		int l = pop(), a = pop(); 
		for (int i = 0; i < l; i++) 
			System.out.printf("%c", cfetch(a + i*CHAR));
	}

	public String last_dir = "";
	public BufferedReader last_buffered_reader;

	int source_id;

	public void refill() {
	switch (source_id) {
		case -1: push(-1); break;
		case 0:
			push(ibuf); push(80);
			accept();
			ilen = pop();
			set_ipos(0);
			push(-1); 
			break;
		default: 
			try {
				str_to_transient(last_buffered_reader.readLine());
				push(-1);
			} catch (Exception e) {
				push(0);
			}
		}
	}

	public void included() {
		int l = pop(), a = pop();
		try {
			include(data_to_str(a, l));
		} catch (FileNotFoundException e) {
			_throw(-38);
		} catch (IOException e) {
			_throw(-37);
		}
	}

	public void include(String filename) throws FileNotFoundException, IOException {
		// save previous input buffer, by writing after it
		int previbuf = ibuf;
		int previpos = get_ipos();
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
			str_to_transient(line);
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

	// -- Outer interpreter --

	public int BASE;

	public void interpret() {
		while (ilen > 0) {
			parse_name();
			trace(2);
			if (tlen == 0) { pop(); pop(); break; }
			find_name(); int w = pop();
			if (w != 0) {
 				if (fetch(STATE) == 0
				|| (fetch(STATE) == 1 && has_flag(w, IMMEDIATE)) 
				|| (fetch(STATE) == 2 && has_flag(w, INSTANTANEOUS))) {
					// TODO This should be something like name>interpret
 					if (!has_flag(w, COLON)) push(dt(w));
 					if (xt(w) != 0) eval(xt(w));
 				} else {
					// TODO This should be something like name>compile
 					if (!has_flag(w, COLON)) literal(dt(w));
 					if (xt(w) != 0) compile(xt(w));
 				}
			} else {
 				StringBuilder sb = new StringBuilder();
 				for (int i = 0; i < tlen; i++) sb.append(cfetch(tok + (i*CHAR)));
 				try {
 					int n = Integer.parseInt(sb.toString(), fetch(BASE));
 					if (fetch(STATE) == 0) push(n);
 					else literal(n);
 				} catch (NumberFormatException e) {
					_throw(-13);
 				}
			}
		}
		trace(1);
	}

	// --------------------------------------------------------------------------
	// -- Combinators -----------------------------------------------------------
	// --------------------------------------------------------------------------

	public void choose() { int f = pop(); int t = pop(); if (pop() != 0) eval(t); else eval(f); }
	public void when() { int q = pop(); if (pop() != 0) eval(q); }
	public void unless() { int q = pop(); if (pop() == 0) eval(q); }

	int ix, jx, kx;	// Loop registers
	int lx; // Leave register

	public void ipush() { rpush(kx); kx = jx; jx = ix; lx = 0; }
	public void ipop() { lx = 0; ix = jx; jx = kx; kx = rpop(); }

	public void leave() { lx = -1; exit(); }

	public void times() { 
		ipush(); 
		int q = pop(), l = pop(); 
		for (ix = 0; ix < l && lx == 0; ix++) 
			eval(q); 
		ipop(); 
	}

	// Algorithm for doloop taken from pForth (pf_inner.c case ID_PLUS_LOOP)
	public void doloop() {
		ipush(); 
		int q = pop();
		ix = pop();
		int l = pop();
		if (ix != l) {
			for (int o = ix - l, d = 0;((o ^ (o + d)) & (o ^ d)) >= 0 && lx == 0;) {
				eval(q);
				if (lx == 0) { // Avoid pop if we're leaving
					d = pop();
					o = ix - l;
					ix += d;
				}
			}
		}
		ipop(); 
	}

	// --------------------------------------------------------------------------
	// -- Debugging -------------------------------------------------------------
	// --------------------------------------------------------------------------

	public void trace(int l) {
		if (l <= tr) {
			System.out.printf("L:%d TR:%d %d:%d {%d} <%d> ", l, tr, mp, tp, fetch(STATE), sp);
			for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
			if (ip >= 0) 
				System.out.printf(": [%d] %d ", ip, ip > 0 ? fetch(ip) : 0);
			for (int i = rp - 1; i >= 0; i--) 
				if (r[i] != -1)
					System.out.printf(": [%d] %d ", r[i], r[i] > 0 ? fetch(r[i]) : r[i]);
			System.out.printf("|| ");
			if (tlen > 0) {
				System.out.printf(" **");
				for (int i = 0; i < tlen; i++)
					System.out.printf("%c", cfetch(tok + (i*CHAR)));
				System.out.printf("**");
			}
			if (ilen > 0)
				for (int i = get_ipos(); i < ilen; i++) 
					System.out.printf("%c", cfetch(ibuf + (i*CHAR)));
			System.out.println();
		}
	}

	// --------------------------------------------------------------------------
	// -- Constructors ----------------------------------------------------------
	// --------------------------------------------------------------------------

	public Sloth(int ssz, int rsz, int lsz, int osz, int fsz, int msz) {
		s = new int[ssz];
		r = new int[rsz];
		l = new int[lsz];
		o = new Object[osz];
		f = new float[fsz];
		m = ByteBuffer.allocateDirect(msz);
		sp = rp = lp = op = fp = 0;
		mp = 0;
		tp = tx = msz;
		ibuf = ilen = 0;
		primitives = new ArrayList<Consumer<Sloth>>();
		ip = -1;
		source_id = 0;
		current = 1;
		latest = 0;
		latestxt = 0;
		latestwid = 1;
		tr = 3;
	}

	public Sloth() { this(64, 256, 32, 32, 32, 65536); }

	// --------------------------------------------------------------------------
	// -- REPL ------------------------------------------------------------------
	// --------------------------------------------------------------------------

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

		// x.trace(3);

		try {
			BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
			while (true) {
				try {
					String line = reader.readLine();
					x.str_to_transient(line);
					x.interpret();
				} catch(SlothException e) {
					if (e.v == -13) { x.push(x.tok); x.push(x.tlen); x.type(); System.out.println(" ?"); }
				}
			}
		} catch(IOException e) {
			e.printStackTrace();
		}
	}

}
