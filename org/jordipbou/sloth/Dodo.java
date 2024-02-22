// TODO: How to do does> part from Java? Interesting for implementing a "native" compiler.
// TODO: Including source code here as a string works well, maybe it will be interesting
// to move everything possible to Forth source code?

package org.jordipbou.sloth;

import java.io.*;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Stack;
import java.util.ArrayList;
import java.util.function.Consumer;

public class Dodo {
	public static int M = 1024; // Minimum margin between here and there	

	public int[] s;
	public int ss = 0;
	public int sp = 0;
	public int[] r;
	public int rs = 0;
	public int rp = 0;
	public ByteBuffer u;
	public int us = 0;
	public ByteBuffer d;
	public int there = M;
	public int ip = -1;
	public int latestxt = 0;
	public int current = 0;
	public int context[] = new int[8];

	public String last_dir = "";

	public Dodo(int ssize, int rsize, int usize, ByteBuffer dict) {
		s = new int[ssize];
		ss = ssize;
		r = new int[rsize];
		rs = rsize;
		u = ByteBuffer.allocateDirect(usize*4);
		us = usize*4;
		ip = -us - 1;
		d = dict;
	}

	public List<Consumer<Dodo>> primitives = new ArrayList<Consumer<Dodo>>();
	public List<Word> wordlists = new ArrayList<Word>();
	public Stack<Word> words = new Stack<Word>();

	public class Word {
		Word previous;
		int xt = -1;
		int dt = -1;
		boolean immediate = false;
		boolean colon = false;
		boolean hidden = false;
		String name = "";
	}

	// ***** Helpers

	// --- Data stack

	public int T() { return s[sp - 1]; }
	public void push(int v) { s[sp++] = v; }
	public int pop() { return s[--sp]; }

	public long lpop() { return (long)s[--sp]; }
	public long upop() { return Integer.toUnsignedLong(s[--sp]); }

	public long msp(long v) { return (v & 0xFFFFFFFF00000000L); }
	public long lsp(long v) { return (v & 0x00000000FFFFFFFFL); }
	public void push(long v) { push((int)lsp(v)); }
	public void dpush(long v) { push(v); push(v >> 32); }
	public long dpop() { long b = lpop(); return msp(b << 32) + lsp(lpop()); }
	public long udpop() { long b = upop(); return msp(b << 32) + lsp(upop()); }

	// --- Return stack

	public int rpop() { return r[--rp]; }
	public void rpush(int v) { r[rp++] = v; }

	// --- Transient region

	public int tallot(int v) {
		int t = (there + v) > (d.capacity() - M) ? d.position() + M : there;
		there += v;
		return t;
	}
	public void talign() { tallot(((there + 3) & ~3) - there); }

	public void str_to_data(String str) { 
		int t = tallot(str.length());
		push(t);
		for (int i = 0; i < str.length(); i++) d.put(t + i, (byte)str.charAt(i));
		push(str.length());
	}

	public String data_to_str() { 
		StringBuilder sb = new StringBuilder();
		int u = pop();
		int a = pop();
		for (int i = 0; i < u; i++) sb.append((char)d.get(a + i));
		return sb.toString();
	}

	public void to_counted_string() { 
		int n = pop(); 
		int a = pop(); 
		int t = tallot(n + 1);
		push(t);
		d.put(t, (byte)n);
		for (int i = 0; i < n; i++) d.put(t + i + 1, d.get(a + i));
	}

	// ***** User Variables

	public static int BASE = 0*4;
	public static int IPOS = 1*4;
	public static int ILEN = 2*4;
	public static int IBUF = 3*4;
	public static int STATE = 4*4;
	// Vectored primitives
	public static int EMIT = 5*4;
	public static int KEY = 6*4;
	// Numeric output
	public static int HOLD_END = 7*4;
	public static int HLD = 8*4;

	public int v(int a) { return u.getInt(a); }
	public void v(int a, int v) { u.putInt(a, v); }

	public int vaddr(int a) { return -us + a; }

	// ***** Words

	public void wordlist() { wordlists.add(null); push(wordlists.size() - 1); }
	public Word latest() { return wordlists.get(current); }
	public void latest(Word w) { wordlists.set(current, w); }

	public void create(String name) {
		words.push(new Word());
		words.peek().name = name;
		words.peek().previous = latest();
		latest(words.peek());
		latest().xt = -1;
		latest().dt = d.position();
	}

	public void create() { parse_name(); if (T() == 0) { two_drop(); return; } create(data_to_str()); }

	public void immediate() { latest().immediate = true; }
	//public void hide() { latest().hidden = true; }

	//public void reveal() { Word l = latest(); l.hidden = false; l.colon = true; }
	public void colon() { 
		create(); 
		latestxt = latest().xt = d.position(); 
		latest().hidden = true;
		latest().colon = true;
		right_bracket(); 
	}
	public void semicolon() { 
		compile(EXIT); 
		bracket(); 
		latest().hidden = false;
	}
	//reveal(); }

	public Word find_name_in(String n, int wl) {
		Word w = wordlists.get(wl);
		while (w != null) {
			if (!w.hidden && w.name.equalsIgnoreCase(n)) break;
			w = w.previous;
		}
		return w;
	}
	public void find_name_in() { int l = pop(); push(words.indexOf(find_name_in(data_to_str(), l))); }

	public Word find_name(String n) {
		for (int i = 0; i < 8; i++) {
			if (context[i] == -1) return null;
			Word w = find_name_in(n, context[i]);
			if (w != null) return w;
		}
		return null;
	}
	public void find_name() { push(words.indexOf(find_name(data_to_str()))); }

