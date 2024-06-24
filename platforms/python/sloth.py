# TODO
# I need to implement [ ]
# They must do two things, create a new lexical scope (that will be 
# destroyed after ] and enter compilation mode

import sys
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

        # Bootstrap
        self.bootstrap()

    # -------------------------------------------------------------------------
    # -- Virtual Machine ------------------------------------------------------
    # -------------------------------------------------------------------------

    # -- Parameter stack --

    def init_stack(self): self.s = [0]*256; self.sp = 0

    def push(self, v): self.s[self.sp] = v; self.sp = self.sp + 1
    def pop(self): v = self.s[self.sp - 1]; self.sp = self.sp - 1; return v
    def pick(self, a): return self.s[self.sp - a - 1]
    def place(self, a, v): self.s[self.sp - a - 1] = v

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

    def allot(self, v): t = self.mp; self.mp = self.mp + v; return t

    # -- Inner interpreter --

    def init_inner(self): 
        self.ip = -1
        self.primitives = []

    def do_prim(self, q): self.primitives[-1 - q](self)
    def call(self, q): self.rpush(self.ip); self.ip = q
    def execute(self, q): 
        if q > 0:
            self.call(q) 
        else:
            self.do_prim(q)
    def exit(self): self.ip = -1 if self.rp == 0 else self.rpop()

    def opcode(self): o = self.m[self.ip]; self.ip = self.ip + 1; return o

    def inner(self): 
        t = self.rp
        while t <= self.rp and self.ip >= 0:
            self.trace()
            self.execute(self.opcode())

    def eval(self, q): self.execute(q); self.inner()

    def do(self, l): 
        for i in l:
            self.eval(i)

    def noname(self, l): self.primitives.append(l); return 0 - len(self.primitives)

    # -- Exceptions --

    def throw(self, v): 
        if not v == 0:
            raise RuntimeError('', v)

    def catch(self, q):
        tsp = self.sp
        trp = self.rp
        try:
            self.eval(q)
            self.push(0)
        except RuntimeError as e:
            self.sp = tsp
            self.rp = trp
            self.push(e.args[1])

    # -------------------------------------------------------------------------
    # -- Basic VM instructions ------------------------------------------------
    # -------------------------------------------------------------------------

    # -- Stack shufflers --

    def drop(self): self.pop()
    def dup(self): self.push(self.pick(0))
    def swap(self): a = self.pop(); b = self.pop(); self.push(a); self.push(b)
    def over(self): self.push(self.pick(1))

    # -- Arithmetic --

    def add(self): a = self.pop(); self.push(self.pop() + a)
    def sub(self): a = self.pop(); self.push(self.pop() - a)
    def mul(self): a = self.pop(); self.push(self.pop() * a)
    def div(self): a = self.pop(); self.push(self.pop() / a)
    def mod(self): a = self.pop(); self.push(self.pop() % a)

    # -- Bit --

    def _and(self): a = self.pop(); self.push(self.pop() & a)
    def _or(self): a = self.pop(); self.push(self.pop() | a)
    def _xor(self): a = self.pop(); self.push(self.pop() ^ a)
    def _invert(self): self.place(0, ~self.pick(0))

    def lshift(self): a = self.pop(); self.push(self.pop() << a)
    # def lrshift(self): python does not have logical right shift, do I need it?
    def rshift(self): a = self.pop(); self.push(self.pop() >> a)

    # -- Comparisons --

    def gt(self): a = self.pop(); b = self.pop(); self.push(-1 if b > a else 0)
    def eq(self): a = self.pop(); b = self.pop(); self.push(-1 if b == a else 0)
    def lt(self): a = self.pop(); b = self.pop(); self.push(-1 if b < a else 0)

    def zgt(self): self.push(-1 if self.pop() > 0 else 0)
    def zeq(self): self.push(-1 if self.pop() == 0 else 0)
    def zlt(self): self.push(-1 if self.pop() < 0 else 0)

    # -- Here starts the meaty part of Sloth !!! ------------------------------

    # -------------------------------------------------------------------------
    # -- Symbols --------------------------------------------------------------
    # -------------------------------------------------------------------------

    # # I have all this things here that come from Forth (hidden, colon, immediate)
    # # and all fields link, xt, dt, wl, flags, ....
    # # But I think that I don't need xt/dt, only xt:
    # # A var is defined as 0 $ my-var
    # # And its value is taken as ^ my-var
    # # To modify a var, 5 $ my-var
    # # To create a new variable use $$ my-var
    # # To create a word  : word ... ;
    # # To repeat a word if it does not exist :: word .... ;

    # HIDDEN = 1
    # COLON = 2
    # IMMEDIATE = 4

    # def init_symbols(self):
    #     self.latest = 0

    # def symsize(self, name):
    #     return Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CHAR + Sloth.CHAR + len(name)*Sloth.CHAR

    # def link(self, w): return w
    # def xt(self, w): return w + Sloth.CELL
    # def dt(self, w): return w + Sloth.CELL + Sloth.CELL
    # def wl(self, w): return w + Sloth.CELL + Sloth.CELL + Sloth.CELL
    # def flags(self, w): return w + Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CELL
    # def namelen(self, w): return w + Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CHAR
    # def name(self, w): return w + Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CELL + Sloth.CHAR + Sloth.CHAR

    # # Basic implementation with symbols inserted between the heap, but it
    # # can be modified to put it at the top of the heap, making latest
    # # effectively the non-writeable part of the heap
    # def symbol(self, name):
    #     w = self.allot(self.symsize(name))
    #     self.store(self.link(w), self.latest)
    #     self.latest = w
    #     self.store(self.xt(w), 0)
    #     self.store(self.dt(w), self.mp)
    #     self.store(self.wl(w), 0)
    #     self.cstore(self.flags(w), 0)
    #     self.cstore(self.namelen(w), len(name))
    #     for i in range(len(name)):
    #         self.cstore(self.name(w) + i*Sloth.CHAR, ord(name[i]))

    # def set_flag(self, w, f): self.cstore(self.flags(w), self.cfetch(self.flags(w)) | f)
    # def unset_flag(self, w, f): self.cstore(self.flags(w), self.cfetch(self.flags(w)) & ~f)

    # def has_flag(self, w, f): return self.cfetch(self.flags(w)) & f

    # def colon(self, name, xt):
    #     self.symbol(name)
    #     self.xt(self.latest, xt)
    #     self.set_flag(self.latest, COLON) 

    # # TODO find !!

    # # To be able to use it like FORSP (and that seems interesting), there
    # # will not be does> (it can be implemented later, but not in symbols
    # # directly and not allowing several does> -i think that makes no sense-)
    # # quote symbol -> \ symbol
    # # push symbol binding -> ' symbol (tick does exactly that, return the xt)
    # # pop and bound to symbol -> $ symbol

    # # quote symbol -> ' symbol
    # # push symbol binding (xt) -> $ symbol
    # # bound symbol to pop -> | symbol

    def init_symbols(self):
        self.latest = 0

    def link(self, w): return w
    def binding(self, w): return w + Sloth.CELL
    def namelen(self, w): return w + Sloth.CELL + Sloth.CELL
    def name(self, w): return w + Sloth.CELL + Sloth.CELL + Sloth.CHAR

    def sym_size(self, u):
        return Sloth.CELL + Sloth.CELL + Sloth.CHAR + u*Sloth.CHAR

    def create_symbol(self, a, u):
        w = self.allot(self.sym_size(u))
        self.store(self.link(w), self.latest)
        self.latest = w
        self.store(self.binding(w), 0)
        self.cstore(self.namelen(w), u)
        for i in range(u):
            self.cstore(self.name(w) + i*Sloth.CHAR, self.cfetch(a + i*Sloth.CHAR))
        return w

    def compare(self, a, u, w):
        if not u == self.cfetch(self.namelen(w)):
            return False
        for i in range(u):
            c1 = self.cfetch(a + i*Sloth.CHAR)
            c2 = self.cfetch(self.name(w) + i*Sloth.CHAR)
            c1 = (c1 - 32) if 97 <= c1 <= 122 else c1
            c2 = (c2 - 32) if 97 <= c2 <= 122 else c2
            if not c1 == c2:
                return False
        return True

    def find_symbol(self, a, u):
        w = self.latest
        while not w == 0:
            if self.compare(a, u, w):
                break
            w = self.fetch(self.link(w))
        return w

    # Syntax:

    # The double char words are there to ensure an hyperstatic environment
    # when its required, like in lexical scope for locals

    # Quote symbol -> ' symbol
    # Create symbol -> '' symbol ( ' symbol <> '' symbol )
    # Pop and bound to symbol -> $ symbol
    # Create symbol and bound to it -> $$ symbol
    # Push symbol binding -> ^ symbol

    # -------------------------------------------------------------------------
    # -- Outer interpreter (for bootstrapping) --------------------------------
    # -------------------------------------------------------------------------

    def init_outer(self):
        self.ibuf = 0
        self.ipos = 0
        self.ilen = 0

        self.tok = 0
        self.tlen = 0

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
            self.trace()
            u = self.pop(); a = self.pop()
            if u == 0:
                break
            w = self.find_symbol(a, u)
            if not w == 0:
                self.eval(self.fetch(self.binding(w)))
            else:
                self.push(self.tok); self.push(self.tlen)
                self.type()
                print('?')
            self.trace()

    # # -------------------------------------------------------------------------

    # def compile(self, v): self.m[self.mp] = v; self.mp = self.mp + 1

    #     self.latest = 0

    #     self.ibuf = 0
    #     self.ipos = 0
    #     self.ilen = 0

    #     self.tok = 0
    #     self.tlen = 0

    #     self.primitives = []

    #     self.symbols = []

    #     self.EXIT = self.colon('EXIT', lambda vm: vm.exit())

    #     self.HELLO = self.colon('HELLO', lambda vm: print('Hello world!'))

    #     self.BYE = self.colon('BYE', lambda vm: sys.exit(0))

    #     self.colon('#', lambda vm: vm.intnum())

    #     self.colon('$', lambda vm: vm.tick())
    #     self.colon('\'', lambda vm: vm.do((Sloth.tick, Sloth.binding)))
    #     self.colon('BINDING', lambda vm: vm.push(vm.binding(vm.pop())))
    #     self.colon('NAME', lambda vm: vm.str_to_transient(vm.name(vm.pop())))

    #     self.colon('TYPE', lambda vm: vm.type())

    #     self.colon('DROP', lambda vm: vm.pop())
    #     self.colon('DUP', lambda vm: vm.push(vm.pick(0)))

    #     self.colon('+', lambda vm: vm.add())
    #     self.colon('-', lambda vm: vm.sub())
    #     self.colon('*', lambda vm: vm.mul())
    #     self.colon('/', lambda vm: vm.div())
    #     self.colon('MOD', lambda vm: vm.mod())

    #     self.colon(':', lambda vm: vm._colon())
    #     self.colon(';', lambda vm: vm._semicolon())

    # -- Parsing numbers --

    def intnum(self):
        self.token()
        try:
            u = self.pop(); a = self.pop()
            n = int(self.data_to_str(a, u))
            self.push(n)
        except ValueError as e:
            print('Error converting number', e) 

    def quote(self):
        self.token()
        u = self.pop(); a = self.pop()
        self.push(self.find_symbol(a, u))

    def set_binding(self):
        self.token()
        u = self.pop(); a = self.pop()
        w = self.find_symbol(a, u)
        if w == 0:
            w = self.create_symbol(a, u)
        self.store(self.binding(w), self.pop())

    def get_binding(self):
        self.token()
        u = self.pop(); a = self.pop()
        w = self.find_symbol(a, u)
        if w == 0:
            w = self.create_symbol(a, u)
        self.push(self.fetch(self.binding(w)))


    def type(self):
        l = self.pop(); a = self.pop()
        for i in range(l):
            print(chr(self.fetch(a + i)), sep='', end='')

    def trace(self):
        print(self.mp, ' ', self.s[:self.sp], ' ', sep='', end='')
        print(': [', self.ip, '] ', sep = '', end = '')
        print(Fore.YELLOW, end='')
        if self.tlen > 0: self.push(self.tok); self.push(self.tlen); self.type()
        print(Style.RESET_ALL, end='')
        if self.ipos < self.ilen: self.push(self.ibuf + self.ipos); self.push(self.ilen); self.type()
        print()

    def str_to_data(self, s, a):
        for i in range(len(s)):
            self.m[a + i*Sloth.CHAR] = ord(s[i])
        self.ibuf = 0
        self.ipos = 0
        self.ilen = len(s)
        self.tok = 0
        self.tlen = 0

    def data_to_str(self, a, u):
        s = ''
        for i in range(u):
            s = s + chr(self.cfetch(a + i*Sloth.CHAR))
        return s

    def pad(self): return self.mp + 1024

    def colon(self, n, l):
        self.str_to_data(n, self.pad())
        w = self.create_symbol(self.pad(), len(n))
        self.store(self.binding(w), self.noname(l))

    def repl(self):
        while True:
            self.str_to_data(input('>>> '), 0)
            self.outer()

    def bootstrap(self):
        self.colon('BYE', lambda vm: exit())

        self.colon('#', lambda vm: vm.intnum())

        self.colon('\'', lambda vm: vm.quote())
        self.colon('$', lambda vm: vm.set_binding())
        self.colon('|', lambda vm: vm.get_binding())

        self.colon('DROP', lambda vm: vm.drop())
        self.colon('DUP', lambda vm: vm.dup())
        self.colon('SWAP', lambda vm: vm.swap())

        self.colon('@', lambda vm: vm.push(vm.fetch(vm.pop())))
        self.colon('!', lambda vm: vm.store(vm.pop(), vm.pop()))

        self.colon('+', lambda vm: vm.add())

        self.colon('=', lambda vm: vm.eq())

def main():
    x = Sloth()

    # Print all symbols
    # w = x.latest
    # while not w == 0:
    #     x.push(x.name(w)); x.push(x.namelen(w))
    #     x.type()
    #     print()
    #     w = x.fetch(x.link(w))

    # x.catch(x.noname(lambda vm: vm.throw(-15)))
    # x.trace()
    x.repl()

if __name__ == '__main__':
    sys.exit(main())
