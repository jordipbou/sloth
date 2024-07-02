# TODO
# I need to implement [ ]
# They must do two things, create a new lexical scope (that will be 


import sys
import operator

import getch
from colorama import Fore, Back, Style

class Sloth:
    def __init__(self):
        # VM initialization
        self.init_stack()
        self.init_rstack()
        self.init_mem()
        self.init_inner()

        # Symbols
        self.init_symbols()

        # Outer interpreter
        self.init_outer()

        # Input/Output
        self.init_io()

        # Combinators
        self.init_combinators()

        # Bootstrap
        self.bootstrap()

        # Debug
        self.init_debug()

    # -------------------------------------------------------------------------
    # -- Virtual Machine ------------------------------------------------------
    # -------------------------------------------------------------------------

    # -- Parameter stack --

    def init_stack(self): self.s = [0]*256; self.sp = 0; self.lp = 256

    def push(self, v): self.s[self.sp] = v; self.sp = self.sp + 1
    def pop(self): v = self.s[self.sp - 1]; self.sp = self.sp - 1; return v

    def pick(self, a): return self.s[self.sp - a - 1]
    def place(self, a, v): self.s[self.sp - a - 1] = v

    # -- Locals stack --

    def lpush(self): self.lp = self.lp - 1; self.s[self.lp] = self.pop()
    def lpop(self): self.push(self.s[self.lp]); self.lp = self.lp + 1

    def lpick(self, a): return self.s[self.lp + a]
    def lplace(self, a, v): self.s[self.lp + a] = v

    # -- Return stack --

    def init_rstack(self): self.r = [0]*256; self.rp = 0

    def rpush(self, v): self.r[self.rp] = v; self.rp = self.rp + 1
    def rpop(self): v = self.r[self.rp - 1]; self.rp = self.rp - 1; return v

    # -- Memory --

    CELL = 1
    CHAR = 1

    def init_mem(self): 
        self.m = [0]*65536
        self.mp = 128 # input buffer

    def store(self, a, v): self.m[a] = v
    def fetch(self, a): return self.m[a]

    def cstore(self, a, v): self.m[a] = v
    def cfetch(self, a): return self.m[a]

    def comma(self, v): self.store(self.mp, v); self.mp = self.mp + Sloth.CELL
    def ccomma(self, v): self.cstore(self.mp, v); self.mp = self.mp + Sloth.CHAR

    def here(self): return self.mp
    def set_here(self, v): self.mp = v

    def allot(self, v): t = self.mp; self.mp = self.mp + v; return t

    def align(self): self.set_here((self.here() + (Sloth.CELL - 1)) & ~(Sloth.CELL - 1))

    # -- Inner interpreter --

    def init_inner(self): 
        self.ip = -1
        self.primitives = []

    def do_prim(self, q): self.primitives[-1 - q](self)
    def call(self, q): self.rpush(self.ip); self.ip = q
    def execute(self, q): self.call(q) if q > 0 else self.do_prim(q)
    def exit(self): self.ip = -1 if self.rp == 0 else self.rpop()

    def opcode(self): o = self.m[self.ip]; self.ip = self.ip + 1; return o

    def inner(self): 
        t = self.rp
        while t <= self.rp and self.ip >= 0:
            self.trace(3)
            self.execute(self.opcode())

    def evaluate(self, q): self.execute(q); self.inner()

    def do(self, l): 
        for i in l:
            self.evaluate(i)

    def noname(self, l): self.primitives.append(l); return 0 - len(self.primitives)

    # -- Exceptions --

    def throw(self, v): 
        if not v == 0:
            raise RuntimeError('', v)

    def catch(self, q):
        tsp = self.sp
        trp = self.rp
        try:
            self.evaluate(q)
            self.push(0)
        except RuntimeError as e:
            self.sp = tsp
            self.rp = trp
            self.push(e.args[1])

    # -------------------------------------------------------------------------
    # -- Basic VM instructions ------------------------------------------------
    # -------------------------------------------------------------------------

    def drop(self): self.pop()
    def dup(self): self.push(self.pick(0))
    def swap(self): a = self.pop(); b = self.pop(); self.push(a); self.push(b)
    def over(self): self.push(self.pick(1))

    # Helper for all binary operators
    def binop(self, o): a = self.pop(); self.push(o(self.pop(), a))
    def lbinop(self, o): a = self.pop(); r = o(self.pop(), a); self.push(-1 if r else 0)

    def _invert(self): self.place(0, ~self.pick(0))

    # -------------------------------------------------------------------------
    # -- Symbols --------------------------------------------------------------
    # -------------------------------------------------------------------------

    NO_FLAGS = 0
    # HIDDEN = 1
    COLON = 2
    IMMEDIATE = 4

    def init_symbols(self):
        self.latest = 0
        self.latestxt = 0

    def get_link(self, w): return self.fetch(w)
    def set_link(self, w, v): self.store(w, v)

    def get_xt(self, w): return self.fetch(w + Sloth.CELL)
    def set_xt(self, w, v): self.store(w + Sloth.CELL, v)

    def get_dt(self, w): return self.fetch(w + 2*Sloth.CELL)
    def set_dt(self, w, v): self.store(w + 2*Sloth.CELL, v)

    def get_flags(self, w): return self.cfetch(w + 3*Sloth.CELL)
    def set_flags(self, w, f): self.cstore(w + 3*Sloth.CELL, f)

    def has_flag(self, w, f): return (self.get_flags(w) & f) == f
    def set_flag(self, w, f): self.set_flags(w, self.get_flags(w) | f)
    def unset_flag(self, w, f): self.set_flags(w, self.get_flags(w) & ~f)

    def get_namelen(self, w): return self.cfetch(w + 3*Sloth.CELL + Sloth.CHAR)
    def set_namelen(self, w, v): self.cstore(w + 3*Sloth.CELL + Sloth.CHAR, v)

    def name_addr(self, w): return w + 3*Sloth.CELL + 2*Sloth.CHAR

    def create_symbol(self, a, u):
        w = self.allot(3*Sloth.CELL + 2*Sloth.CHAR + u*Sloth.CHAR); self.align()
        self.set_link(w, self.latest); self.latest = w
        self.set_xt(w, 0)
        self.set_dt(w, self.here())
        self.set_flags(w, Sloth.NO_FLAGS)
        self.set_namelen(w, u)
        for i in range(u):
            self.cstore(self.name_addr(w) + i*Sloth.CHAR, self.cfetch(a + i*Sloth.CHAR))
        return w

    def compare_no_case(self, a, u, w):
        if not u == self.get_namelen(w):
            return False
        for i in range(u):
            c1 = self.cfetch(a + i*Sloth.CHAR)
            c2 = self.cfetch(self.name_addr(w) + i*Sloth.CHAR)
            c1 = (c1 - 32) if 97 <= c1 <= 122 else c1
            c2 = (c2 - 32) if 97 <= c2 <= 122 else c2
            if not c1 == c2:
                return False
        return True

    def find_symbol(self, a, u):
        w = self.latest
        while not w == 0:
            if self.compare_no_case(a, u, w):
                break
            w = self.get_link(w)
        return w

    def eval_symbol(self, w):
        if not self.has_flag(w, Sloth.COLON):
            self.push(self.get_dt(w))
        if not self.get_xt(w) == 0:
            self.evaluate(self.get_xt(w))

    def compile_symbol(self, w):
        if not self.has_flag(w, Sloth.COLON):
            self.comma(self.doLIT)
            self.comma(self.get_dt(w))
        if not self.get_xt(w) == 0:
            self.comma(self.get_xt(w))

    # -------------------------------------------------------------------------
    # -- Outer interpreter (for bootstrapping) --------------------------------
    # -------------------------------------------------------------------------

    def init_outer(self):
        self.ibuf = 0
        self.ipos = 0
        self.ilen = 0

        self.tok = 0
        self.tlen = 0

        self.STATE = 0

    def token(self):
        while self.ipos < self.ilen and self.cfetch(self.ibuf + self.ipos*Sloth.CHAR) < 33:
            self.ipos = self.ipos + Sloth.CHAR 
        self.tok = self.ibuf + self.ipos*Sloth.CHAR
        self.tlen = 0
        while self.ipos < self.ilen and self.cfetch(self.ibuf + self.ipos*Sloth.CHAR) > 32:
            self.ipos = self.ipos + Sloth.CHAR 
            self.tlen = self.tlen + 1
        self.push(self.tok); self.push(self.tlen)

    def outer(self):
        while self.ipos < self.ilen:
            self.token()
            self.trace(2)
            u = self.pop(); a = self.pop()
            if u == 0:
                break
            w = self.find_symbol(a, u)
            if not w == 0:
                if self.STATE == 0 or self.has_flag(w, Sloth.IMMEDIATE):
                    self.eval_symbol(w)
                else:
                    self.compile_symbol(w)
            else:
                s = self.data_to_str(a, u)
                try:
                    n = int(s)
                    if self.STATE == 0:
                        self.push(n)
                    else:
                        self.comma(self.doLIT)
                        self.comma(n)
                except ValueError as e:
                    try:
                        f = float(s)
                        if self.STATE == 0:
                            self.push(f)
                        else:
                            self.comma(doFLIT)
                            self.comma(f)
                    except ValueError as e:
                        self.throw(-13)
            self.trace(2)

    # -------------------------------------------------------------------------
    # -- I/O ------------------------------------------------------------------
    # -------------------------------------------------------------------------

    def init_io(self):
        self.KEY = 0
        self.EMIT = 0

    def key(self):
        if self.KEY == 0:
           self.push(ord(getch.getch()))
        else:
            self.evaluate(self.KEY)

    def emit(self):
        if self.EMIT == 0:
            print(chr(self.pop()), sep='', end='', flush=True)
        else:
            self.evaluate(self.EMIT)

    def included(self):
        # Save input source
        u = self.pop(); a = self.pop()
        with open(self.data_to_str(a, u)) as f:
            for line in f:
                self.str_to_data(line, 0)
                self.catch(self.OUTER)
                r = self.pop()
                if r == -13:
                    self.push(self.tok); self.push(self.tlen); self.type(); print(' ?')
                self.trace(0)
        # Restore input source

    # -------------------------------------------------------------------------
    # -- Combinators ----------------------------------------------------------
    # -------------------------------------------------------------------------

    def init_combinators(self): self.lx = 0; self.ix = 0; self.jx = 0; self.kx = 0

    def when(self): 
        t = self.pop(); c = self.pop()
        if not c == 0:
            self.evaluate(t)

    def unless(self):
        f = self.pop(); c = self.pop();
        if c == 0:
            self.evaluate(f)

    def choose(self):
        f = self.pop(); t = self.pop(); c = self.pop()
        if not c == 0:
            self.evaluate(t)
        else:
            self.evaluate(f)

    def nif(self):
        qp = self.pop(); qz = self.pop(); qn = self.pop(); n = self.pop()
        if n < 0:
            self.evaluate(qn)
        elif n == 0:
            self.evaluate(qz)
        elif n > 0:
            self.evaluate(qp)

    # -- Loops --

    def ipush(self): self.rpush(self.kx); self.kx = self.jx; self.jx = self.ix; self.lx = 0
    def ipop(self): self.ix = self.jx; self.jx = self.kx; self.kx = self.rpop(); self.lx = 0

    def set_i(self): self.ix = self.pop()

    def loop(self): 
        q = self.pop(); 
        self.ipush()
        while self.lx == 0:
            self.evaluate(q)
            self.ix = self.ix + 1
        self.ipop()

    # TODO Investigate how to do 2LEAVE and 3LEAVE
    def leave(self, v): 
        if not self.pop() == 0:
            self.lx = 1
            self.exit()

    def call_leave(self):
        q = self.pop()
        if not self.pop() == 0:
            self.evaluate(q)
            self.lx = 1
            self.exit()

    def times(self):
        q = self.pop(); n = self.pop()
        self.ipush()
        for self.ix in range(n):
            self.evaluate(q)
            if not self.lx == 0:
                break
        self.ipop()

    def while_(self):
        q = self.pop(); c = self.pop()
        while True:
            self.evaluate(c)
            if self.pop() == 0:
                break
            self.evaluate(q)

    def two_while_(self):
        q = self.pop(); c2 = self.pop(); c1 = self.pop()
        while True:
            self.evaluate(c1)
            if self.pop() == 0:
                break
            self.evaluate(c2)
            if self.pop() == 0:
                break
            self.evaluate(q)

    # -- Dataflow

    # Preserving

    def dip(self):
        q = self.pop(); x = self.pop()
        self.evaluate(q)
        self.push(x)

    def two_dip(self): 
        q = self.pop(); y = self.pop(); x = self.pop()
        self.evaluate(q)
        self.push(x); self.push(y)

    def three_dip(self): 
        q = self.pop(); z = self.pop(); y = self.pop(); x = self.pop()
        self.evaluate(q)
        self.push(x); self.push(y); self.push(z)

    def four_dip(self): 
        q = self.pop(); z = self.pop(); y = self.pop(); x = self.pop(); w = self.pop()
        self.evaluate(q)
        self.push(w); self.push(x); self.push(y); self.push(z)

    def keep(self):
        q = self.pop(); x = self.pick(0)
        self.evaluate(q)
        self.push(x)

    def two_keep(self):
        q = self.pop(); y = self.pick(0); x = self.pick(1)
        self.evaluate(q)
        self.push(x); self.push(y)

    def three_keep(self):
        q = self.pop(); z = self.pick(0); y = self.pick(1); x = self.pick(2)
        self.evaluate(q)
        self.push(x); self.push(y); self.push(z)

    # Cleave

    def bi(self):
        q = self.pop(); p = self.pop(); x = self.pick(0)
        self.evaluate(p)
        self.push(x)
        self.evaluate(q)

    # -- Recursion

    def _binrec(self, q1, q2, q3, q4):
        self.eval(q1)
        if self.pop() != 0:
            self.evaluate(q2)
        else:
            self.evaluate(q3)
            self._binrec(q1, q2, q3, q4)
            self.swap()
            self._binrec(q1, q2, q3, q4)
            self.evaluate(q4)

    def binrec(self):
        q4 = self.pop(); q3 = self.pop(); q2 = self.pop(); q1 = self.pop()
        self._binrec(q1, q2, q3, q4)

    # -------------------------------------------------------------------------
    # -- Bootstrapping --------------------------------------------------------
    # -------------------------------------------------------------------------

    def get_ibuf(self): return self.ibuf
    def set_ibuf(self, v): self.ibuf = v
    def get_ipos(self): return self.ipos
    def set_ipos(self, v): self.ipos = v
    def get_ilen(self): return self.ilen
    def set_ilen(self, v): self.ilen = v

    def str_to_data(self, s, a):
        for i in range(len(s)):
            self.m[a + i*Sloth.CHAR] = ord(s[i])
        self.ibuf = a
        self.ipos = 0
        self.ilen = len(s)
        self.tok = 0
        self.tlen = 0

    def pad(self): return self.here() + 1024

    def colon(self, n, l):
        self.str_to_data(n, self.pad())
        w = self.create_symbol(self.pad(), len(n))
        xt = self.noname(l)
        self.set_xt(w, xt)
        self.set_flag(w, Sloth.COLON)
        return xt

    def immediate(self): self.set_flag(self.latest, Sloth.IMMEDIATE)

    def bootstrap(self):
        self.EXIT = self.colon('EXIT', lambda vm: vm.exit())
        self.doLIT = self.colon('doLIT', lambda vm: vm.do_lit())
        self.doFLIT = self.colon('doFLIT', lambda vm: vm.do_flit())
        self.doQUOTE = self.colon('doQUOTE', lambda vm: vm.do_quote())
        self.doSTRING = self.colon('doSTRING', lambda vm: vm.do_string())
        
        self.COMPILE = self.colon('COMPILE', lambda vm: vm.comma(vm.pop()))
        self.colon('LITERAL', lambda vm: vm.literal(vm.pop())); self.immediate()
        self.colon('EXECUTE', lambda vm: vm.execute(vm.pop()))
        self.colon('BYE', lambda vm: exit())

        self.OUTER = self.noname(lambda vm: vm.outer())

        self.colon('S"', lambda vm: vm.strlit()); self.immediate()

        # Stack shuffling

        self.colon('DROP', lambda vm: vm.drop())
        self.colon('DUP', lambda vm: vm.dup())
        self.colon('SWAP', lambda vm: vm.swap())
        self.colon('OVER', lambda vm: vm.over())

        self.colon('>R', lambda vm: vm.rpush(vm.pop()))
        self.colon('R>', lambda vm: vm.push(vm.rpop()))

        # Locals stack

        self.colon('>L', lambda vm: vm.lpush())
        self.colon('L>', lambda vm: vm.lpop())
        self.colon('X', lambda vm: vm.push(vm.lpick(0)))
        self.colon('Y', lambda vm: vm.push(vm.lpick(1)))
        self.colon('Z', lambda vm: vm.push(vm.lpick(2)))
        self.colon('U', lambda vm: vm.push(vm.lpick(3)))
        self.colon('X!', lambda vm: vm.lplace(0, vm.pop()))
        self.colon('Y!', lambda vm: vm.lplace(1, vm.pop()))
        self.colon('Z!', lambda vm: vm.lplace(2, vm.pop()))
        self.colon('U!', lambda vm: vm.lplace(3, vm.pop()))

        # Memory access

        self.colon('@', lambda vm: vm.push(vm.fetch(vm.pop())))
        self.colon('!', lambda vm: vm.store(vm.pop(), vm.pop()))
        self.colon('C@', lambda vm: vm.push(vm.cfetch(vm.pop())))
        self.colon('C!', lambda vm: (a := vm.pop(), vm.cstore(a, vm.pop())))
        self.colon('HERE', lambda vm: vm.push(vm.here()))
        self.colon('HERE!', lambda vm: vm.set_here(vm.pop()))
        self.colon('CELL', lambda vm: vm.push(Sloth.CELL))
        self.colon('CHAR', lambda vm: vm.push(Sloth.CHAR))

        # Arithmetic

        self.colon('+', lambda vm: vm.binop(operator.add))
        self.colon('-', lambda vm: vm.binop(operator.sub))
        self.colon('*', lambda vm: vm.binop(operator.mul))
        self.colon('/', lambda vm: vm.binop(operator.truediv))
        self.colon('MOD', lambda vm: vm.binop(operator.mod))

        # Comparison

        self.colon('<', lambda vm: vm.lbinop(operator.lt))
        self.colon('=', lambda vm: vm.lbinop(operator.eq))
        self.colon('>', lambda vm: vm.lbinop(operator.gt))

        # Bitwise

        self.colon('AND', lambda vm: vm.lbinop(operator.and_))
        self.colon('OR', lambda vm: vm.lbinop(operator.or_))
        self.colon('XOR', lambda vm: vm.lbinop(operator.xor_))
        self.colon('INVERT', lambda vm: vm._invert())

        self.colon('LSHIFT', lambda vm: vm.binop(operator.lshift))
        self.colon('RSHIFT', lambda vm: vm.binop(operator.rshift))

        # Parsing

        self.colon('IBUF', lambda vm: vm.push(vm.get_ibuf()))
        self.colon('IBUF!', lambda vm: vm.set_ibuf(vm.pop()))
        self.colon('IPOS', lambda vm: vm.push(vm.get_ipos()))
        self.colon('IPOS!', lambda vm: vm.set_ipos(vm.pop()))
        self.colon('ILEN', lambda vm: vm.push(vm.get_ilen()))
        self.colon('ILEN!', lambda vm: vm.set_ilen(vm.pop()))

        self.colon('TOKEN', lambda vm: vm.token())

        # Words

        self.colon('VARIABLE', lambda vm: vm.variable())
        self.colon('CREATE', lambda vm: vm.create())
        self.colon(':', lambda vm: vm.colondef())
        self.colon(';', lambda vm: vm.semicolon()); self.immediate()
        self.colon('POSTPONE', lambda vm: vm.postpone()); self.immediate()
        self.colon('IMMEDIATE', lambda vm: vm.immediate())
        # self.colon('RECURSE', lambda vm: vm.comma(vm.latestxt))

        self.colon('FIND', lambda vm: vm.find_name())
        self.colon('LINK', lambda vm: vm.push(vm.get_link(vm.pop())))
        self.colon('XT', lambda vm: vm.push(vm.get_xt(vm.pop())))
        self.colon('XT!', lambda vm: vm.set_xt(vm.pop(), vm.pop()))
        self.colon('DT', lambda vm: vm.push(vm.get_dt(vm.pop())))
        self.colon('DT!', lambda vm: vm.set_dt(vm.pop(), vm.pop()))
        self.colon('NAME', lambda vm: (nt := vm.pop(), vm.push(vm.name_addr(nt)), vm.push(vm.get_namelen(nt))))

        # Input/Output

        self.colon('KEY', lambda vm: vm.key())
        self.colon('EMIT', lambda vm: vm.emit())

        self.colon('INCLUDED', lambda vm: vm.included())

        self.colon('TYPE', lambda vm: vm.type())

        # Quotations and combinators

        self.colon('[', lambda vm: vm.start_quotation()); self.immediate()
        self.colon(']', lambda vm: vm.end_quotation()); self.immediate()

        self.colon('WHEN', lambda vm: vm.when())
        self.colon('UNLESS', lambda vm: vm.unless())
        self.colon('CHOOSE', lambda vm: vm.choose())
        self.colon('NIF', lambda vm: vm.nif())

        self.colon('LOOP', lambda vm: vm.loop())
        self.colon('TIMES', lambda vm: vm.times())
        self.colon('?LEAVE', lambda vm: vm.leave(1))
        # self.colon('2LEAVE', lambda vm: vm.leave(2))
        # self.colon('3LEAVE', lambda vm: vm.leave(3))
        self.colon('?CALL/LEAVE', lambda vm: vm.call_leave())
        self.colon('I', lambda vm: vm.push(vm.ix))
        self.colon('I!', lambda vm: vm.set_i())
        self.colon('J', lambda vm: vm.push(vm.jx))
        self.colon('K', lambda vm: vm.push(vm.kx))

        self.colon('WHILE', lambda vm: vm.while_())
        self.colon('2WHILE', lambda vm: vm.two_while_())

        self.colon('DIP', lambda vm: vm.dip())
        self.colon('2DIP', lambda vm: vm.two_dip())
        self.colon('3DIP', lambda vm: vm.three_dip())
        self.colon('4DIP', lambda vm: vm.four_dip())

        self.colon('KEEP', lambda vm: vm.keep())
        self.colon('2KEEP', lambda vm: vm.two_keep())
        self.colon('3KEEP', lambda vm: vm.three_keep())

        self.colon('BI', lambda vm: vm.bi())

        self.colon('BINREC', lambda vm: vm.binrec())

        # Debug

        self.colon('TRACE!', lambda vm: vm.set_trace(vm.pop()))
        self.colon('DUMP', lambda vm: vm.dump())

    def repl(self):
        while True:
            self.str_to_data(input('>>> '), 0)
            self.catch(self.OUTER)
            r = self.pop()
            if r == -13:
                self.push(self.tok); self.push(self.tlen); self.type(); print(' ?')
            self.trace(1)

    # -------------------------------------------------------------------------
    # -- Words ----------------------------------------------------------------
    # -------------------------------------------------------------------------

    # -- Numbers --

    def do_lit(self): self.push(self.opcode())
    def do_flit(self): self.push(self.opcode())

    def literal(self, v): self.comma(self.doLIT); self.comma(v)

    # -- Strings --

    def do_string(self): 
        n = self.opcode()
        self.push(self.ip)
        self.push(n)
        self.ip = self.ip + n*Sloth.CHAR

    def strlit(self):
        self.ipos = self.ipos + 1
        a = self.ibuf + self.ipos*Sloth.CHAR
        u = 0
        while self.ipos < self.ilen and chr(self.cfetch(self.ibuf + self.ipos*Sloth.CHAR)) != '"':
            self.ipos = self.ipos + 1
            u = u + 1
        self.ipos = self.ipos + 1
        self.comma(self.doSTRING)
        self.comma(u)
        if self.STATE == 0:
            self.push(self.mp)
        for i in range(u):
            self.ccomma(self.cfetch(a + i*Sloth.CHAR))
        if self.STATE == 0:
            self.push(u)

    # -- Word definition --

    def variable(self):
        self.token()
        u = self.pop(); a = self.pop()
        w = self.create_symbol(a, u)

    def create(self):
        self.token()
        u = self.pop(); a = self.pop()
        w = self.create_symbol(a, u)

    def colondef(self):
        self.token()
        u = self.pop(); a = self.pop()
        w = self.create_symbol(a, u)
        self.set_xt(w, self.get_dt(w))
        # self.set_flag(w, Sloth.HIDDEN)
        self.set_flag(w, Sloth.COLON)
        self.STATE = 1

    def semicolon(self):
        self.comma(self.EXIT)
        self.STATE = 0
        # self.unset_flag(self.latest, Sloth.HIDDEN)

    # def postpone(self):
    #     self.token()
    #     u = self.pop(); a = self.pop()
    #     w = self.find_symbol(a, u)
    #     if self.has_flag(w, Sloth.IMMEDIATE):
    #         self.comma(self.get_xt(w))
    #     else:
    #         self.comma(self.doLIT)
    #         self.comma(self.get_xt(w))
    #         self.comma(self.COMPILE)

    def find_name(self):
        u = self.pop(); a = self.pop()
        self.push(self.find_symbol(a, u))

    # -- Quotations --

    def do_quote(self):
        self.push(self.ip + self.CELL)
        self.ip = self.ip + self.opcode()

    def start_quotation(self):
        self.comma(self.doQUOTE)
        self.push(self.mp)
        self.comma(0)
        self.latestxt = self.mp
        if self.STATE == 0:
            self.push(self.mp)
            self.swap()
        self.STATE = self.STATE + 1

    def end_quotation(self):
        self.comma(self.EXIT)
        self.STATE = self.STATE - 1
        a = self.pop()
        self.store(a, self.mp - a)

    # --

    def type(self):
        l = self.pop(); a = self.pop()
        try:
            for i in range(l):
                print(chr(self.fetch(a + i*Sloth.CHAR)), sep='', end='', flush=True)
        except:
            None

    def data_to_str(self, a, u):
        s = ''
        for i in range(u):
            s = s + chr(self.cfetch(a + i*Sloth.CHAR))
        return s


    # -- Debug helpers --

    def init_debug(self):
        self.tr = 1

    def set_trace(self, v): self.tr = v

    def find_xt(self, xt):
        w = self.latest
        while not w == 0:
            if self.get_xt(w) == xt:
                break
            w = self.get_link(w)
        return w

    def trace(self, v):
        if v <= self.tr:
            print('[', self.STATE, '] ', sep='', end='')
            print(self.mp, ' ', list(reversed(self.s[self.lp:])), ' ', self.s[:self.sp], ' ', sep='', end='')
            if self.ip >= 0:
                w = self.find_xt(self.fetch(self.ip))
                if w == 0:
                    print(': [', self.ip, '] ', sep = '', end = '')
                else:
                    print(': ', end='')
                    self.push(self.name_addr(w)); self.push(self.get_namelen(w)); self.type()
                    print(' ', end='')
            if self.rp > 0:
                for i in reversed(range(self.rp)):
                    w = self.find_xt(self.r[i])
                    if w == 0:
                        print(':', self.r[i], '', end = '')
                    else:
                        print(': ', end='')
                        self.push(self.name_addr(w))
                        self.push(self.get_namelen(w))
                        self.type()
                        print(' ', end='')
            print(Fore.YELLOW, end='')
            if self.tlen > 0: self.push(self.tok); self.push(self.tlen); self.type()
            print(Style.RESET_ALL, end='')
            if self.ipos < self.ilen: 
                self.push(self.ibuf + self.ipos)
                self.push(self.ilen - self.ipos)
                self.type()
            print()

    def dump(self):
        n = self.pop()
        a = self.pop()
        for i in range(0, n, 8):
            print('{0:08x}  '.format(a), end='')
            for j in range(i, i+4):
                print('{0:08x} '.format(self.fetch(a + j)), end='')
            print(' ', end='')
            for j in range(i+4, i+8):
                print('{0:08x} '.format(self.fetch(a + j)), end='')
            print(' ', end='')
            for j in range(i, i + 8):
                c = self.cfetch(a + j)
                print(chr(c) if c > 31 else '.', end='')
            print()
            a = a + 8
        

def main():
    x = Sloth()
    x.str_to_data('../../4th/sloth.4th', 0)
    x.push(0); x.push(19)
    x.included()
    x.repl()

if __name__ == '__main__':
    sys.exit(main())