	// ***** Non organized primitives

	public void depth() { push(sp); }
	public void drop() { --sp; }
	public void two_drop() { sp = sp - 2; }
	public void dup() { push(s[sp - 1]); }
	public void two_dup() { over(); over(); }
	public void swap() { int a = s[sp - 1]; s[sp - 1] = s[sp - 2]; s[sp - 2] = a; }
	public void two_swap() { rot(); to_r(); rot(); from_r(); }
	public void over() { push(s[sp - 2]); }
	public void two_over() { push(3); pick(); push(3); pick(); }
	public void pick() { int a = pop(); push(s[sp - a - 1]); }
	public void rot() { to_r(); swap(); from_r(); swap(); }
	public void qdup() { if (s[sp - 1] != 0) dup(); }
	public void nip() { swap(); drop(); }

	public void store() { int a = pop(); store(a, pop()); }
	public void store(int a, int v) { if (a >= 0) d.putInt(a, v); else u.putInt(us + a, v); }
	public void comma(int v) { d.putInt(v); }
	public void comma() { int v = pop(); comma(v); }
	public void fetch() { push(fetch(pop())); }
	public int fetch(int a) { if (a >= 0) return d.getInt(a); else return u.getInt(us + a); }
	public void cstore() { int a = pop(); cstore(a, (byte)pop()); }
	public void cstore(int a, byte v) { d.put(a, v); }
	public void ccomma() { int a = pop(); d.put((byte)a); }
	public byte cfetch(int a) { return d.get(a); }
	public void cfetch() { push(cfetch(pop())); }
	public void plus_store() { dup(); fetch(); rot(); plus(); swap(); store(); }
	public void two_store() { swap(); over(); store(); cell_plus(); store(); }
	public void two_fetch() { dup(); cell_plus(); fetch(); swap(); fetch(); }
	public void align() { d.position((d.position() + 3) & ~3); }
	public void aligned() { push((pop() + 3) & ~3); }
	public void allot() { int a = pop(); d.position(d.position() + a); }
	public void here() { push(d.position()); }

	public void to_r() { r[rp++] = s[--sp]; }
	public void from_r() { s[sp++] = r[--rp]; }
	public void r_fetch() { s[sp++] = r[rp - 1]; }

	public void zero_less() { s[sp - 1] = s[sp - 1] < 0 ? -1 : 0; }
	public void zero_equals() { s[sp - 1] = s[sp - 1] == 0 ? -1 : 0; }
	public void less_than() { s[sp - 2] = s[sp - 2] < s[sp - 1] ? -1 : 0; sp--; }
	public void equals() { s[sp - 2] = s[sp - 2] == s[sp - 1] ? -1 : 0; sp--; }
	public void greater_than() { s[sp - 2] = s[sp - 2] > s[sp - 1] ? -1 : 0; sp--; }
	public void u_less() { long b = upop(); push(upop() < b ? -1 : 0); }

	public void plus() { s[sp - 2] = s[sp - 2] + s[sp - 1]; sp--; }
	public void minus() { s[sp - 2] = s[sp - 2] - s[sp - 1]; sp--; }
	public void times() { s[sp - 2] = s[sp - 2] * s[sp - 1]; sp--; }
	public void two_times() { s[sp - 1] *= 2; }
	public void div() { s[sp - 2] = s[sp - 2] / s[sp - 1]; sp--; }
	public void two_div() { s[sp - 1] = s[sp - 1] >> 1; }
	public void div_mod() { int a = pop(); int b = pop(); push(b % a); push(b / a); }
	public void mod() { s[sp - 2] = s[sp - 2] % s[sp - 1]; sp--; }
	public void one_plus() { s[sp - 1] += 1; }
	public void one_minus() { s[sp - 1] -= 1; }
	public void abs() { if (s[sp - 1] < 0) s[sp - 1] = 0 - s[sp - 1]; }
	public void max() { if (s[sp - 2] < s[sp - 1]) { swap(); } drop(); }
	public void min() { if (s[sp - 2] > s[sp - 1]) { swap(); } drop(); }
	public void times_div() { to_r(); m_times(); from_r(); sm_div_rem(); swap(); drop(); }
	public void times_div_mod() { to_r(); m_times(); from_r(); sm_div_rem(); }
	public void fm_div_mod() { long n = lpop(); long d = dpop(); push(Math.floorMod(d, n));push(Math.floorDiv(d, n)); }
	public void sm_div_rem() { long n = lpop(); long d = dpop(); push(d % n); push(d / n); }
	public void m_times() { long r = lpop() * lpop(); dpush(r); }
	public void u_m_times() { long r = upop() * upop(); dpush(r); }
	public void u_m_div_mod() { long u = upop(); long d = dpop(); push(Long.remainderUnsigned(d, u)); push(Long.divideUnsigned(d, u)); }
	
	public void negate() { s[sp - 1] = 0 - s[sp - 1]; }

	public void and() { s[sp - 2] = s[sp - 2] & s[sp - 1]; sp--; }
	public void or() { s[sp - 2] = s[sp - 2] | s[sp - 1]; sp--; }
	public void xor() { s[sp - 2] = s[sp - 2] ^ s[sp - 1]; sp--; }
	public void invert() { s[sp - 1] = ~s[sp - 1]; }
	public void lshift() { int a = pop(); s[sp - 1] = s[sp - 1] << a; }
	public void rshift() { int a = pop(); s[sp - 1] = s[sp - 1] >>> a; }

	public void cell_plus() { s[sp - 1] = s[sp - 1] + 4; }
	public void cells() { s[sp - 1] = s[sp - 1] * 4; }

