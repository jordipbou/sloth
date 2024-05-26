// Virtual Forth computer and API
// jordipbou, 2024

package io.github.jordipbou;

import java.net.URL;
import java.io.File;
import java.util.List;
import java.util.Scanner;
import java.util.ArrayList;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.function.Consumer;
import java.io.FileNotFoundException;

public class Sloth {

	public class SlothException extends RuntimeException {
		public int v;

		public SlothException(int v) { this.v = v; }
	}

	public static int CELL = 4;
	public static int CHAR = 2;	// Sloth uses 16 bit chars, as Java does

	// -- Outer interpreter -----------------------------------------------------

	public void quit() {
		rp = 0;							// Empty the return stack
		store(STATE, 0);		// Set outer interpreter to interpretation state
		do {
			refill();					// Refill the input buffer from file or input device
			if (top() == 0) break; else drop();
			_catch(fetch(INTERPRET));
			switch (top()) {
				// TODO Change this to not use printf
				case 0: drop(); System.out.printf(" OK "); dump_stack(); break;
				case -1: store(STATE, 0); /* Aborted */ break;
				case -2: store(STATE, 0); /* Display message from abort" */ break;
				case -13: store(STATE, 0); drop(); push('?'); emit(); cr(); break;
				default: store(STATE, 0); System.out.printf(" #%d\n", pop()); break;
				// TODO I'm not sure if data stack should be cleared or not
			}
		} while(true);
	}

	public void interpret() {
		if (fetch(INTERPRET) > 0) eval(INTERPRET);	// If vectored, execute it
		else {
			do { 
				parse_name(); if (top() == 0) break;		// Parse until no more input
				push(FORTH_RECOGNIZER); recognize();		// Recognize the token
				push(fetch(STATE)); cells(); add(); at_execute(); // Excute rec-type
			} while (true);
			two_drop();
		}
	}

	// -- Recognizers -----------------------------------------------------------

	public void rectype() { create(); swap(); rot(); comma(); comma(); comma(); }
	public int rectype(String s) {
		header(s);
		swap(); rot(); comma(); comma(); comma();
		return dt(latest());
	}

	public void rec_null() { type(); push(-13); _throw(); }

	// Recognizer for finding names --

	public void recint_nt() { 
		int w = pop();
		if (!has_flag(w, COLON)) { push(dt(w)); }
		if (xt(w) != 0) { push(xt(w)); execute(); }
	}
	public void reccomp_nt() {
		int w = pop();
		if (!has_flag(w, COLON)) { literal(dt(w)); }
		if (xt(w) != 0) {
			if (has_flag(w, IMMEDIATE)) { push(xt(w)); execute(); }
			else { compile(xt(w)); }
		}
	}
	public void recpost_nt() {
		int w = pop();
		if (!has_flag(w, COLON)) { literal(dt(w)); compile(LITERAL); }
		if (xt(w) != 0) {
			if (has_flag(w, IMMEDIATE)) { compile(xt(w)); }
			else { literal(xt(w)); compile(COMPILE); }
		}
	}

	public void rec_find() { 
		find_name();
		if (top() != 0) push(RECTYPE_NT);
		else { drop(); push(RECTYPE_NULL); }
	}

	// Recognizer for single cell numbers --

	public void recint_num() { /* NOOP */ }
	public void reccomp_num() { literal(); }
	public void recpost_num() { literal(); }

	public void rec_num() {
		int l = pop();
		int a = pop();
		if (cfetch(a) == '\'' && cfetch(a + CHAR + CHAR) == '\'') {
			push(cfetch(a + CHAR));	
			push(RECTYPE_NUM);
		} else {
			int n;
			StringBuilder sb = new StringBuilder();
			int r = fetch(BASE);
			int start = 0;
			if (cfetch(a) == '#') { r = 10; start = 1; }
			else if (cfetch(a) == '$') { r = 16; start = 1; }
			else if (cfetch(a) == '%') { r = 2; start = 1; }
			for (int i = start; i < l; i++) sb.append(cfetch(a + (i * CHAR)));
			try {
				n = Integer.parseInt(sb.toString(), r);
				push(n);
				push(RECTYPE_NUM);
			} catch (NumberFormatException e) {
				push(RECTYPE_NULL);
			}
		}
	}

	// Recognizer for double cell numbers --

	public void recint_dnum() { /* NOOP */ }
	public void reccomp_dnum() { two_literal(); }
	public void recpost_dnum() { two_literal(); }

