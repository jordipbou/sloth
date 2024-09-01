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
	// ------------------------------------------------------
	// -- Virtual Machine -----------------------------------
	// ------------------------------------------------------

	// The virtual machine is a typical two stack Forth 
	// machine, but it knows nothing of the language 
	// implemented on top of it. It can load an image file 
	// and execute directly from it.

	// -- Debugging -----------------------------------------

	// The tr register is used to set the level of debugging, 
	// helpful during the development of the virtual machine 
	// and the bootstrapping of Sloth.

	int tr;

	// -- Parameter stack -----------------------------------

	// The parameter stack is implemented with a Java array. 
	// Its size is defined on the constructor.

	int s[];
	int sp;

	public void push(int v) { s[sp++] = v; }
	public int pop() { return s[--sp]; }

	public void place(int a, int v) { s[sp - a - 1] = v; }

	// This four methods are the only stack shuffling 
	// primitives required by Sloth.
	public int pick(int a) { return s[sp - a - 1]; }
	public void drop() { pop(); }
	public void over() { push(pick(1)); }
	public void swap() { int t = pick(0); place(0, pick(1)); place(1, t); }

	// Helpers to get the most/least significant bits from a 
	// long integer, used for working with double cell 
	// numbers which are a requirement for ANS Forth (even 
	// when not implementing the optional Double-Number word
	// set! as they are required for numeric output).
	public long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	public long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }

	public void push(long v) { push((int)lsp(v)); }

	// Push/pop variants (double and unsigned numbers)
	public void dpush(long v) { push(v); push(v >> 32); }
	public long dpop() { long b = xpop(); return msp(b << 32) + lsp(xpop()); }
	public long xpop() { return (long)s[--sp]; }
	public long upop() { return Integer.toUnsignedLong(s[--sp]); }
	public long udpop() { long b = upop(); return msp(b << 32) + lsp(upop()); }

	// -- Address stack -------------------------------------

	int r[];
	int rp;

	public void rpush(int v) { r[rp++] = v; }
	public int rpop() { return r[--rp]; }

	public void rplace(int a, int v) { r[rp - a - 1] = v; }
	public int rpick(int a) { return r[rp - a - 1]; }

	public void from_r() { push(rpop()); }
	public void to_r() { rpush(pop()); }

	// -- Locals stack --------------------------------------

	int l[];
	int lp;

	public void lpush(int v) { l[lp++] = v; }
	public int lpop() { return l[--lp]; }

	public void lplace(int a, int v) { l[lp - a - 1] = v; }
	public int lpick(int a) { return l[lp - a - 1]; }

	// -- Floating point stack ------------------------------

	float f[];
	int fp;

	public void fpush(float v) { f[fp++] = v; }
	public float fpop() { return f[--fp]; }

	// -- Objects stack -------------------------------------

	// Being a Java implementation of Sloth, it needs access 
	// to Java objects to be really useful as an scripting 
	// language for Java applications.

	// TODO: Check BlueJ project and jShell to see how they
	// manage to make Java interactive.

	Object o[];
	int op;

	public void opush(Object v) { o[op++] = v; }
	public Object opop() { return o[--op]; }

	// -- Memory --------------------------------------------

	public int CELL = 4;
	public int CHAR = 2;
	public int BYTE = 1;

	ByteBuffer m;
	int mp;

	public int fetch(int a) { return m.getInt(a); }
	public void store(int a, int v) { m.putInt(a, v); }

	public char cfetch(int a) { return m.getChar(a); }
	public void cstore(int a, char v) { m.putChar(a, v); }

	public byte bfetch(int a) { return m.get(a); }
	public void bstore(int a, byte v) { m.put(a, v); }

	public int here() { return mp; }
	public int allot(int v) { int t = mp; mp += v; return t; }
	public void align() { allot(((here() + (CELL - 1)) & ~(CELL - 1)) - here()); }
	public int aligned(int a) { return ((a + (CELL - 1)) & ~(CELL - 1)); }

	int tp, tx;

	public int there() { return tp; }
	public int tallot(int v) { tp -= v; return tp; }
	public void tfree() { tp = tx; }

	// -- Objects memory ------------------------------------

	// As Java objects can not be stored as an integer 
	// (or long), a hashtable of number->object is created 
	// to map memory addresses inside Sloth to Java objects.
	// When working with Java, performance is not the main
	// concern as it will work on big processors with big
	// memory, so this should work ok.

	HashMap<Integer, Object> om;

	public Object ofetch(int a) { return om.get(a); }
	public void ostore(int a, Object v) { om.put(a, v); }

	// -- Inner interpreter ---------------------------------

	int ip;

	ArrayList<Consumer<Sloth>> primitives;

	// NONAME adds an anonymous primitive that can be called 
	// by its xt (position on primitives list)
	public int noname(Consumer<Sloth> c) { 
		primitives.add(c); 
		return 0 - primitives.size(); 
	}

	public int opcode() { int v = fetch(ip); ip += CELL; return v; }
	public void do_prim(int p) { primitives.get(-1 - p).accept(this); }
	public void call(int q) { rpush(ip); ip = q; }

	public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	public void exit() { if (rp > 0) ip = rpop(); else ip = -1; }

	// The interaction between Sloth and Java is complicated, 
	// and this has been designed to allow calling from 
	// Sloth to Java primitives and from that primitives to 
	// Sloth code again without breaking anything. 
	// To be able to do that, the inner interpreter only 
	// exits to the level of the return address in which it 
	//started. If the return address pointer goes below (by
	// calling exit) it just returns from the function to 
	// Java code again.
	// This is really helpful to create combinators that call 
	// Sloth code from Java.
	public void inner() {	
		int t = rp; 
		while (t <= rp && ip >= 0) { 
			trace(3); 
			execute(opcode()); 
		} 
	}

	// Eval allows calling Sloth code from Java.
	// When calling a Sloth word from Java, inner interpreter 
	// has to be started to continue executing all the 
	// opcodes that form that word but, if executing a 
	// primitive and the inner interpreter is called when the 
	// return stack is not empty it will continue/ executing 
	// the next opcode thinking that we are inside a non 
	// primitive word and that will be an error.
	public void eval(int q) { 
		execute(q); 
		if (q >= 0) inner(); 
	}

	// -- Exceptions ----------------------------------------

	// This implementation of Forth exceptions allow its use 
	// both in Sloth and inside Java code.
	// It just reuses the Java exception system and
	// implements an Sloth exception as a RuntimeException,
	// not needing to declare it on every method.
	// If the exception is thrown from Java with _throw or
	// from inside Sloth with throw, it will just bubble up
	// until a _catch/catch its found.

	public class SlothException extends RuntimeException {
		public int v;

		public SlothException(int v) { this.v = v; }
	}

	public void _throw(int v) { 
		if (v != 0)
			throw new SlothException(v); 
	}

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
			// Here we catch every possible exception (not just
			// the SlothExceptions) to not break the interactive
			// environment. Its up to the user to decide if the
			// exception is severe enough to restart the system.
			e.printStackTrace();
			sp = tsp;
			rp = trp;
			push(-1000);
		}
	}

	// -- Save/load image -----------------------------------

	// Useful to create turnkey applications, and to
	// test new implementations without needing the bootstrap
	// part.

	// TODO: Not implemented yet.

	// ------------------------------------------------------
	// -- Symbol table / Dictionary -------------------------
	// ------------------------------------------------------

	// This dictionary implementation does not need to know 
	// how the virtual machine works, it just needs access to 
	// fetch/store/cfetch/cstore/here/allot to work on the 
	// memory of the virtual machine.

	int current = 1, latest = 0, latestxt = 0, latestwid = 1;

	// ORDER is the xt of the default executable search order 
	// (its executed to search one or more wordlists). 
	// Just by modifying it from SLOTH its possible to reuse
	// the find function in Java without having to implement 
	// a search order.

	// TODO: I'm not totally sure this is needed, as the
	// search order will be implemented with deferred words
	// and finding should be reimplemented in Sloth anyway 
	// the less dependency with Java variables the best.
	int ORDER = 0;

	public static char HIDDEN = 1;
	public static char IMMEDIATE = 2;
	// Instantaneous flag allow use of ]] [[ without a
	// complicated implementation.
	public static char INSTANTANEOUS = 4;

	public void comma(int v) { store(here(), v); allot(CELL); }
	public void ccomma(char v) { cstore(here(), v); allot(CHAR); }

	public void compile(int v) { comma(v); }

	public int link(int w) { return fetch(w); }

	public int xt(int w) { if (w != 0) return fetch(w + CELL); else return 0; }
	public void xt(int w, int v) { store(w + CELL, v); }

	public int get_wl(int w) { return fetch(w + 2*CELL); }
	public void set_wl(int w, int v) { store(w + 2*CELL, v); }

	public char flags(int w) { return cfetch(w + 3*CELL); }
	public void flags(int w, char v) { cstore(w + 3*CELL, v); }
	public void set_flag(int w, char v) { flags(w, (char)(flags(w) | v)); }
	public void unset_flag(int w, char v) { flags(w, (char)(flags(w) & ~v)); }
	public boolean has_flag(int w, char v) { return (flags(w) & v) == v; }

	public void set_immediate() { set_flag(latest, IMMEDIATE); }
	public void set_instantaneous() { set_flag(latest, INSTANTANEOUS); }

	public char namelen(int w) { return cfetch(w + 3*CELL + CHAR); }
	public int name(int w) { return w + 3*CELL + 2*CHAR; }

	public void start_header() {
		align();
		int nt = here(); comma(latest); latest = nt;	// LINK (and NT address)
		comma(0);																		// XT
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
		xt(latest, here());
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
			for (int i = 0; i < l; i++) System.out.printf("%c", cfetch(a + i*CHAR));
			System.out.println();
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

	public int find_xt(int xt) {
		int w = latest;
		while (w != 0) {
			if (xt(w) == xt) break;
			w = link(w);
		}
		return w;
	}

	// --------------------------------------------------------------------------
	// -- Bootstrapping ---------------------------------------------------------
	// --------------------------------------------------------------------------

	// With the implementation of a basic virtual machine and the implementation
	// of the dicionary on top of it, we have enough structure to start the
	// bootstrapping process. More functions will be added as required to be
	// able to bootstrap a full Forth environment from source code.

	// This helper function will create a named variable in the Forth environemnt.

	// public int variable(String s) { header(s); allot(CELL); return dt(latest); }
	public int variable(String s) {
		header(s);
		literal(xt(latest) + 4*CELL);
		compile(EXIT);
		compile(EXIT);
		comma(0);
		return here() - CELL;
	}

	// The next function helps with the creation of colon definitions from Java
	// code.
	// This words will be primitives in the Forth environment and can be called
	// both from Forth and from Java (with eval)

	public int colon(String s, Consumer<Sloth> c) { return colon(s, noname(c)); }

	public int colon(String s, int xt) {
		header(s);
		xt(latest, xt);
		latestxt = xt;
		// set_colon();
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
		// colon("+", (vm) -> { int a = pop(); int b = pop(); push(b + a); });
		colon("-", (vm) -> { int a = pop(); int b = pop(); push(b - a); });
		// colon("*", (vm) -> { int a = pop(); int b = pop(); push(b * a); });
		// colon("/", (vm) -> { int a = pop(); int b = pop(); push(b / a); });
		// colon("MOD", (vm) -> { int a = pop(); int b = pop(); push(b % a); });
		colon("*/MOD", (vm) -> { long n = xpop(); long d = xpop() * xpop(); push(d % n); push(d / n); });
		colon("UM*", (vm) -> { long r = upop() * upop(); dpush(r); });
		colon("UM/MOD", (vm) -> { long u = upop(); long d = dpop(); push(Long.remainderUnsigned(d, u)); push(Long.divideUnsigned(d, u)); });

		// -- Comparison --
		colon("0<", (vm) -> push(pop() < 0 ? -1 : 0));
		// colon("<", (vm) -> { int a = pop(); int b = pop(); push(b < a ? -1 : 0); });
		// colon("=", (vm) -> { int a = pop(); int b = pop(); push(b == a ? -1 : 0); });
		// colon(">", (vm) -> { int a = pop(); int b = pop(); push(b > a ? -1 : 0); });

		// -- Bits --
		colon("AND", (vm) -> { int a = pop(); int b = pop(); push(b & a); });
		// colon("OR", (vm) -> { int a = pop(); int b = pop(); push(b | a); });
		// colon("XOR", (vm) -> { int a = pop(); int b = pop(); push(b ^ a); });
		colon("INVERT", (vm) -> push(~pop()));

		colon("2/", (vm) -> { int a = pop(); push(a >> 1); });
		colon("RSHIFT", (vm) -> { int a = pop(); int b = pop(); push(b >>> a); });
		colon("LSHIFT", (vm) -> { int a = pop(); int b = pop(); push(b << a); });

		colon("LATEST@", (vm) -> push(latest));
		colon("LATEST!", (vm) -> { latest = pop(); });

		// -- Definitions and execution --
		colon(":", (vm) -> colon());
		colon(";", (vm) -> semicolon()); set_immediate();
		colon("POSTPONE", (vm) -> postpone()); set_immediate();
		colon("IMMEDIATE", (vm) -> set_immediate()); 
	 	colon(":NONAME", (vm) -> { push(here()); latestxt = here(); store(STATE, 1); });
		colon("[:", (vm) -> start_quotation()); set_immediate();
		colon(";]", (vm) -> end_quotation()); set_immediate();
		EXECUTE = colon("EXECUTE", (vm) -> eval(pop()));
		colon("RECURSE", (vm) -> compile(latestxt)); set_immediate();
		colon("CREATE", (vm) -> create());
		DOES = noname((vm) -> store(xt(latest) + 2*CELL, pop()));
		colon("DOES>", (vm) -> does()); set_immediate(); 
		colon("[", (vm) -> store(STATE, 0)); set_immediate();
		colon("]", (vm) -> store(STATE, 1));
		colon("]]", (vm) -> store(STATE, 2)); set_immediate();
		// TODO An internal variable could be used to store previous state and
		// restore it when doing ]] [[
		colon("[[", (vm) -> store(STATE, 1)); set_instantaneous();

		colon("EVALUATE", (vm) -> evaluate(-1));

		// -- Exceptions --
		colon("THROW", (vm) -> _throw(pop()));
		colon("CATCH", (vm) -> _catch(pop()));

		// -- Memory access and compilation --
		colon("@", (vm) -> push(fetch(pop())));
		colon("!", (vm) -> { int a = pop(); store(a, pop()); });
		colon("C@", (vm) -> push(cfetch(pop())));
		colon("C!", (vm) -> { int a = pop(); cstore(a, (char)pop()); });
		colon("B@", (vm) -> push(bfetch(pop())));
		colon("B!", (vm) -> { int a = pop(); bstore(a, (byte)pop()); });

		colon("HERE", (vm) -> push(here()));
		colon("ALLOT", (vm) -> allot(pop()));
		// colon("ALIGN", (vm) -> align());
		// colon("ALIGNED", (vm) -> push(aligned(pop())));
		colon("UNUSED", (vm) -> push(m.capacity() - here()));

		colon("THERE", (vm) -> push(there()));
		colon("TALLOT", (vm) -> push(tallot(pop())));
		colon("TFREE", (vm) -> tfree());

		// colon(",", (vm) -> comma(pop()));
		// colon("C,", (vm) -> ccomma((char)pop()));

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

		// colon("'", (vm) -> {
		// 	parse_name();
		// 	find_name();
		// 	int nt = pop();
		// 	push(xt(nt));
		// });
		// colon("[']", (vm) -> {
		// 	parse_name();
		// 	find_name();
		// 	int nt = pop();
		// 	literal(xt(nt));
		// }); set_immediate();

		// colon("NAME>INTERPRET", (vm) -> push(xt(pop())));
		// colon("NAME>COMPILE", (vm) -> {
		// 	int nt = pop();
		// 	if (nt == 0) {
		// 		push(NOOP);
		// 	} else {
		// 		if (has_flag(nt, IMMEDIATE)) {
		// 			push(xt(nt));
		// 			push(EXECUTE);
		// 		} else {
		// 			push(xt(nt));
		// 			push(COMPILE);
		// 		}
		// 		// TODO: What should happen if it has instantaneous flag ?
		// 	}
		// });
		colon("IMMEDIATE?", (vm) -> push(has_flag(pop(), IMMEDIATE) ? -1 : 0));
		colon(">BODY", (vm) -> push(pop() + 4*CELL));

		// -- Combinators --
		colon("CHOOSE", (vm) -> choose());
		colon("WHEN", (vm) -> when());
		colon("UNLESS", (vm) -> unless());

		colon("TIMES", (vm) -> times());
		colon("I", (vm) -> push(ix));
		colon("J", (vm) -> push(jx));
		colon("K", (vm) -> push(kx));
		colon("DOLOOP", (vm) -> doloop());
		colon("LEAVE", (vm) -> leave());
		colon("UNLOOP", (vm) -> unloop());

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
	int EXECUTE;
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

	public void create() { // parse_name(); header(); }
		parse_name();
		header();
		literal(here() + 4*CELL);
		// Two exits are compiled to ensure the second one is used when
		// patching the first one with DOES>
		compile(EXIT);
		compile(EXIT);
	}
	public void does() { literal(here() + 16); compile(DOES); compile(EXIT); }

	public void colon() { 
		parse_name();
		header(); 
		// xt(latest, dt(latest));
		latestxt = xt(latest);
		// set_flag(latest, COLON);
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

	public void postpone(int nt) {
		if (has_flag(nt, IMMEDIATE)) compile(xt(nt));
		else {
			literal(xt(nt));
			compile(COMPILE);
		}
	}

	public void postpone() {
		parse_name();
		find_name();
		postpone(pop());
		// int w = pop();
		// if (has_flag(w, IMMEDIATE)) {	compile(xt(w));	} 
		// else { literal(xt(w)); compile(COMPILE);	}
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

	public void set_ibuf() { ilen = pop(); ibuf = pop(); set_ipos(0); }

	public void str_to_transient(String s) {
		int a = tallot(s.length()*CHAR);
		str_to_data(s, a);
		push(a);
		push(s.length());
		// ibuf = a;
		// ilen = s.length();
		// set_ipos(0);
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
			int l = pop(), a = pop();
			try {
				BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
				String line = reader.readLine();
				for (int i = 0; i < line.length(); i++) cstore(a + i*CHAR, line.charAt(i));
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
				set_ibuf();
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

	 	while (true) {
	 		String line = last_buffered_reader.readLine();
	 		if (line == null) break;
			try {
				evaluate(line, 1);
			} catch(Exception e) {
				System.out.printf("Exception on line: [%s]\n", line);
				if (e instanceof SlothException) {
					last_buffered_reader.close();
					last_buffered_reader = prevReader;
					last_dir = prevDir;
					throw e;
				} else {
					e.printStackTrace();
					break;
				}
			}
	 	}

		last_buffered_reader.close();
		last_buffered_reader = prevReader;
		last_dir = prevDir;
	}

	// -- Outer interpreter --

	public int BASE;

	public void interpret() {
		while (get_ipos() < ilen) {
			parse_name();
			trace(2);
			if (tlen == 0) { pop(); pop(); break; }
			find_name(); int w = pop();
			if (w != 0) {
 				if (fetch(STATE) == 0
				|| (fetch(STATE) == 1 && has_flag(w, IMMEDIATE)) 
				|| (fetch(STATE) == 2 && has_flag(w, INSTANTANEOUS))) {
					eval(xt(w));
 				} else {
					if (fetch(STATE) == 1) compile(xt(w));
					else /* STATE == 2 */ postpone(w);
 				}
			} else {
				// TODO Move this to >number ?
 				StringBuilder sb = new StringBuilder();
 				for (int i = 0; i < tlen; i++) sb.append(cfetch(tok + (i*CHAR)));
				if (sb.charAt(0) == '\'') {
					if (fetch(STATE) == 0) push((int)sb.charAt(1));
					else literal((int)sb.charAt(1));
				} else {
					int b = fetch(BASE);
					String sn;
					if (sb.charAt(0) == '#') { b = 10; sn = sb.substring(1); }
					else if (sb.charAt(0) == '$') { b = 16; sn = sb.substring(1); }
					else if (sb.charAt(0) == '%') { b = 2; sn = sb.substring(1); }
					else sn = sb.toString();
	 				try {
	 					int n = Integer.parseInt(sn, b);
	 					if (fetch(STATE) == 0) push(n);
	 					else literal(n);
	 				} catch (NumberFormatException e) {
						_throw(-13);
	 				}
				}
			}
		}
		trace(1);
	}

	public void evaluate(int sid) {
		int l = pop(), a = pop();
		// TODO This is save_source, maybe move to its own word?
		int previbuf = ibuf;
		int previpos = get_ipos();
		int previlen = ilen;
		int last_source_id = source_id;
		int prevtok = tok;
		int prevtlen = tlen;

		source_id = sid; // -1;
		ibuf = a;
		set_ipos(0);
		ilen = l;

		interpret();

		source_id = last_source_id;
		ibuf = previbuf;
		set_ipos(previpos);
		ilen = previlen;
		tlen = prevtlen;
		tok = prevtok;
	}

	public void evaluate(String s, int sid) { str_to_transient(s); evaluate(sid); }

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

	public void leave() { lx = 1; exit(); }

	public void times() { 
		ipush(); 
		int q = pop(), l = pop(); 
		for (ix = 0; ix < l && lx == 0; ix++) 
			eval(q); 
		ipop(); 
	}

	public void unloop() { 
		lx--;
		if (lx == -1) {
			ix = jx; jx = kx; kx = rpick(1);
		} else if (lx == -2) {
			ix = jx; jx = kx; kx = rpick(3);
		}
	}

	// Algorithm for doloop taken from pForth (pf_inner.c case ID_PLUS_LOOP)
	public void doloop() {
		ipush(); 
		int q = pop();
		int do_first_loop = pop();
		ix = pop();
		int l = pop();

		int o = ix - l, d = 0;

		// First iteration is executed always on a DO
		if (do_first_loop == 1) { 
			eval(q);
			if (lx == 0) {
				d = pop();
				o = ix - l;
				ix += d;
			}
		}

		if (!(do_first_loop == 0 && o == 0)) {
			while (((o ^ (o + d)) & (o ^ d)) >= 0 && lx == 0) {
				eval(q);
				if (lx == 0) { // Avoid pop if we're leaving
					d = pop();
					o = ix - l;
					ix += d;
				}
			}
		}

		if (lx == 0 || lx == 1) { ipop(); /* Leave case */ }
		else if (lx < 0) { lx++; rpop(); exit(); /* unloop case */ }
	}

	// ------------------------------------------------------
	// -- Debugging -----------------------------------------
	// ------------------------------------------------------

	public void trace(int l) {
		if (l <= tr) {
			System.out.printf("%d:%d {%d} <%d> ", mp, tp, fetch(STATE), sp);
			for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
			if (ip >= 0) {
				// System.out.printf(": [%d] %d ", ip, ip > 0 ? fetch(ip) : 0);
				int nt = find_xt(fetch(ip));
				if (nt != 0) {
					System.out.printf(": [%d] ", ip);
					push(name(nt)); push(namelen(nt)); type();
					System.out.printf(" ");
				} else {
					System.out.printf(": [%d] %d ", ip, ip > 0 ? fetch(ip) : 0);
				}
			}
			for (int i = rp - 1; i >= 0; i--) 
				if (r[i] != -1) {
					// System.out.printf(": [%d] %d ", r[i], r[i] > 0 ? fetch(r[i]) : r[i]);
					// TODO: Try to find the nearest xt and put
					// the distance from it
					int xt = r[i];
					int nt = 0;
					while (xt > 0) {
						nt = find_xt(xt);
						if (nt != 0) break;
						xt -= CELL;
					}
					if (nt != 0) {
						System.out.printf(": ");
						push(name(nt)); push(namelen(nt)); type();
						System.out.printf(" ");
					} else {
						System.out.printf(": [%d]", r[i]);
					}
				}
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
		// l = new int[lsz];
		o = new Object[osz];
		f = new float[fsz];
		m = ByteBuffer.allocateDirect(msz);
		sp = rp = /* lp = */ op = fp = 0;
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

	public Sloth() { this(64, 256, 32, 32, 32, 512*1024); }

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
					//x.str_to_transient(line);
					//x.interpret();
					x.evaluate(line, -1);
				} catch(SlothException e) {
					if (e.v == -13) { x.push(x.tok); x.push(x.tlen); x.type(); System.out.println(" ?"); }
				}
			}
		} catch(IOException e) {
			e.printStackTrace();
		}
	}

}