	public void _char() { parse_name(); drop(); cfetch(); }
	public void bracket_char() { parse_name(); drop(); cfetch(); literal(); }
	public void char_plus() { one_plus(); }
	public void chars() { /* Does nothing */ }

	public void constant() { create(); latest().dt = pop(); } 
	public void variable() { create(); comma(0); } 
	public void to_body() { 
		int xt = pop();
		for (int i = words.size() - 1; i >= 0; i--) {
			Word w = words.get(i);
			if (w.xt == xt) { push(w.dt); return; }	
		}
		push(0);
	}

	public void dodoes() { latest().xt = pop();	}
	public void does() { literal(d.position() + 16); compile(DODOES); compile(EXIT); }

	public void bracket() { v(STATE, 0); }
	public void right_bracket() { v(STATE, 1); }

	public void find() { 
		int cstr = T();
		count();
		find_name();
		if (T() == -1) { drop(); push(cstr); push(0); }
		else {
			Word w = words.get(pop());
			push(w.xt);
			if (w.immediate) push(1);
			else push(-1);
		}
	}
	public void bracket_tick() { tick(); literal(); }
	public void postpone() { 
		parse_name();
		find_name();
		Word w = words.get(pop());
		if (w.immediate) {
			compile(w.xt);
		} else {
			literal(w.xt);
			compile(COMPILE);
		}
	}

	public void _if() { compile(ZBRANCH); push(d.position()); d.putInt(0); }
	public void _else() { compile(BRANCH); push(d.position()); d.putInt(0); swap(); int m = pop(); d.putInt(m, d.position() - m); }
	public void then() { int m = pop(); d.putInt(m, d.position() - m); }
	public void _do() { 
		// This one is tricky, as it needs to allow use of LOOP, +LOOP, UNLOOP and LEAVE
		push(d.position() + 4); // position to be resolved for every leave
		literal(0); // placeholder for pushing were to jump on a leave
		compile(TO_R); // move leave address to return stack
		begin();	// compile a backwards mark
		compile(TO_R);	// move index to return stack
		compile(TO_R); // move limit to return stack
	}
	public void i() { s[sp++] = r[rp - 2]; }
	public void j() { s[sp++] = r[rp - 5]; }
	public void leave() { 
		from_r(); drop(); // remove limit from return stack
		from_r(); drop(); // remove index from return stack
		// as leave address should be on the return stack, technically we can return to it
		exit();
	}
	public void unloop() { from_r(); from_r(); from_r(); drop(); drop(); drop(); }
	public void loop() {
		compile(FROM_R); // limit is on the data stack
		compile(FROM_R); // index is on the data stack ( limit index -- )
		compile(ONE_PLUS); // increment index
		compile(OVER);
		compile(OVER);
		compile(EQUALS);
		until();
		// Resolve the leave address
		compile(DROP);
		compile(DROP);
		compile(FROM_R);
		compile(DROP);
		int l = pop();
		d.putInt(l, d.position());
	}

	public void plus_loop_p() {
		// Code adapted from pForth C source code
		int limit = rpop();
		int index = rpop();
		int delta = pop();
		int newIndex = index + delta;
		int oldDiff = index - limit;

		if (((oldDiff ^ (oldDiff + delta)) & (oldDiff ^ delta)) < 0) {
			ip += 4;
			push(limit);
			push(newIndex);
		} else {
			push(limit);
			push(newIndex);
			ip += d.getInt(ip);
		}
	}

	public void plus_loop() { 
		compile(DOPLUSLOOP);
		comma(pop() - d.position());
		// Resolve the leave address
		compile(DROP);
		compile(DROP);
		compile(FROM_R);
		compile(DROP);
		int l = pop();
		d.putInt(l, d.position());
	}
	public void begin() { push(d.position()); }
	public void again() { compile(BRANCH); comma(pop() - d.position()); }
	public void repeat() { again(); then(); }
	public void until() { compile(ZBRANCH); comma(pop() - d.position()); }
	public void _while() { _if(); swap(); }

	public void emit() { execute(v(EMIT)); }
	public void key() { execute(v(KEY)); }

	public void source() { push(v(IBUF)); push(v(ILEN)); }
	public void accept() {
		int max = pop();
		int a = pop();
		for (int i = 0; i < max; i++) {
			execute(v(KEY));
			byte k = (byte)pop();
			if (k == 8 || k == 127) {
				i--;
				push(8); execute(v(EMIT));
				push(32); execute(v(EMIT));
				push(8); execute(v(EMIT));
			} else if (k == 10 || k == 13) {
				push(i);
				return;
			} else if (k > 31) {
				push(k); execute(v(EMIT));
				cstore(a + i, k);
			}
		}
		push(max);
	}
	public void type() { System.out.printf("%s", data_to_str()); }