	public void rec_dnum() {
		int l = pop();
		int a = pop();
		if (cfetch(a + ((l-1)*CHAR)) == '.') {
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < (l - 1); i++) sb.append(cfetch(a + (i * CHAR)));
			long n;
			try {
				n = Long.parseLong(sb.toString(), fetch(BASE));
				dpush(n);
				push(RECTYPE_DNUM);
			} catch (NumberFormatException e) {
				push(RECTYPE_NULL);
			}
		} else {
			push(RECTYPE_NULL);
		} 
	}

	// Recognizer for floating point numbers --

	// TODO

	public void recognize() {
		int r = fetch(pop()); // Recognizer stack identifier (address)
		int l = pop();
		int a = pop();
		int rl = fetch(r - CELL);	// Length of stack is at one cell before its adddress
		for (int i = r; i < r + (rl * CELL); i += CELL) {
			push(a);
			push(l);
			push(fetch(i));
			execute();
			if (top() != RECTYPE_NULL) return;
			else drop();
		}
		push(a);
		push(l);
		push(RECTYPE_NULL);
	}

	// -- Evaluation ------------------------------------------------------------

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

	// -- Word definitions ------------------------------------------------------

	public void create() { parse_name(); header(); }
	public void does() { literal(here() + 16); compile(doDOES); compile(EXIT); }

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
		// If latestxt is not a :NONAME definition, remove its hidden flag
		if (xt(latest()) == latestxt())	
			unset_flag(latest(), HIDDEN); 
	};

	public void noname() {
		align();
		push(here());
		latestxt(here());
		store(STATE, 1);
	}

	public void start_quotation() {
		if (fetch(STATE) > 0) {
			align();
			compile(doBLOCK);
			push(here());
			comma(0);
			latestxt(here());
			store(STATE, fetch(STATE) + 1);
		} else if (fetch(STATE) == 0) {
			push(here());
			latestxt(here());
			store(STATE, fetch(STATE) - 1);
		} else {
			talign();
			compile(doBLOCK);
			push(there());
			comma(0);
			latestxt(there());
			store(STATE, fetch(STATE) - 1);
		}
	}

	public void end_quotation() {
		compile(EXIT);
		if (fetch(STATE) > 0) {
			int mark = pop();
			store(mark, here() - mark);
			store(STATE, fetch(STATE) - 1);
		} else {
			int tmark = pop();
			store(tmark, there() - tmark);
			store(STATE, fetch(STATE) + 1);
		}
	}

	// -- Parsing ---------------------------------------------------------------

	public void parse_name() {
		int ipos = fetch(IPOS);
		while (ipos < ilen && cfetch(ibuf + (ipos * CHAR)) < 33) ipos++;
		push(ibuf + (ipos * CHAR));
		while (ipos < ilen && cfetch(ibuf + (ipos * CHAR)) > 32) ipos++;
		push((ibuf + (ipos * CHAR) - pick(0)) / CHAR);
		store(IPOS, ipos + (ipos < ilen ? 1 : 0));
	}

	// -- Input buffer ----------------------------------------------------------

	public int ibuf;
	public int ilen;

	public String last_dir = "";
	public BufferedReader last_buffered_reader;

	public void refill() {
	switch (fetch(SOURCE_ID)) {
		case -1: push(-1); break;
		case 0: 
			ibuf = tallot(80);
			push(ibuf); 
			push(80); 
			accept();
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

	// public void include(String filename) throws FileNotFoundException, IOException {
	//  	File file = new File(filename);
	// 	if (!file.exists()) {
	// 		file = new File(last_dir + filename);
	// 		if (!file.exists()) {
	// 			throw new FileNotFoundException();
	// 		}
	// 	}

	// 	String prevDir = last_dir;
	// 	last_dir = file.getAbsolutePath().replace(file.getName(), "");

	// 	BufferedReader prevReader = last_buffered_reader;
	//  	last_buffered_reader = new BufferedReader(new InputStreamReader(new FileInputStream(file)));

	// 	int last_source_id = fetch(SOURCE_ID);
	// 	store(SOURCE_ID, 1);

	//  	while (true) {
	//  		String line = last_buffered_reader.readLine();
	//  		if (line == null) break;
	// 		System.out.println(line);
	// 		str_to_transient(line);
	// 		ilen = pop();
	// 		ibuf = pop();
	// 		store(IPOS, 0);
	// 		try {
	// 			interpret();
	// 		} catch(Exception e) {
	// 			// TODO Exceptions should be managed by the inner interpreter
	// 			System.out.printf("Exception on line: [%s]\n", line);
	// 			e.printStackTrace();
	// 			break;
	// 		}
	//  	}

	// 	store(SOURCE_ID, last_source_id);
	// 	last_buffered_reader.close();
	// 	last_buffered_reader = prevReader;
	// 	last_dir = prevDir;
	// }

	// -- Input/Output ----------------------------------------------------------

	public void emit() {
		if (fetch(EMIT) != 0) eval(fetch(EMIT));
		else System.out.printf("%c", (char)pop());
	}

	public void emit(char v) { push(v); emit(); }

	public void cr() { emit('\n'); }

	public void type(int a, int l) {
		for (int i = a; i < a+(l*CHAR); i += CHAR) {
			push(cfetch(i));
			emit();
		}
	}

	public void type() { int l = pop(); type(pop(), l); }

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

	public void accept() {
		if (fetch(ACCEPT) != 0) eval(fetch(ACCEPT));
		else if (fetch(KEY) == 0) {
			String s = System.console().readLine(); // This does not work with gradle run !!!!
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

	// -- Finding words ---------------------------------------------------------

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

	public boolean in_search_order(int w) {
		int order_len = fetch(fetch(ORDER) - CELL);
		for (int i = 0; i < (order_len * CELL); i += CELL) {
			if (wordlist(w) == fetch(fetch(ORDER) + i)) return true;
		}
		return false;
	}

	public void find_name() {
		int l = pop(); 
		int n = pop();
		int w = latest();
		while (w != 0) {
			if (compare_without_case(n, l, w) && !has_flag(w, HIDDEN) && in_search_order(w)) break;
			w = link(w);
		}
		push(w);
	}

	// -- Forth State -----------------------------------------------------------

	// Variables --

	public int STATE = 0*CELL;
	public int LATEST = 1*CELL;
	public int LATESTXT = 2*CELL;
	public int CURRENT = 3*CELL;
	public int BASE = 4*CELL;
	public int SOURCE_ID = 5*CELL;
	public int IPOS = 6*CELL;
	public int FORTH_RECOGNIZER = 7*CELL;
	public int ORDER = 8*CELL;

	// Vectored words --

	public int INTERPRET = 9*CELL;
	public int CATCH = 10*CELL;
	public int THROW = 11*CELL;
	public int ACCEPT = 12*CELL;
	public int KEY = 13*CELL;
	public int EMIT = 14*CELL;

	public int LAST_VAR = 15*CELL;

	// Some required primitives to store their xt to be compiled from Java

	public int doLIT;
	public int doDOES;
	public int doBLOCK;

	public int LITERAL;
	public int COMPILE;

	public int RECTYPE_NULL;
	public int RECTYPE_NT;
	public int RECTYPE_NUM;
	public int RECTYPE_DNUM;

	// -- Contiguous memory -----------------------------------------------------

	public int dp;	// Dictionary pointer
	public int tp;	// Transient memory pointer

	public int here() { return dp; }
	public void here(int v) { dp = v; }
	public void allot(int v) { 
		if (here() + v > mem.capacity()) {
			/* Throw exception */
		} else {
			here(here() + v);
		}
	}
	public void allot() { allot(pop()); }
	public void align() { here((here() + (CELL - 1)) & ~(CELL - 1)); }

	// -- Transient memory ------------------------------------------------------

	public int MARGIN = 512;

	public int there() { return tp; }
	public void there(int v) { tp = v; }
	public int tallot(int v) { 
		if (there() < (here() + MARGIN)) there(here() + MARGIN);
		if ((there() + v) > mem.capacity()) there(here() + MARGIN);
		// if ((there() + v) > mem.capacity()) throw exception ??!!
		int x = there();
		there(there() + v);
		return x;
	}
	public void tallot() { tallot(pop()); }
	public void talign() { there((there() + (CELL - 1)) & ~(CELL - 1)); }

	// Helpers for working with string objects

	public void str_to_transient(String str) {
		int t = tallot(str.length() * CHAR);
		push(t);
		for (int i = 0; i < str.length(); i++) cstore(t + (i * CHAR), str.charAt(i));
		push(str.length());
	}

	public String data_to_str(int a, int u) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < u; i++) sb.append(cfetch(a + (i * CHAR)));
		return sb.toString();
	}

	// -- Compilation -----------------------------------------------------------

	// If state is negative, everything is compiled to transient memory.
	// This will allow using immediate words like IF or [: in interpret
	// mode.

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

	public void comma() { comma(pop()); }
	public void ccomma() { ccomma((char)pop()); }

	public void compile(int v) { comma(v); }
	public void compile() { compile(pop()); }

	public void literal(int v) { compile(doLIT); compile(v); }
	public void literal() { literal(pop()); }

	public void two_literal(long v) { compile(doLIT); compile((int)(msp(v) >> 32)); compile(doLIT); compile((int)lsp(v)); }
	public void two_literal() { two_literal(dpop()); }

	// -- Defining words --------------------------------------------------------

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

	public void set_hidden() { set_flag(latest(), (char)(flags(latest()) | HIDDEN)); }
	public void set_colon() { set_flag(latest(), (char)(flags(latest()) | COLON)); }
	public void set_immediate() { set_flag(latest(), (char)(flags(latest()) | IMMEDIATE)); }

	public void header() {
		int l = pop();
		int a = pop();
		align();
		int w = here();																				// Address of NT/XT
		comma(latest()); latest(w);														// LINK
		comma(0);																							// XT
		comma(0);																							// DT
		comma(fetch(CURRENT));																// WORDLIST
		ccomma((char)0);																			// FLAGS
		ccomma((char)l);																			// NAME LENGTH
		for (int i = 0; i < l; i++) ccomma(cfetch(a + (i * CHAR)));	// Name
		align();
		dt(latest(), here());
	}

	// == Java API ==============================================================

	public void header(String s) { str_to_transient(s); header(); }

	// NONAME adds an anonymous primitive that can be called by its xt
	public int noname(Consumer<Sloth> c) { primitives.add(c); return 0 - primitives.size(); }

	// COLON is used to create a named primitive 
	public int colon(String s, Consumer<Sloth> c) { 
		header(s); 
		int xt = noname(c);
		xt(latest(), xt); 
		set_colon(); 
		return xt;
	}

	// CREATE DOES> from Java becomes:
	// colon("CONSTANT", (vm) -> { create(); comma(); does((vm) -> fetch()); });
	public void does(Consumer<Sloth> c) { xt(latest(), noname(c)); }

	public int find_name(String s) { str_to_transient(s); find_name(); return pop(); }

	public void bootstrap() {
		allot(LAST_VAR);

		store(STATE, 0);
		store(LATEST, 0);
		store(LATESTXT, 0);
		store(CURRENT, 1);
		store(BASE, 10);
		store(SOURCE_ID, 0);
		store(IPOS, 0);
		store(FORTH_RECOGNIZER, 0);

		store(EMIT, 0);
		store(KEY, 0);
		store(ACCEPT, 0);
		store(INTERPRET, 0);

		// Define basic search order
		comma(8); // Max number of items on the search order stack
		comma(1); // Current number of items on the search order stack
		store(ORDER, here());
		comma(1);	// ROOT wordlist to be searched
		allot(7 * CELL); // Leave space for the rest of the possible wordlists

		colon("EXIT", (vm) -> exit());

		store(INTERPRET, colon("INTERPRET", (vm) -> interpret()));

		// Define basic recognizer types

		push(noname((vm) -> rec_null()));
		dup();
		dup();
		RECTYPE_NULL = rectype("RECTYPE-NULL");

		push(noname((vm) -> recint_nt()));
		push(noname((vm) -> reccomp_nt()));
		push(noname((vm) -> recpost_nt()));
		RECTYPE_NT = rectype("RECTYPE-NT");

		push(noname((vm) -> recint_num()));
		push(noname((vm) -> reccomp_num()));
		push(noname((vm) -> recpost_num()));
		RECTYPE_NUM = rectype("RECTYPE-NUM");

		push(noname((vm) -> recint_dnum()));
		push(noname((vm) -> reccomp_dnum()));
		push(noname((vm) -> recpost_dnum()));
		RECTYPE_DNUM = rectype("RECTYPE-DNUM");

		// Define default recognizer
		comma(8);
		comma(3);
		store(FORTH_RECOGNIZER, here());
		comma(noname((vm) -> rec_find()));
		comma(noname((vm) -> rec_num()));
		comma(noname((vm) -> rec_dnum()));
		allot(5 * CELL);

		// -- Primitive words -----------------------------------------------------

		colon("DROP", (vm) -> drop());
		colon("NIP", (vm) -> nip());
		colon("DUP", (vm) -> dup());
		colon("OVER", (vm) -> over());
		colon("SWAP", (vm) -> swap());
		colon("ROT", (vm) -> rot());

		colon("+", (vm) -> add());
		colon("-", (vm) -> sub());
		colon("*", (vm) -> mul());
		colon("/", (vm) -> div());
	}

	// == Virtual machine =======================================================

	// TODO Add load/save image
	// TODO Add tracing for debugging purposes
	// TODO Add cloning of an already bootstrapped VM (like forth in C)
	// TODO Add floating point stack and instructions
	// TODO Add object stack and memory and instructions

	// The virtual machine is language independent.

	int s[];					// Parameter stack
	int sp;						// Parameter stack pointer

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

	public int pick(int a) { return s[sp - a - 1]; }
	public void place(int a, int v) { s[sp - a - 1] = v; }

	public int top() { return pick(0); }

	public void drop() { pop(); }
	public void dup() { push(top()); }
	public void swap() { int t = top(); place(0, pick(1)); place(1, t); }
	public void over() { push(pick(1)); }
	public void rot() { int t = top(); place(0, pick(2)); place(2, pick(1)); place(1, t); }
	public void nip() { place(1, pick(0)); drop(); }

	public void two_drop() { drop(); drop(); }

	int r[];					// Return stack
	int rp;						// Return stack pointer

	public void rpush(int v) { r[rp++] = v; }
	public int rpop() { return r[--rp]; }

	public void to_r() { rpush(pop()); }
	public void from_r() { push(rpop()); }

	// -- Memory

	ByteBuffer mem;		// Dictionary memory

	public int fetch(int a) { return mem.getInt(a); }
	public void store(int a, int v) { mem.putInt(a, v); }
	public char cfetch(int a) { return mem.getChar(a); }
	public void cstore(int a, char v) { mem.putChar(a, v); }

	public void fetch() { push(fetch(pop())); }
	public void store() { int a = pop(); store(a, pop()); }
	public void cfetch() { push(cfetch(pop())); }
	public void cstore() { int a = pop(); cstore(a, (char)pop()); }

	public void cells() { place(0, pick(0) * CELL); }
	public void chars() { place(0, pick(0) * CHAR); }

	// -- Inner interpreter

	public int ip;

	public List<Consumer<Sloth>> primitives;

	public int EXIT = -1;	// EXIT is the only required opcode value to be predefined

	public int token() { int v = fetch(ip); ip += CELL; return v; }
	public boolean valid_ip() { return ip >= 0 && ip < mem.capacity(); }
	public void do_prim(int p) { primitives.get(-1 - p).accept(this); }
	public boolean tail() { return !valid_ip() || fetch(ip) == EXIT; }
	public void call(int q) { if (!tail()) rpush(ip); ip = q; }
	// Negative xts represent primitive functions from the primitives list.
	public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	// Will execute until returning from current level, or an exception is thrown
	public void inner() {	int t = rp; while (t <= rp && valid_ip()) { execute(token()); } }
	// Eval is equivalent to execute but its meant to be called from Java, as
	// it starts a new inner interpreter
	public void eval(int q) { execute(q); inner(); }

	public void execute() { execute(pop()); }
	public void at_execute() { execute(fetch(pop())); }

	public void exit() { if (rp > 0) ip = rpop(); else ip = -1; }

	public void _throw(int v) { if (v != 0) { throw new SlothException(v); } }
	public void _throw() { _throw(pop()); }
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
		}
	}
	public void _catch() { _catch(pop()); }

	// -- Arithmetic 

	public void add() { place(1, pick(1) + pick(0)); drop(); }
	public void sub() { place(1, pick(1) - pick(0)); drop(); }
	public void mul() { place(1, pick(1) * pick(0)); drop(); }
	public void div() { place(1, pick(1) / pick(0)); drop(); }

	// -- Constructor

	public Sloth() {
		s = new int[256];
		r = new int[256];
		mem = ByteBuffer.allocateDirect(65536);
		primitives = new ArrayList<Consumer<Sloth>>();
		sp = 0;
		rp = 0;
		ip = -1;
		dp = 0;
		tp = MARGIN;
	}

	// -- Simple REPL -----------------------------------------------------------

	public void dump_stack() {
		System.out.printf("<%d> ", sp);
		for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
		System.out.println();
	}

	public void dump_memory(int a, int l) {
		// For reusing it...
		// if (fetch(DUMP) != 0) eval(DUMP); else ...
		for (int i = a; i < a + l;) {
			System.out.printf("%08X ", i);
			for (int j = 0; j < 4; j++, i += 2) System.out.printf("%04X ", (short)cfetch(i));
			System.out.printf(" ");
			for (int j = 0; j < 4; j++, i += 2) System.out.printf("%04X ", (short)cfetch(i));
			i -= 16;
			for (int j = 0; j < 4; j++, i += 2) {
				char c = cfetch(i);
				System.out.printf("%c", c < 32 ? '.' : c);
			}
			System.out.printf(" ");
			for (int j = 0; j < 4; j++, i += 2) {
				char c = cfetch(i);
				System.out.printf("%c", c < 32 ? '.' : c);
			}
			System.out.println();
		}
	}

	public int recfib(int n) { if (n > 1) return recfib(n - 1) + recfib(n - 2); else return n; }
	public void fib() { push(recfib(pop())); }

	public void exctest() { _throw(-15); }

	public static void main(String[] args) {
		Sloth x = new Sloth();
		x.bootstrap();

		x.colon("FIB", (vm) -> vm.fib());
		x.colon(".S", (vm) -> vm.dump_stack());
		x.colon("BYE", (vm) -> System.exit(0));
		x.colon("EXCTEST", (vm) -> vm.exctest());

		x.push(36);

		x.quit();

		// x.evaluate("fIb");

		// x.dump_stack();

		////x.eval(x.find_name("FIB"));
		//x.push(x.xt(x.find_name("FIB")));

		//x.execute();

		//x.dump_stack();

		//x.colon("CONSTANT", (vm) -> { vm.create(); vm.comma(); vm.does((vmi) -> vmi.fetch()); });

		//x.push(19);

		//x.str_to_transient("TEST");
		//x.dump_stack();
		//x.ilen = x.pop();
		//x.ibuf = x.pop();
		//x.store(x.IPOS, 0);

		//x.eval(x.xt(x.find_name("CONSTANT")));

		//System.out.printf("LATEST NAME: "); x.type(x.name(x.latest()), x.namelen(x.latest())); System.out.println();

		//x.dump_stack();

		//x.push(x.dt(x.find_name("TEST")));
		//x.eval(x.xt(x.find_name("TEST")));

		//x.dump_stack();
		

		// x.store(0, -1);
		// x.cstore(16, 'Z');
		// x.cstore(32, (char)182);
		// x.dump_memory(0, 128);

		// x.noname((vm) -> vm.exit());
		// x.noname((vm) -> vm.push(vm.token()));
		// x.noname((vm) -> vm.add());
		// x.noname((vm) -> vm.fib());

		// x.store(0, -2);
		// x.store(4, 36);
		// x.store(8, -4);
		// x.store(12, -1);

		// x.store(0, -2);
		// x.store(4, 11);
		// x.store(8, -2);
		// x.store(12, 13);
		// x.store(16, -3);
		// x.store(20, -1);

		// x.dump_memory(0, 32);

		// x.eval(0);

		// x.dump_stack();

		// x.evaluate("quit");
		// try {
		// 	URL url = Thread.currentThread().getContextClassLoader().getResource("sloth.4th");
		// 	x.include(url.getFile());
		// } catch(IOException e) {
		//  	e.printStackTrace();
		// } catch(Exception e) {
		// 	e.printStackTrace();
		// }
	}


	// // --------------------------------------------------------------------------

	// // Constructor

	// public Sloth() {
	// 	s = new int[256];
	// 	sp = 0;
	// 	a = new int[256];
	// 	ap = 0;
	// 	f = new double[16];
	// 	fp = 0;
	// 	o = new Object[256];
	// 	op = 0;
	// 	mem = ByteBuffer.allocateDirect(65536);
	// 	omem = new Object[65536 >> 3];
	// 	// store(STATE, 0);
	// 	// here(0);
	// 	// there(MARGIN);
	// 	// ip = -1;
	// 	// primitives = new ArrayList<Consumer<Sloth>>();
	// }

	// // -- Data stack ------------------------------------------------------------

	// public int[] s;			// Data stack
	// public int sp;			// Data stack pointer
	// 
	// // Helpers to get the most/least significant bits from a long integer, 
	// // used for working with double cell numbers which are a requirement for
	// // ANS Forth (even when not implementing the optional Double-Number word
	// // set).
	// public long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	// public long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }

	// // Push/pop variants
	// public void dpush(long v) { push(v); push(v >> 32); }
	// public void push(long v) { push((int)lsp(v)); }
	// public void push(int v) { s[sp++] = v; }
	// public long dpop() { long b = lpop(); return msp(b << 32) + lsp(lpop()); }
	// public long lpop() { return (long)s[--sp]; }
	// public long upop() { return Integer.toUnsignedLong(s[--sp]); }
	// public long udpop() { long b = upop(); return msp(b << 32) + lsp(upop()); }
	// public int pop() { return s[--sp]; }

	// // Pick from the stack
	// public int pick(int a) { return s[sp - a - 1]; }
	// public void place(int a, int v) { s[sp - a - 1] = v; }

	// // -- Address stack ---------------------------------------------------------

	// public int[] a;			// Return stack
	// public int ap;			// Return stack pointer

	// public void apush(int v) { a[ap++] = v; }
	// public int apop() { return a[--ap]; }

	// public void to_a() { apush(pop()); }
	// public void from_a() { push(apop()); }

	// // -- Floating point stack --------------------------------------------------

	// public double[] f;	// Floating point stack
	// public int fp;			// Floating point stack pointer

	// public void fpush(double v) { f[fp++] = v; }
	// public double fpop() { return f[--fp]; }

	// // -- Object stack ----------------------------------------------------------

	// public Object[] o;	// Object stack
	// public int op;			// Object stack pointer

	// public void opush(Object v) { o[op++] = v; }
	// public Object opop() { return o[--op]; }

	// // -- Dictionary ------------------------------------------------------------

	// public static int MARGIN = 512;

	// public ByteBuffer mem;
	// public int dp;	// Dictionary pointer
	// public int tp;	// Transient memory pointer

	// public Object[] omem;

	// public int fetch(int a) { return mem.getInt(a); }
	// public void store(int a, int v) { mem.putInt(a, v); }
	// public char cfetch(int a) { return mem.getChar(a); }
	// public void cstore(int a, char v) { mem.putChar(a, v); }

	// public double ffetch(int a) { return mem.getDouble(a); }
	// public void fstore(int a, double v) { mem.putDouble(a, v); }

	// public Object ofetch(int a) { return omem[a >> 3]; }
	// public void ostore(int a, Object v) { omem[a >> 3] = v; }

	// // -- Contiguous memory

	// public int here() { return dp; }
	// public void here(int v) { dp = v; }
	// public void allot(int v) { here(here() + v); }
	// public void align() { here((here() + (CELL - 1)) & ~(CELL - 1)); }

	// // -- Transient memory

	// public int there() { return tp; }
	// public void there(int v) { tp = v; }
	// public int tallot(int v) { 
	// 	if (there() < (here() + MARGIN)) there(here() + MARGIN);
	// 	if ((there() + v) > mem.capacity()) there(here() + MARGIN);
	// 	// if ((there() + v) > mem.capacity()) throw exception ??!!
	// 	int x = there();
	// 	there(there() + v);
	// 	return x;
	// }
	// public void talign() { there((there() + (CELL - 1)) & ~(CELL - 1)); }

	// // -- Inner interpreter -----------------------------------------------------

	// public int ip;	// Instruction pointer

	// public List<Consumer<Sloth>> primitives;

	// public int token() { int v = fetch(ip); ip += CELL; return v; }
	// public boolean valid_ip() { return ip >= 0 && ip < mem.capacity(); }
	// public boolean tail() { return !valid_ip() || fetch(ip) == EXIT; }
	// public void do_prim(int p) { primitives.get(-1 - p).accept(this); }
	// // NOTE: Tail call optimization has failed when 2R> was implemented directly in SLOTH.
	// // It also fails with words that end on r> when executed in interpretation mode in the REPL.
	// public void call(int q) { if (!tail()) apush(ip); ip = q; }
	// public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	// public void inner() {	int t = ap; while (t <= ap && valid_ip()) { /* trace(); System.out.println(); */ execute(token()); } }
	// public void eval(int q) { execute(q); inner(); }
	// public void exit() { if (ap > 0) ip = apop(); else ip = -1; }

	// // // Helpers words used on development
	// // // To be removed, relocated or cleaned ***********

	// // public void trace() {
	// // 	System.out.printf("[%d] ", fetch(STATE));
	// // 	System.out.printf("IP::%d ", ip);
	// // 	xt_to_name(fetch(ip)); type(); System.out.printf(" ");
	// // 	dump_stack();
	// // }

	// // public void xt_to_name(int v) {
	// // 	int w = latest();
	// // 	while (w != 0) {
	// // 		if (xt(w) == v) { push(name(w)); push(namelen(w)); return; }
	// // 		w = link(w);
	// // 	}
	// // 	push(0); push(0);
	// // }

	// // *************

	// // TODO Input buffer is primitive, I think

	// // !! End of Virtual Machine implementation !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// // -- Primitives to be accessed from Java -----------------------------------

	// public int EXIT;
	// public int doLIT;

	// // -- Forth state -----------------------------------------------------------

	// public int STATE;
	// public int BASE;
	// public int SOURCE_ID;
	// public int IPOS;
	// public int FORTH_RECOGNIZER;

	// // -- Forth words -----------------------------------------------------------

	// public void drop() { pop(); }
	// public void dup() { push(pick(0)); }
	// public void over() { push(pick(1)); }
	// public void swap() { int a = pop(); int b = pop(); push(a); push(b); };
	// // public void rot() .....

	// public void two_drop() { pop(); pop(); }

	// public void parse_name() {
	// 	int ipos = fetch(IPOS);
	// 	while (ipos < ilen && cfetch(ibuf + (ipos * CHAR)) < 33) ipos++;
	// 	push(ibuf + (ipos * CHAR));
	// 	while (ipos < ilen && cfetch(ibuf + (ipos * CHAR)) > 32) ipos++;
	// 	push((ibuf + (ipos * CHAR) - pick(0)) / CHAR);
	// 	store(IPOS, ipos + (ipos < ilen ? 1 : 0));
	// }


	// //// If state is negative, everything is compiled to transient memory.
	// //// This will allow using immediate words like IF or [: in interpret
	// //// mode.
	// //public void comma(int v) { 
	// //	if (fetch(STATE) >= 0) {
	// //		store(here(), v); allot(CELL); 
	// //	} else {
	// //		store(there(), v); tallot(CELL);
	// //	}
	// //}
	// //public void ccomma(char v) { 
	// //	if (fetch(STATE) >= 0) {
	// //		cstore(here(), v); allot(CHAR);
	// //	} else {
	// //		cstore(there(), v); tallot(CHAR);
	// //	}
	// //}

	// //public void compile(int v) { comma(v); }
	// //public void literal(int v) { compile(doLIT); compile(v); }

	// //// Copies a string object content to transient memory

	// //public void str_to_transient(String str) {
	// //	int t = tallot(str.length());
	// //	push(t);
	// //	for (int i = 0; i < str.length(); i++) cstore(t + (i * CHAR), str.charAt(i));
	// //	push(str.length());
	// //}

	// //// Creates a string object from a char addressed array from memory

	// //public String data_to_str(int a, int u) {
	// //	StringBuilder sb = new StringBuilder();
	// //	for (int i = 0; i < u; i++) sb.append(cfetch(a + (i * CHAR)));
	// //	return sb.toString();
	// //}

	// //// -- Forth state -----------------------------------------------------------

	// //public static int LATEST = 0*CELL;
	// //public static int LATESTXT = 1*CELL;
	// //public static int STATE = 2*CELL;
	// //public static int SOURCE_ID = 3*CELL; 
	// //public static int IPOS = 4*CELL;
	// //public static int BASE = 5*CELL;
	// //public static int CURRENT = 6*CELL;

	// //// Vectored words 

	// //public static int EMIT = 7*CELL;
	// //public static int KEY = 8*CELL;
	// //public static int ACCEPT = 9*CELL;
	// //public static int INTERPRET = 10*CELL;

	// //public static int LAST_VAR = 11*CELL;

	// //// Primitives that need to be compiled from Java code

	// //public static int EXIT;
	// //public static int doLIT;
	// //public static int doDOES;
	// //public static int doBLOCK;
	// //public static int COMPILE;
	// //public static int BRANCH;
	// //public static int zBRANCH;

	// //// -- Words -----------------------------------------------------------------

	// //public static char HIDDEN = 1;
	// //public static char COLON = 2;
	// //public static char IMMEDIATE = 4;

	// //public int latest() { return fetch(LATEST); }
	// //public void latest(int v) { store(LATEST, v); }

	// //public int latestxt() { return fetch(LATESTXT); }
	// //public void latestxt(int v) { store(LATESTXT, v); }

	// //public int xt(int w) { return fetch(w + CELL); }
	// //public void xt(int w, int v) { store(w + CELL, v); }

	// //public int dt(int w) { return fetch(w + CELL + CELL); }
	// //public void dt(int w, int v) { store(w + CELL + CELL, v); }

	// //public int link(int w) { return fetch(w); }

	// //public int wordlist(int w) { return fetch(w + CELL + CELL + CELL); }
	// //public void wordlist(int w, int v) { store(w + CELL + CELL + CELL, v); }

	// //public char flags(int w) { return cfetch(w + CELL + CELL + CELL + CELL); }
	// //public void flags(int w, char v) { cstore(w + CELL + CELL + CELL + CELL, v); }
	// //public void set_flag(int w, char v) { flags(w, (char)(flags(w) | v)); }
	// //public void unset_flag(int w, char v) { flags(w, (char)(flags(w) & ~v)); }
	// //public boolean has_flag(int w, char v) { return (flags(w) & v) == v; }

	// //public char namelen(int w) { return cfetch(w + CELL + CELL + CELL + CELL + CHAR); }
	// //public int name(int w) { return w + CELL + CELL + CELL + CELL + CHAR + CHAR; }

	// //public void immediate() { set_flag(latest(), (char)(flags(latest()) | IMMEDIATE)); }

	// //public void header(String s) { str_to_transient(s); header(); }

	// //public void header() {
	// //	int l = pop();
	// //	int a = pop();
	// //	align();
	// //	int w = here();																				// Address of NT/XT
	// //	comma(latest()); latest(w);														// LINK
	// //	comma(0);																							// XT
	// //	comma(0);																							// DT
	// //	comma(fetch(CURRENT));																// WORDLIST
	// //	ccomma((char)0);																			// FLAGS
	// //	ccomma((char)l);																			// NAME LENGTH
	// //	for (int i = 0; i < l; i++) ccomma(cfetch(a + (i * CHAR)));	// Name
	// //	align();
	// //	dt(latest(), here());
	// //}

	// //public boolean compare_without_case(int n, int l, int w) {
	// //	if (l != namelen(w)) return false;
	// //	for (int i = 0; i < l; i++) {
	// //		char a = cfetch(n + (i * CHAR));
	// //		char b = cfetch(name(w) + (i * CHAR));
	// //		if (a >= 97 && a <= 122) a -= 32;
	// //		if (b >= 97 && b <= 122) b-= 32;
	// //		if (a != b) return false;
	// //	}
	// //	return true;
	// //}

	// //public boolean in_search_order(int w) {
	// //	for (int i = wordlists_ptr - 1; i >= 0; i--) {
	// //		if (wordlist(w) == wordlists[i]) return true;
	// //	}
	// //	return false;
	// //}

	// //public void find_name() {
	// //	int l = pop(); 
	// //	int n = pop();
	// //	int w = latest();
	// //	while (w != 0) {
	// //		if (compare_without_case(n, l, w) && !has_flag(w, HIDDEN) && in_search_order(w)) break;
	// //		w = link(w);
	// //	}
	// //	push(w);
	// //}

	// //// --------------------------------------------------------------------------

	// //// Input buffer

	// //public int ibuf;
	// //public int ilen;

	// //public String last_dir = "";
	// //public BufferedReader last_buffered_reader;

	// //public void key() {
	// //	if (fetch(KEY) != 0) eval(fetch(KEY));
	// //	else {
	// //		try {
	// //			push(System.in.read());
	// //		} catch (IOException e) {
	// //			push(0);
	// //		}
	// //	}
	// //}

	// //public void emit() {
	// //	if (fetch(EMIT) != 0) eval(fetch(EMIT));
	// //	else System.out.printf("%c", (char)pop());
	// //}

	// //public void accept() {
	// //	if (fetch(ACCEPT) != 0) eval(fetch(ACCEPT));
	// //	else if (fetch(KEY) == 0) {
	// //		String s = System.console().readLine();
	// //		int l = pop();
	// //		int a = pop();
	// //		for (int i = 0; i < s.length() && i < l; i++) 
	// //			cstore(a + (i * CHAR), s.charAt(i));
	// //		push(s.length());
	// //	} else {
	// //		int l = pop();
	// //		int a = pop();
	// //		int n = 0;
	// //		while (l > 0) {
	// //			key();
	// //			char c = (char)pop();
	// //			if (c == 10 || c == 13) { 
	// //				push(n); return;
	// //			} else if (c == 127) { 
	// //				if (n > 0) {
	// //					n--;
	// //					l++;
	// //					a--;
	// //					push(8); emit();
	// //					push(32); emit();
	// //					push(8); emit();
	// //				}
	// //			} else {
	// //				cstore(a, c);
	// //				n++;
	// //				a++;
	// //				l--;
	// //			}
	// //		}
	// //	}
	// //}

	// //public void type() {
	// //	int l = pop();
	// //	int a = pop();
	// //	for (int i = 0; i < l; i++) { 
	// //		push(cfetch(a + (i * CHAR))); emit(); 
	// //	}
	// //}

	// //public void source_id() { push(SOURCE_ID); }
	// //public void refill() {
	// //	switch (fetch(SOURCE_ID)) {
	// //		case -1: push(-1); break;
	// //		case 0: 
	// //			ibuf = tallot(80);
	// //			push(ibuf); 
	// //			push(80); 
	// //			eval(fetch(ACCEPT)); 
	// //			ilen = pop();
	// //			store(IPOS, 0);
	// //			push(-1); 
	// //			break;
	// //		default: 
	// //			try {
	// //				str_to_transient(last_buffered_reader.readLine());
	// //				ilen = pop();
	// //				ibuf = pop();
	// //				store(IPOS, 0);
	// //				push(-1);
	// //			} catch (Exception e) {
	// //				push(0);
	// //			}
	// //	}
	// //}

	// //public void include(String filename) throws FileNotFoundException, IOException {
	// // 	File file = new File(filename);
	// //	if (!file.exists()) {
	// //		file = new File(last_dir + filename);
	// //		if (!file.exists()) {
	// //			throw new FileNotFoundException();
	// //		}
	// //	}

	// //	String prevDir = last_dir;
	// //	last_dir = file.getAbsolutePath().replace(file.getName(), "");

	// //	BufferedReader prevReader = last_buffered_reader;
	// // 	last_buffered_reader = new BufferedReader(new InputStreamReader(new FileInputStream(file)));

	// //	int last_source_id = fetch(SOURCE_ID);
	// //	store(SOURCE_ID, 1);

	// // 	while (true) {
	// // 		String line = last_buffered_reader.readLine();
	// // 		if (line == null) break;
	// //		System.out.println(line);
	// //		str_to_transient(line);
	// //		ilen = pop();
	// //		ibuf = pop();
	// //		store(IPOS, 0);
	// //		try {
	// //			interpret();
	// //		} catch(Exception e) {
	// //			// TODO Exceptions should be managed by the inner interpreter
	// //			System.out.printf("Exception on line: [%s]\n", line);
	// //			e.printStackTrace();
	// //			break;
	// //		}
	// // 	}

	// //	store(SOURCE_ID, last_source_id);
	// //	last_buffered_reader.close();
	// //	last_buffered_reader = prevReader;
	// //	last_dir = prevDir;
	// //}

	// //// -- Parsing ---------------------------------------------------------------

	// //// --------------------------------------------------------------------------

	// //// Definitions

	// //// Creates a word only if it does not exist in the dictionary
	// //// Useful for bootstrapping
	// //public void maybe_colon() { 
	// //	parse_name();
	// //	find_name(); 
	// //	store(IPOS, pop() == 0 ? 1 : ilen); 
	// //}

	// //public void colon() {
	// //	parse_name();
	// //	header();
	// //	xt(latest(), dt(latest()));
	// //	latestxt(xt(latest()));
	// //	set_flag(latest(), COLON);
	// //	set_flag(latest(), HIDDEN);
	// //	store(STATE, 1);
	// //}

	// //public void semicolon() { 
	// //	compile(EXIT); 
	// //	store(STATE, 0); 
	// //	// If latestxt is not a :NONAME definition, remove its hidden flag
	// //	if (xt(latest()) == latestxt())	
	// //		unset_flag(latest(), HIDDEN); 
	// //};
	// //public void create() { parse_name(); header(); }
	// //public void does() { literal(here() + 16); compile(doDOES); compile(EXIT); }

	// //public void noname() {
	// //	align();
	// //	push(here());
	// //	latestxt(here());
	// //	store(STATE, 1);
	// //}

	// //public void start_quotation() {
	// //	if (fetch(STATE) > 0) {
	// //		align();
	// //		compile(doBLOCK);
	// //		push(here());
	// //		comma(0);
	// //		latestxt(here());
	// //		store(STATE, fetch(STATE) + 1);
	// //	} else if (fetch(STATE) == 0) {
	// //		push(here());
	// //		latestxt(here());
	// //		store(STATE, fetch(STATE) - 1);
	// //	} else {
	// //		talign();
	// //		compile(doBLOCK);
	// //		push(there());
	// //		comma(0);
	// //		latestxt(there());
	// //		store(STATE, fetch(STATE) - 1);
	// //	}
	// //}

	// //public void end_quotation() {
	// //	compile(EXIT);
	// //	if (fetch(STATE) > 0) {
	// //		int mark = pop();
	// //		store(mark, here() - mark);
	// //		store(STATE, fetch(STATE) - 1);
	// //	} else {
	// //		int tmark = pop();
	// //		store(tmark, there() - tmark);
	// //		store(STATE, fetch(STATE) + 1);
	// //	}
	// //}

	// //// -- Outer interpreter and evaluation --------------------------------------

	// //public void evaluate(String str) { str_to_transient(str); evaluate(); }
	// //public void evaluate() {
	// //	int l = ilen;
	// //	int b = ibuf;
	// //	int p = fetch(IPOS);
	// //	int source_id = fetch(SOURCE_ID);
	// //	ilen = pop();
	// //	ibuf = pop();
	// //	store(IPOS, 0);
	// //	store(SOURCE_ID, -1);
	// //	interpret();
	// //	store(SOURCE_ID, source_id);
	// //	store(IPOS, p);
	// //	ibuf = b;
	// //	ilen = l;
	// //}

	// //public void interpret() {
	// //}

	// // // TODO Add recognizers to the interpreter directly here !!! (Why not?!)
	// // public void interpret() {
	// // 	if (fetch(INTERPRET) != 0) eval(fetch(INTERPRET));
	// // 	else {
	// // 		while (true) {
	// // 			parse_name(); 
	// // 			System.out.printf("TOKEN ["); over(); over(); type(); System.out.printf("]\n");
	// // 			if (s[sp - 1] == 0) { pop(); pop(); return; }
	// // 			int caddr = s[sp - 2]; int u = s[sp - 1];
	// // 			find_name(); int w = pop();
	// // 			if (w != 0) {
	// // 				System.out.printf("WORD FOUND W::%d XT(W)::%d DT(W)::%d\n", w, xt(w), dt(w));
	// // 				if (fetch(STATE) == 0 || has_flag(w, IMMEDIATE)) {
	// // 					System.out.println("INTERPRETING");
	// // 					if (!has_flag(w, COLON)) push(dt(w));
	// // 					if (xt(w) != 0) eval(xt(w));
	// // 				} else {
	// // 					System.out.println("COMPILING");
	// // 					if (!(has_flag(w, COLON))) { literal(dt(w)); } // comma(doLIT); comma(dt(w)); }
	// // 					if (xt(w) != 0) compile(xt(w)); // comma(xt(w));
	// // 				}
	// // 			} else {
	// // 				System.out.println("WORD NOT FOUND, CONVERTING TO NUMBER");
	// // 				StringBuffer sb = new StringBuffer();
	// // 				for (int i = 0; i < u; i++) sb.append(cfetch(caddr + (i * CHAR)));
	// // 				int n = (int)Long.parseLong(sb.toString(), fetch(BASE));
	// // 				if (fetch(STATE) == 0) push(n);
	// // 				else { comma(doLIT); comma(n); }
	// // 			}
	// // 		}
	// // 	}
	// // }

	// // // --------------------------------------------------------------------------

	// // // Include



	// // // --------------------------------------------------------------------------

	// // // Bootstrapping

	// // public void primitive(String s, Consumer<Sloth> c) {
	// // 	primitives.add(c);
	// // 	header(s);
	// // 	xt(latest(), 0 - primitives.size());
	// // 	set_flag(latest(), COLON);
	// // }

	// // public void bootstrap() {
	// // 	allot(LAST_VAR);
	// // 	store(LATEST, 0);
	// // 	store(STATE, 0);
	// // 	store(SOURCE_ID, 0);
	// // 	store(IPOS, 0);
	// // 	store(BASE, 10);
	// // 	store(CURRENT, 1);

	// // 	store(EMIT, 0);
	// // 	store(KEY, 0);
	// // 	store(ACCEPT, 0);
	// // 	store(INTERPRET, 0);

	// // 	// This primitives are used inside Java code. We to store
	// // 	// its xt on variables.
	// // 	primitive("EXIT", (vm) -> exit());
	// // 	EXIT = xt(latest());
	// // 	primitive("doLIT", (vm) -> push(token()));
	// // 	doLIT = xt(latest());
	// // 	primitive("doDOES", (vm) -> xt(latest(), pop()));
	// // 	doDOES = xt(latest());
	// // 	primitive("COMPILE,", (vm) -> comma(pop()));
	// // 	COMPILE = xt(latest());

	// // 	primitive("BRANCH", (vm) -> ip += token());
	// // 	BRANCH = xt(latest());
	// // 	primitive("?BRANCH", (vm) -> { if (pop() == 0) ip += token(); else ip += CELL; });
	// // 	zBRANCH = xt(latest());

	// // 	primitive("doBLOCK", (vm) -> { int v = token(); push(ip); ip += v - CELL; });
	// // 	doBLOCK = xt(latest());

	// // 	primitive("doSTRING", (vm) -> { int l = token(); push(ip); push(l); ip += (l + (CELL - 1)) & ~(CELL - 1);	});

	// // 	primitive("EXECUTE", (vm) -> execute(pop()));
	// // 	primitive("RECURSE", (vm) -> compile(latestxt()));
	// // 	primitive(":NONAME", (vm) -> { push(here()); latestxt(here()); store(STATE, 1); });
	// // 	primitive("BYE", (vm) -> System.exit(0));

	// // 	// Forth state ------------------------------------------------------------

	// // 	primitive("LATEST", (vm) -> push(LATEST));
	// // 	primitive("LATESTXT", (vm) -> push(LATESTXT));
	// // 	primitive("STATE", (vm) -> push(STATE));
	// // 	primitive("HERE", (vm) -> push(HERE));
	// // 	primitive("THERE", (vm) -> push(THERE));
	// // 	primitive("SOURCE-ID!", (vm) -> store(SOURCE_ID, pop()));
	// // 	primitive("SOURCE", (vm) -> { push(ibuf); push(ilen); });
	// // 	primitive(">IN", (vm) -> push(IPOS));
	// // 	primitive("BASE", (vm) -> push(BASE));
	// // 	primitive("CURRENT", (vm) -> push(CURRENT));

	// // 	primitive("(INTERPRET)", (vm) -> push(INTERPRET));

	// // 	// Return stack manipulation ----------------------------------------------

	// // 	primitive("RP@", (vm) -> push(rp));
	// // 	primitive("RP!", (vm) -> rp = pop());
	// // 	primitive(">R", (vm) -> apush(pop()));
	// // 	primitive("R>", (vm) -> push(apop()));

	// // 	// Data stack manipulation ------------------------------------------------

	// // 	primitive("SP@", (vm) -> push(sp));
	// // 	primitive("SP!", (vm) -> sp = pop());
	// // 	primitive("DROP", (vm) -> pop());
	// // 	primitive("DUP", (vm) -> dup());
	// // 	primitive("OVER", (vm) -> over());
	// // 	primitive("SWAP", (vm) -> swap());
	// // 	primitive("PICK", (vm) -> push(P(pop())));

	// // 	// Memory operations ------------------------------------------------------

	// // 	primitive("HERE", (vm) -> push(here()));
	// // 	primitive("ALLOT", (vm)-> allot(pop()));
	// // 	primitive("UNUSED", (vm) -> push(mem.capacity() - here()));
	// // 	primitive("@", (vm) -> push(fetch(pop())));
	// // 	primitive("!", (vm) -> { int a = pop(); int v = pop(); store(a, v); });
	// // 	primitive("C@", (vm) -> push(cfetch(pop())));
	// // 	primitive("C!", (vm) -> { int a = pop(); int v = pop(); cstore(a, (char)v); });
	// // 	primitive("CELLS", (vm) -> push(pop() * CELL));
	// // 	primitive("CHARS", (vm) -> push(pop() * CHAR));

	// // 	// Bit operations ---------------------------------------------------------

	// // 	primitive("0<", (vm) -> { int v = pop(); push(v < 0 ? -1 : 0); });
	// // 	primitive("AND", (vm) -> { int v = pop(); int w = pop(); push(w & v); });
	// // 	primitive("INVERT", (vm) -> { int v = pop(); push(~v); });
	// // 	primitive("RSHIFT", (vm) -> { int a = pop(); int b = pop(); push(b >>> a); });
	// // 	primitive("2/", (vm) -> { int a = pop(); push(a >> 1); });

	// // 	// Required Arithmetic operations -----------------------------------------

	// // 	primitive("-", (vm) -> { int a = pop(); int b = pop(); push(b - a); });
	// // 	primitive("*/MOD", (vm) -> { long n = lpop(); long d = lpop() * lpop(); push(d % n); push(d / n); });
	// // 	primitive("UM*", (vm) -> { long r = upop() * upop(); dpush(r); });
	// // 	primitive("UM/MOD", (vm) -> { long u = upop(); long d = dpop(); push(Long.remainderUnsigned(d, u)); push(Long.divideUnsigned(d, u)); });

	// // 	// Additional Arithmetic operations ---------------------------------------

	// // 	primitive("+", (vm) -> { int a = pop(); int b = pop(); push(b + a); });

	// // 	// Definitions ------------------------------------------------------------

	// // 	primitive("?:", (vm) -> maybe_colon());
	// // 	primitive(":", (vm) -> colon());
	// // 	primitive(";", (vm) -> semicolon()); immediate(); 
	// // 	primitive("CREATE", (vm) -> create());
	// // 	primitive("DOES>", (vm) -> does());	immediate();
	// // 	primitive("IMMEDIATE", (vm) -> immediate());
	// // 	primitive("'", (vm) -> { parse_name(); find_name(); push(xt(pop())); });
	// // 	primitive("POSTPONE", (vm) -> {
	// // 		parse_name();
	// // 		find_name();
	// // 		int w = pop();
	// // 		if (has_flag(w, IMMEDIATE)) {	compile(xt(w));	} 
	// // 		else { literal(xt(w)); compile(COMPILE);	}
	// // 	}); immediate();

	// // 	// primitive("IMMEDIATE-FLAG", (vm) -> push(IMMEDIATE));
	// // 	// primitive("COLON-FLAG", (vm) -> push(COLON));
	// // 	// primitive("HIDDEN-FLAG", (vm) -> push(HIDDEN));

	// // 	primitive("NT>LINK", (vm) -> link(pop()));
	// // 	primitive("NT>XT", (vm) -> xt(pop()));
	// // 	primitive("NT>DT", (vm) -> dt(pop()));
	// // 	primitive("NT>WORDLIST", (vm) -> wordlist(pop()));
	// // 	primitive("NT>FLAGS", (vm) -> flags(pop()));
	// // 	primitive("NT>NAME", (vm) -> { int v = pop(); push(name(v)); push(namelen(v)); });
	// // 	primitive("NAME>STRING", (vm) -> { int v = pop(); push(name(v)); push(namelen(v)); });

	// // 	primitive("IMMEDIATE?", (vm) -> has_flag(pop(), IMMEDIATE));
	// // 	primitive("COLON?", (vm) -> has_flag(pop(), COLON));
	// // 	primitive("HIDDEN?", (vm) -> has_flag(pop(), HIDDEN));

	// // 	// Evaluation -------------------------------------------------------------

	// // 	primitive("EVALUATE", (vm) -> evaluate());
	// // 	primitive("INCLUDED", (vm) -> { 
	// // 		int l = pop(); 
	// // 		int a = pop(); 

	// // 		// TODO: Manage exceptions
	// // 		try {
	// // 			include(data_to_str(a, l)); 
	// // 		} catch(Exception e) {
	// // 			e.printStackTrace();
	// // 		}
	// // 	});

	// // 	// Control structures -----------------------------------------------------

	// // 	// This two are only needed for DO/LOOP implementation, but maybe DO/LOOP should
	// // 	// be a primitive.
	// // 	primitive(">MARK", (vm) -> { push(here()); comma(0); });
	// // 	primitive(">>RESOLVE", (vm) -> store(pop(), here()));

	// // 	primitive("AHEAD", (vm) -> { compile(BRANCH); push(here()); comma(0); }); immediate();
	// // 	primitive("IF", (vm) -> { compile(zBRANCH); push(here()); comma(0); }); immediate();
	// // 	primitive("THEN", (vm) -> { int a = pop(); store(a, here() - a); }); immediate();

	// // 	primitive("BEGIN", (vm) -> push(here())); immediate();
	// // 	primitive("UNTIL", (vm) -> { compile(zBRANCH); comma(pop() - here()); }); immediate();
	// // 	primitive("AGAIN", (vm) -> { compile(BRANCH); comma(pop() - here()); }); immediate();

	// // 	// Quotations -------------------------------------------------------------

	// // 	primitive("[:", (vm) -> { compile(doBLOCK); push(here()); comma(0); }); immediate();
	// // 	primitive(";]", (vm) -> { compile(EXIT); int a = pop(); store(a, here() - a); }); immediate();

	// // 	// Input/output -----------------------------------------------------------

	// // 	// TODO These two must be deferred ?
	// // 	primitive("EMIT", (vm) -> emit());
	// // 	primitive("KEY", (vm) -> key());
	// // 	primitive("REFILL", (vm) -> refill());
	// // 	primitive("ACCEPT", (vm) -> accept());
	// // 	primitive("(ACCEPT)", (vm) -> push(ACCEPT));

	// // 	// Tools ------------------------------------------------------------------

	// // 	primitive(".S", (vm) -> dump_stack());

	// // 	// Error management -------------------------------------------------------
	// // 
	// // 	// TODO CATCH THROW
	// // }

	// // // --------------------------------------------------------------------------

	// // // Utilities

	// // public void dump_stack() {
	// // 	// Temporary, should use EMIT/TYPE
	// // 	System.out.printf("<%d> ", sp);
	// // 	for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
	// // }

	// // // --------------------------------------------------------------------------

	// // // Simple REPL

	// // public static void main(String[] args) {
	// // 	Sloth x = new Sloth();
	// // 	x.bootstrap();

	// // 	try {
	// // 		URL url = Thread.currentThread().getContextClassLoader().getResource("sloth.4th");
	// // 		x.include(url.getFile());
	// // 	} catch(IOException e) {
	// // 	 	e.printStackTrace();
	// // 	} catch(Exception e) {
	// // 		e.printStackTrace();
	// // 	}
	// // }
}