	public void bl() { push(32); }
	public void cr() { push(10); emit(); }
	public void space() { bl(); emit(); }
	public void spaces() { int a = pop(); for (;a > 0; a--) space(); }
	public void count() { dup(); one_plus(); swap(); cfetch(); }
	public void fill() { 
		byte c = (byte)pop(); 
		int u = pop(); 
		int a = pop();
		for(int i = 0; i < u; i++) {
			cstore(a + i, c);
		}
	}
	public void move() {
		int u = pop();
		int a2 = pop();
		int a1 = pop();
		if (a1 >= a2) {
			for (int i = 0; i < u; i++) {
				cstore(a2 + i, cfetch(a1 + i));
			}
		} else {
			for (int i = u - 1; i >= 0; i--) {
				cstore(a2 + i, cfetch(a1 + i));
			}
		}
	}
	public void dot_quote() { s_quote(); if (v(STATE) == 0) execute(TYPE); else compile(TYPE); }
	public void s_quote() { 
		push(34); 
		parse(); 
		int u = pop() - 1;
		int a = pop();
		if (v(STATE) == 0) {
			push(there);
			for(int i = 0; i < u; i++) {
				d.put(there, d.get(a + i));
				//there++; too_far();
				tallot(1);
			}
			push(u);
			//there = (there + 3) & ~3; too_far();
			talign();
		} else {
			compile(STRING);
			int h = d.position();
			d.position(d.position() + 4);
			for (int i = 0; i < u; i++) {
				d.put(d.get(a + i));
			}
			d.putInt(h, d.position() - h);
			align();
		}
	}

	
	public void s_to_d() { dpush((long)pop()); }

	
	public void u_d_div_mod() { to_r(); push(0); r_fetch(); u_m_div_mod(); from_r(); swap(); to_r(); u_m_div_mod(); from_r(); }


	public void environment() { /* TODO */ }

	public void compile(int v) { d.putInt(v); }
	public void compile() { int xt = pop(); compile(xt); }

	public void branch() { ip += d.getInt(ip); }
	public void zbranch() { if (pop() == 0) ip += d.getInt(ip); else ip += 4; }
	public void string() { 
		push(ip + 4); 
		int x = d.getInt(ip); 
		push(x - 4); 
		ip = (ip + x + 3) & ~3;
	}
	public void quotation() { push(ip + 4); ip += d.getInt(ip); }
	public void lit() { push(d.getInt(ip)); ip += 4; }

	public int peek() { return d.getInt(ip); }
	public int token() { int t = d.getInt(ip); ip += 4; return t; }
	public boolean valid_ip() { return ip >= 0 && ip < d.capacity(); }
	public boolean tail() { return !valid_ip() || peek() == EXIT; }
	public void do_prim(int p) { primitives.get(0 - p).accept(this); }
	public void call(int q) { if (!tail()) r[rp++] = ip; ip = q; }
	public void execute(int q) { if (q < 0) do_prim(q); else call(q); }
	public void execute() { execute(pop()); }
	public void inner() {	
		int t = rp; 
		while (t <= rp && valid_ip()) { 
			execute(token());
		} 
	}
	public void eval(int q) { execute(q); inner(); }

	public void exit() { if (rp > 0) ip = r[--rp]; else ip = -1; }
	public void literal(int v) { compile(LIT); compile(v); }
	public void literal() { literal(pop()); }
	public void recurse() { compile(latestxt); } // compile(latest().xt); }

	public void _throw() { /* TODO */ }
	public void _catch() { /* TODO */ }
	public void abort() { /* TODO */ }
	public void abortq() { /* TODO */ }

	public void included() { included(data_to_str()); }
	public void included(String filename) {
		File file;
		file = new File(filename);
		if (!file.exists()) {
			file = new File(last_dir + filename);
			if (!file.exists()) {
				return;
			}
		}

		String prev_last_dir = last_dir;
		last_dir = file.getAbsolutePath().replace(file.getName(), "");

		try {
			BufferedReader f =
  		  new BufferedReader(
  		    new InputStreamReader(
						new FileInputStream(file)));

			String l = "";
  		try {
				while (true) {
  			  l = f.readLine();
  			  if (l == null) break;
   			  evaluate(l);
  			}
  		} catch(Exception e) {
				System.out.println("EXCEPTION ON LINE:");
				System.out.printf("%s\n", l);
				System.out.println(e);
				e.printStackTrace();
			}
		} catch (FileNotFoundException e) {
		}

		last_dir = prev_last_dir;
	}

	public void convert_to_number() {
		int l = pop();
		int t = pop();
		if (l == 3 && d.get(t) == (byte)'\'' && d.get(t + 2) == (byte)'\'') {
			push(d.get(t + 1));
			if (v(STATE) != 0) literal();
		} else {
			int base = v(BASE);
			switch (d.get(t)) {
				case '#': base = 10; t++; l--; break;
				case '$': base = 16; t++; l--; break;
				case '%': base = 2; t++; l--; break;
			}
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < l; i++) {
				sb.append((char)d.get(t + i));
			}
			String s = sb.toString();
			try {
				int n = Integer.parseInt(s, base);
				push(n);
				if (v(STATE) != 0) literal();
			} catch (NumberFormatException e) {
				System.out.printf("DO_NUMBER:Can't convert %s to number\n", s);
				System.out.println(e);
			}
		}
	}

	public void parse_chars() {
		byte c = (byte)pop();
		push(v(IBUF) + v(IPOS));
		while (v(IPOS) < v(ILEN) && d.get(v(IBUF) + v(IPOS)) == c) v(IPOS, v(IPOS) + 1);
		push(v(IBUF) + v(IPOS) - T());
	}

	public void parse() {
		byte c = (byte)pop();
		push(v(IBUF) + v(IPOS));
		while (v(IPOS) < v(ILEN) && d.get(v(IBUF) + v(IPOS)) != c) v(IPOS, v(IPOS) + 1);
		v(IPOS, v(IPOS) + 1);
		push(v(IBUF) + v(IPOS) - T());
	}

	// This can be made from parse_chars and parse (both using space)
	public void parse_name() {
		while (v(IPOS) < v(ILEN) && d.get(v(IBUF) + v(IPOS)) <= 32) v(IPOS, v(IPOS) + 1);
		push(v(IBUF) + v(IPOS));
		while (v(IPOS) < v(ILEN) && d.get(v(IBUF) + v(IPOS)) > 32) v(IPOS, v(IPOS) + 1);
		push(v(IBUF) + v(IPOS) - T());
		v(IPOS, v(IPOS) + 1);
	}

	public void interpret() {
		do {
			parse_name();
			if (T() == 0) break;
			two_dup();
			find_name();
			if (T() > 0) {
				nip(); nip();
				Word w = words.get(pop());
				if (v(STATE) == 0 || w.immediate) {
					if (!w.colon) push(w.dt);
					if (w.xt != -1) eval(w.xt);
				} else {
					if (!w.colon) literal(w.dt);
					if (w.xt != -1) compile(w.xt);
				}
			} else {
				drop();
				convert_to_number();
			}
		} while(true);
		two_drop();
	}

	public void evaluate() { 
		// Save previous source
		int ipos = v(IPOS);
		int ilen = v(ILEN);
		int ibuf = v(IBUF);
		// Set source to string on stack and interpret
		v(IPOS, 0);
		v(ILEN, pop());
		v(IBUF, pop());
		interpret();
		// Restore previous source
		v(IPOS, ipos);
		v(ILEN, ilen);
		v(IBUF, ibuf);
	}
	public void evaluate(String s) { str_to_data(s); evaluate(); }

	public void quit() { /* TODO */	}

	public void primitive(String name, Consumer<Dodo> code) {
		primitives.add(code);
		create(name);
		latest().colon = true;
		latest().xt = 0 - (primitives.size() - 1);
	}

	// REORGANIZATION

	// Kernel primitive index in primitives table
	public static int BRANCH = -9;
	public static int ZBRANCH = -10;
	public static int STRING = -11;
	public static int QUOTATION = -12;
	public static int LIT = -13;
	public static int DODOES = -14;
	public static int DOPLUSLOOP = -15;
	public static int COMPILE = -16;
	public static int EXIT = -18;
	public static int PLUS = -19;
	public static int ONE_PLUS = -20;
	public static int EQUALS = -21;
	public static int TO_R = -22;
	public static int DROP = -24;
	public static int OVER = -26;
	public static int FROM_R = -27;
	public static int TYPE = -28;

	public void kernel() {
		wordlist(); drop();	// Create ROOT wordlist
		current = 0;				// Set it to be the current compilation wordlist and the only one to search
		context[0] = 0;		
		context[1] = -1;
		v(BASE, 10);
		v(IPOS, 0);
		v(ILEN, 0);
		v(IBUF, 0);
		v(STATE, 0);
		// Without this first colon, bye does not work ?!
		/* 00 */ primitive("FIRST_COLON", (vm) -> { });
		/* 01 */ primitive("NOOP", (vm) -> { });
		/* 02 */ primitive("BYE", (vm) -> System.exit(0));
		/* 03 */ primitive("SP@", (vm) -> vm.push(vm.sp));
		/* 04 */ primitive("SP!", (vm) -> vm.sp = vm.pop());
		/* 05 */ primitive("SP0", (vm) -> push(0));
		/* 06 */ primitive("RP@", (vm) -> vm.push(vm.rp));
		/* 07 */ primitive("RP!", (vm) -> vm.rp = vm.pop());
		/* 08 */ primitive("RP0", (vm) -> push(0));
		/* 09 */ primitive("(BRANCH)", (vm) -> vm.branch());
		/* 10 */ primitive("(?BRANCH)", (vm) -> vm.zbranch());
		/* 11 */ primitive("(STRING)", (vm) -> vm.string());
		/* 12 */ primitive("(QUOTATION)", (vm) -> vm.quotation());
		/* 13 */ primitive("(LIT)", (vm) -> vm.lit());
		/* 14 */ primitive("(DOES>)", (vm) -> vm.dodoes());
		/* 15 */ primitive("(+LOOP)", (vm) -> vm.plus_loop_p());
		/* 16 */ primitive("COMPILE,", (vm) -> vm.compile());
		/* 17 */ primitive("EXECUTE", (vm) -> vm.execute());
		/* 18 */ primitive("EXIT", (vm) -> vm.exit());
		/* 19 */ primitive("+", (vm) -> vm.plus());
		/* 20 */ primitive("1+", (vm) -> vm.one_plus());
		/* 21 */ primitive("=", (vm) -> vm.equals());
		/* 22 */ primitive(">R", (vm) -> vm.to_r());
		/* 23 */ primitive("@", (vm) -> vm.fetch());
		/* 24 */ primitive("DROP", (vm) -> vm.drop());
		/* 25 */ primitive("DUP", (vm) -> vm.dup());
		/* 26 */ primitive("OVER", (vm) -> vm.over());
		/* 27 */ primitive("R>", (vm) -> vm.from_r());
		/* 28 */ primitive("TYPE", (vm) -> vm.type());
		/* 29 */ primitive("!", (vm) -> vm.store());
		/* 30 */ primitive(":", (vm) -> vm.colon());
		/* 31 */ primitive(";", (vm) -> vm.semicolon()); immediate();
		/* 32 */ primitive("IMMEDIATE", (vm) -> vm.immediate());
	}

	public void digit() {
		to_r();
		dup(); 
		if (pop() >= ((byte)'a')) {
			push((byte)'a'); minus(); push((byte)'A'); plus();
		}
		dup(); dup(); 
		if (pop() > (((byte)'A') - 1)) {
			push((byte)'A'); minus(); push((byte)'9'); plus(); one_plus();
		} else {
			dup();
			if (pop() > (byte)'9') {
				drop(); push(0);
			}
		}
		push((byte)'0'); minus();
		dup(); from_r(); less_than();
		if (pop() != 0) {
			dup(); one_plus();
			if (pop() > 0) {
				nip(); push(-1);
			} else {
				drop(); push(0);
			}
		} else {
			drop(); push(0);
		}
	}

	public void to_number() {
		// Adapted from pForth
		to_r();
		do {
			r_fetch();
			if (pop() > 0) {
				dup(); cfetch(); push(vaddr(BASE)); fetch();
				digit();
				if (pop() != 0) {
					push(-1);
				} else {
					drop(); push(0);
				}
			} else {
				push(0);
			}
			if (pop() == 0) break;
			swap(); to_r();
			swap(); push(vaddr(BASE)); fetch();
			u_m_times(); drop();
			rot(); push(vaddr(BASE)); fetch();
			u_m_times();
			d_plus();
			from_r(); one_plus();
			from_r(); one_minus(); to_r();
		} while(true);
		from_r();
	}

	// CORE WORDS (that are not in the kernel set)

	// --- Numeric output

	public void num_start() { tallot(100); v(HOLD_END, there); v(HLD, there); }
	public void num() { 
		push(vaddr(BASE)); fetch(); 
		u_d_div_mod(); rot(); push(9); over(); less_than();
		if (pop() != 0) { push(7); plus(); }
		push(48); plus(); hold();
	}
	public void num_s() { 
		do { num(); over(); over(); or(); } while (pop() != 0); 
	}
	public void hold() { 
		push(vaddr(HLD)); fetch(); one_minus(); dup(); 
		push(vaddr(HLD)); store(); 
		cstore(); 
	}
	public void sign() { if (pop() < 0) { push(45); hold(); } }
	public void holds() { 
		while (true) { 
			dup(); if (pop() == 0) break; 
			one_minus(); two_dup(); plus(); cfetch(); hold(); 
		} 
		two_drop(); 
	}
	public void num_end() { 
		drop(); drop(); 
		push(vaddr(HLD)); fetch(); 
		push(v(HOLD_END)); over(); minus(); 
	}

	public void d_dot_r() { 
		to_r(); swap(); over(); dabs(); 
		num_start(); num_s(); rot(); sign(); num_end(); 
		from_r(); over(); minus(); spaces(); type(); 
	}
	public void d_dot() { push(0); d_dot_r(); space(); }
	public void dot() { dup(); push(0); less_than(); d_dot();	}
	public void ud_dot_r() { 
		to_r(); 
		num_start(); num_s(); num_end(); 
		from_r(); over(); minus(); spaces(); type(); 
	}
	public void ud_dot() { push(0); ud_dot_r(); space(); }
	public void u_dot() { push(0); ud_dot(); }

	public void tick() { parse_name(); find_name(); push(words.get(pop()).xt); }

	// --- Parsing
	public void word() { dup(); parse_chars(); drop(); drop(); parse(); one_minus(); to_counted_string(); }
	public void paren() { push(41); parse(); drop(); drop(); }

	public void to_in() { push(vaddr(IPOS)); }
	public void base() { push(vaddr(BASE)); }
	public void state() { push(vaddr(STATE)); }
	public void decimal() { v(BASE, 10); }

	public void core() {
		primitive("#", (vm) -> vm.num());
		primitive("#>", (vm) -> vm.num_end());
		primitive("#S", (vm) -> vm.num_s());
		primitive("'", (vm) -> vm.tick());
		primitive("(", (vm) -> vm.paren()); immediate();
		primitive("*", (vm) -> vm.times());
		primitive("*/", (vm) -> vm.times_div());
		primitive("*/MOD", (vm) -> vm.times_div_mod());
		primitive("+!", (vm) -> vm.plus_store());
		primitive("+LOOP", (vm) -> vm.plus_loop()); immediate();
		primitive(",", (vm) -> vm.comma());
		primitive("-", (vm) -> vm.minus());
		primitive(".", (vm) -> vm.dot());
		primitive(".\"", (vm) -> vm.dot_quote()); immediate();
		primitive("/", (vm) -> vm.div());
		primitive("/MOD", (vm) -> vm.div_mod());
		primitive("0<", (vm) -> vm.zero_less());
		primitive("0=", (vm) -> vm.zero_equals());
		primitive("1-", (vm) -> vm.one_minus());
		primitive("2!", (vm) -> vm.two_store());
		primitive("2*", (vm) -> vm.two_times());
		primitive("2/", (vm) -> vm.two_div());
		primitive("2@", (vm) -> vm.two_fetch());
		primitive("2DROP", (vm) -> vm.two_drop());
		primitive("2DUP", (vm) -> vm.two_dup());
		primitive("2OVER", (vm) -> vm.two_over());
		primitive("2SWAP", (vm) -> vm.two_swap());
		primitive("<", (vm) -> vm.less_than());
		primitive("<#", (vm) -> vm.num_start());
		primitive(">", (vm) -> vm.greater_than());
		primitive(">BODY", (vm) -> vm.to_body());
		primitive(">IN", (vm) -> vm.to_in());
		primitive(">NUMBER", (vm) -> vm.to_number());
		primitive("?DUP", (vm) -> vm.qdup());
		primitive("ABORT", (vm) -> vm.abort());
		primitive("ABORT\"", (vm) -> vm.abortq());
		primitive("ABS", (vm) -> vm.abs());
		primitive("ACCEPT", (vm) -> vm.accept());
		primitive("ALIGN", (vm) -> vm.align());
		primitive("ALIGNED", (vm) -> vm.aligned());
		primitive("ALLOT", (vm) -> vm.allot());
		primitive("AND", (vm) -> vm.and());
		primitive("BASE", (vm) -> vm.base());
		primitive("BEGIN", (vm) -> vm.begin()); immediate();
		primitive("BL", (vm) -> vm.bl());
		primitive("C!", (vm) -> vm.cstore());
		primitive("C,", (vm) -> vm.ccomma());
		primitive("C@", (vm) -> vm.cfetch());
		primitive("CELL+", (vm) -> vm.cell_plus());
		primitive("CELLS", (vm) -> vm.cells());
		primitive("CHAR", (vm) -> vm._char());
		primitive("CHAR+", (vm) -> vm.char_plus());
		primitive("CHARS", (vm) -> vm.chars());
		primitive("CONSTANT", (vm) -> vm.constant());
		primitive("COUNT", (vm) -> vm.count());
		primitive("CR", (vm) -> vm.cr());
		primitive("CREATE", (vm) -> vm.create());
		primitive("DECIMAL", (vm) -> vm.decimal());
		primitive("DEPTH", (vm) -> vm.depth());
		primitive("DO", (vm) -> vm._do()); immediate();
		primitive("DOES>", (vm) -> vm.does()); immediate();
		primitive("ELSE", (vm) -> vm._else()); immediate();
		primitive("ENVIRONMENT?", (vm) -> vm.environment());
		primitive("EVALUATE", (vm) -> vm.evaluate());
		primitive("FILL", (vm) -> vm.fill());
		primitive("FIND", (vm) -> vm.find());
		primitive("FM/MOD", (vm) -> vm.fm_div_mod());
		primitive("HERE", (vm) -> vm.here());
		primitive("HOLD", (vm) -> vm.hold());
		primitive("I", (vm) -> vm.i());
		primitive("IF", (vm) -> vm._if()); immediate();
		primitive("INVERT", (vm) -> vm.invert());
		primitive("J", (vm) -> vm.j());
		primitive("LEAVE", (vm) -> vm.leave());
		primitive("LITERAL", (vm) -> vm.literal()); immediate();
		primitive("LOOP", (vm) -> vm.loop()); immediate();
		primitive("LSHIFT", (vm) -> vm.lshift());
		primitive("M*", (vm) -> vm.m_times());
		primitive("MAX", (vm) -> vm.max());
		primitive("MIN", (vm) -> vm.min());
		primitive("MOD", (vm) -> vm.mod());
		primitive("MOVE", (vm) -> vm.move());
		primitive("NEGATE", (vm) -> vm.negate());
		primitive("OR", (vm) -> vm.or());
		primitive("POSTPONE", (vm) -> vm.postpone()); immediate();
		primitive("QUIT", (vm) -> vm.quit());
		primitive("R@", (vm) -> vm.r_fetch());
		primitive("RECURSE", (vm) -> vm.recurse()); immediate();
		primitive("REPEAT", (vm) -> vm.repeat()); immediate();
		primitive("ROT", (vm) -> vm.rot());
		primitive("RSHIFT", (vm) -> vm.rshift());
		primitive("S\"", (vm) -> vm.s_quote()); immediate();
		primitive("S>D", (vm) -> vm.s_to_d());
		primitive("SIGN", (vm) -> vm.sign());
		primitive("SM/REM", (vm) -> vm.sm_div_rem());
		primitive("SOURCE", (vm) -> vm.source());
		primitive("SPACE", (vm) -> vm.space());
		primitive("SPACES", (vm) -> vm.spaces());
		primitive("STATE", (vm) -> vm.state());
		primitive("SWAP", (vm) -> vm.swap());
		primitive("THEN", (vm) -> vm.then()); immediate();
		primitive("U.", (vm) -> vm.u_dot());
		primitive("U<", (vm) -> vm.u_less());
		primitive("UM*", (vm) -> vm.u_m_times());
		primitive("UM/MOD", (vm) -> vm.u_m_div_mod());
		primitive("UNLOOP", (vm) -> vm.unloop());
		primitive("UNTIL", (vm) -> vm.until()); immediate();
		primitive("VARIABLE", (vm) -> vm.variable());
		primitive("WHILE", (vm) -> vm._while()); immediate();
		primitive("WORD", (vm) -> vm.word());
		primitive("XOR", (vm) -> vm.xor());
		primitive("[", (vm) -> vm.bracket()); immediate();
		primitive("[']", (vm) -> vm.bracket_tick()); immediate();
		primitive("[CHAR]", (vm) -> vm.bracket_char()); immediate();
		primitive("]", (vm) -> vm.right_bracket());
	}

	// CORE EXTENSION WORDS

	public void dot_paren() { push(41); parse(); one_minus(); type(); }
	public void dot_r() { push(0); swap(); d_dot_r(); }
	public void zero_ne() { push(pop() != 0 ? -1 : 0); }
	public void zero_more() { push(pop() > 0 ? -1 : 0); }
	public void two_to_r() { swap(); to_r(); to_r(); }
	public void two_from_r() { from_r(); from_r(); swap(); }
	public void two_r_fetch() { push(r[rp - 2]); push(r[rp - 1]); }
	public void noname() { latestxt = d.position(); push(latestxt); right_bracket(); }
	public void not_equals() { s[sp - 2] = s[sp - 2] != s[sp - 1] ? -1 : 0; sp--; }
	public void q_do() {
		// This one is tricky, as it needs to allow use of LOOP, +LOOP, UNLOOP and LEAVE
		push(d.position() + 4); // position to be resolved for every leave
		literal(0); // placeholder for pushing were to jump on a leave
		compile(TO_R); // move leave address to return stack
		// Compile check equals
		compile(OVER);
		compile(OVER);
		compile(EQUALS);
		_if();
		compile(DROP);
		compile(DROP);
		// If equals exit
		compile(EXIT);
		then();
		// next is like do
		begin();	// compile a backwards mark
		compile(TO_R);	// move index to return stack
		compile(TO_R); // move limit to return stack
	}
	public void buffer() { create(); allot(); }
	public void hex() { v(BASE, 16); }
	public void marker() {
		create();
		// TODO: marker is a defining word. It should set latest().xt to the xt of a word
		// that does the delete based on the dt of that word.
	}
	public void tuck() { swap(); over(); }
	public void u_more() { long b = upop(); push(upop() > b ? -1 : 0); }
	public void unused() { push(d.capacity() - d.position()); }
	public void backslash() { push(10); parse(); drop(); drop(); }

	public String defer_compat = """
	\\ deferred words and perform

	\\ This file is in the public domain. NO WARRANTY.
	
	: perform ( ? addr -- ? )
	    @ execute ;
	
	: defer ( "name" -- )
	    create ['] abort , \\ you should not rely on initialization with noop
	does> ( ? -- ? )
	    perform ;
	
	: defer@ ( xt1 -- xt2 )
	  >body @ ;
	
	: defer! ( xt2 xt1 -- )
	  >body ! ;
	
	: <is> ( xt "name" -- )
	    ' defer! ;
	
	: [is] ( compilation: "name" -- ; run-time: xt -- )
	    postpone ['] postpone defer! ; immediate
	
	: is
	  state @ if
	    postpone [is]
	  else
	    <is>
	  then ; immediate
	
	: action-of
	 state @ if
	   POSTPONE ['] POSTPONE defer@
	 else
	   ' defer@
	then ; immediate
	
	""";

	public void core_extensions() {
		primitive(".(", (vm) -> vm.dot_paren()); immediate();
		primitive(".R", (vm) -> vm.dot_r());
		primitive("0<>", (vm) -> vm.zero_ne());
		primitive("0>", (vm) -> vm.zero_more());
		primitive("2>R", (vm) -> vm.two_to_r());
		primitive("2R>", (vm) -> vm.two_from_r());
		primitive("2R@", (vm) -> vm.two_r_fetch());
		primitive(":NONAME", (vm) -> vm.noname());
		primitive("<>", (vm) -> vm.not_equals());
		primitive("?DO", (vm) -> q_do()); immediate();
		primitive("AGAIN", (vm) -> vm.again()); immediate();
		primitive("BUFFER:", (vm) -> vm.buffer());
		primitive("HEX", (vm) -> vm.hex());
		primitive("MARKER", (vm) -> vm.marker());
		primitive("NIP", (vm) -> vm.nip());
		primitive("PARSE", (vm) -> vm.parse());
		primitive("PARSE-NAME", (vm) -> vm.parse_name());
		evaluate(": PICK ( x0 i*x u.i -- x0 i*x x0 ) dup 0= if drop dup exit then swap >r 1- recurse r> swap ;");
		evaluate(": ROLL ( x0 i*x u.i -- i*x x0 ) dup 0= if drop exit then swap >r 1- recurse r> swap ;");
		primitive("TUCK", (vm) -> vm.tuck());
		primitive("U>", (vm) -> u_more());
		primitive("UNUSED", (vm) -> unused());
		evaluate(": WITHIN ( n1|u1 n2|u2 n3|u3 -- flag ) over - >r - r> u< ;");
		primitive("\\", (vm) -> vm.backslash()); immediate();
		evaluate(defer_compat);
	}

	// TOOLS

	public void dump() {
		int n = pop();
		int a = pop();
		if (n == -1) n = 64;
		while (n > 0) {
			System.out.printf("\n%04X:%04X  ", (a >> 16) & 0xFFFF, a & 0xFFFF);
			for (int i = 0; i < 8; i++) System.out.printf("%02X ", d.get(a + i) & 0xFF);
			System.out.printf(" ");
			for (int i = 0; i < 8; i++) System.out.printf("%02X ", d.get(a + 8 + i) & 0xFF);
			System.out.printf("   ");
			for (int i = 0; i < 16; i++) {
				byte k = d.get(a + i);
				if (k >= 32 && k <= 127) System.out.printf("%c", k);
				else System.out.printf(".");
			}
			a += 16;
			n -= 16;
		}
		System.out.println();
	}

	public void dot_s() {
		System.out.printf("<%d> ", sp);
		for (int i = 0; i < sp; i++) System.out.printf("%d ", s[i]);
	}

	public void trace() {
		System.out.printf("[%d] ", v(STATE));
		dot_s();
	}

	public void tools() {
		primitive("DUMP", (vm) -> vm.dump());
		primitive(".S", (vm) -> vm.dot_s());
		primitive("TRACE", (vm) -> vm.trace());
	}

	// FILE

	public void file() {
		primitive("INCLUDED", (vm) -> vm.included());
	}

	// DOUBLE

	public void dabs() { long a = dpop(); if (a < 0) a = 0 - a; dpush(a); }
	public void d_minus() { long b = dpop(); dpush(dpop() - b); }
	public void dnegate() { long a = dpop(); dpush(0 - a); }
	public void d_plus() { long b = dpop(); dpush(dpop() + b); }

	public void _double() {
		primitive("DABS", (vm) -> vm.dabs());
		primitive("D-", (vm) -> vm.d_minus());
		primitive("DNEGATE", (vm) -> vm.dnegate());
		primitive("D+", (vm) -> vm.d_plus());
	}

	public void bootstrap() {
		kernel();
		core();
		core_extensions();
		tools();
		file();
		_double();
	}
}
