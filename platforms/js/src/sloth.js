export const suCHAR = 1;
export const sCELL = 4;
export const CELL_BITS = 32;
export const hCELL_MASK = 0xFFFF;
export const hCELL_BITS = 16;
export const sFCELL = 8;
export const sSFCELL = 4;
export const sDFCELL = 8;

export const STACK_SIZE = 64;
export const RETURN_STACK_SIZE = 64;
export const FLOAT_STACK_SIZE = 64;

export const CBUF = 64;

export const HERE = 0;
export const INTERNAL_WL = 1 * sCELL;
export const FORTH_WL = 2 * sCELL;

export const CURRENT = 0 * sCELL;
export const ORDER = 1 * sCELL;
export const LOCALS_WORDLIST = 2 * sCELL;
export const CONTEXT = 3 * sCELL;
export const BASE = 19 * sCELL;
export const STATE = 20 * sCELL;
export const IBUF = 21 * sCELL;
export const IPOS = 22 * sCELL;
export const ILEN = 23 * sCELL;
export const SOURCE_ID = 24 * sCELL;
export const SOURCE_POS = 25 * sCELL;
export const LATESTXT = 26 * sCELL;
export const INTERPRET = 27 * sCELL;
export const ROOT_PATH_LENGTH = 28 * sCELL;
export const PATH_START = 29 * sCELL;
export const PATH_END = 30 * sCELL;
export const PATHS = 31 * sCELL;
export const INCLUDED_FILES = 95 * sCELL;
export const PRECISION = 96 * sCELL;

export const HIDDEN = 1;
export const IMMEDIATE = 2;

export const STACK_OVERFLOW = -3;
export const STACK_UNDERFLOW = -4;
export const RETURN_STACK_OVERFLOW = -5;
export const RETURN_STACK_UNDERFLOW = -6;
export const DIVISION_BY_ZERO = -10;
export const UNDEFINED_WORD = -13;
export const COMPILE_ONLY_WORD = -14;
export const ZERO_LENGTH_NAME = -16;
export const PARSED_STRING_OVERFLOW = -18;
export const NAME_TOO_LONG = -19;
export const INVALID_NUMERIC_ARGUMENT = -24;
export const COMPILER_NESTING = -29;
export const FLOAT_STACK_OVERFLOW = -44;
export const FLOAT_STACK_UNDERFLOW = -45;

const defaultHost = {
  encode: (s) => new TextEncoder().encode(s),
  decode: (b) => new TextDecoder().decode(b),
  cwd: () => '.',
  os: () => 'linux',
  isTTY: () => false,
  write: () => {},
  writeString: () => {},
  writeError: () => {},
  readByte: () => -1,
  openRead: () => null,
  openMode: () => null,
  openReadWrite: () => null,
  openWrite: () => null,
  exists: () => false,
  remove: () => false,
  rename: () => false,
  exit: () => {},
};

function is_digit(c) {
  return c >= 48 && c <= 57;
}

function parse_int(str, base) {
  if (str.length === 0) return null;
  let neg = false;
  let i = 0;
  if (str[0] === '+' || str[0] === '-') {
    neg = str[0] === '-';
    i = 1;
  }
  if (i >= str.length) return null;
  let v = 0n;
  const b = BigInt(base);
  for (; i < str.length; i++) {
    const c = str.charCodeAt(i);
    let d;
    if (c >= 48 && c <= 57) d = c - 48;
    else if (c >= 65 && c <= 90) d = c - 55;
    else if (c >= 97 && c <= 122) d = c - 87;
    else return null;
    if (d >= base) return null;
    v = v * b + BigInt(d);
  }
  return neg ? -v : v;
}

export function parse_float(str) {
  let i = 0;
  let digits = 0;
  const n = str.length;
  if (i < n && (str[i] === '+' || str[i] === '-')) i++;
  while (i < n && is_digit(str.charCodeAt(i))) {
    i++;
    digits++;
  }
  if (i < n && str[i] === '.') {
    i++;
    while (i < n && is_digit(str.charCodeAt(i))) {
      i++;
      digits++;
    }
  }
  if (digits === 0) return null;
  let end = i;
  if (i < n && (str[i] === 'e' || str[i] === 'E')) {
    let j = i + 1;
    let ed = 0;
    if (j < n && (str[j] === '+' || str[j] === '-')) j++;
    while (j < n && is_digit(str.charCodeAt(j))) {
      j++;
      ed++;
    }
    if (ed > 0) end = j;
  }
  const v = Number(str.slice(0, end));
  return Number.isNaN(v) ? null : v;
}

export class SlothError extends Error {
  constructor(value) {
    super('SlothError ' + value);
    this.value = value;
  }
}

export class Sloth {
  constructor(dsize = 524288, usize = 1024, osize = 1024, host = defaultHost) {
    this.host = host;
    this.s = new Int32Array(STACK_SIZE);
    this.sp = 0;
    this.r = new Int32Array(RETURN_STACK_SIZE);
    this.rp = 0;
    this.f = new Float64Array(FLOAT_STACK_SIZE);
    this.fp = 0;
    this.ip = -1;
    this.ep = 0;
    this.p = [];
    this.o = new Array(osize).fill(null);
    this.m = new Array(256).fill(null);
    this.mv = new Array(256).fill(null);
    this.ts_pos = 0;
    this.selfObject = 0;

    this.os = host.os ? host.os() : 'linux';
    const win = this.os.indexOf('win') !== -1;
    this.KEY_BACKSPACE = win ? 8 : 127;
    this.KEY_ENTER = win ? 13 : 10;
    this.non_tty = !(host.isTTY && host.isTTY());
    this.has_float = false;

    this.d = this.putByteBuffer(new Uint8Array(dsize));
    this.u = this.putByteBuffer(new Uint8Array(usize));
    this.ts = this.putByteBuffer(new Uint8Array(2048));

    this.store(HERE, 3 * sCELL);
    this.store(INTERNAL_WL, 0);
    this.store(FORTH_WL, 0);

    this.user_set(CURRENT, this.to_abs(FORTH_WL));
    this.user_set(ORDER, 2);
    this.user_set(LOCALS_WORDLIST, 0);
    this.user_set(CONTEXT, this.to_abs(FORTH_WL));
    this.user_set(CONTEXT + sCELL, this.to_abs(INTERNAL_WL));
  }

  check_data_stack(n, r) {
    if (this.sp < n) {
      this._throw(STACK_UNDERFLOW);
      return false;
    }
    if (this.sp - n + r > STACK_SIZE) {
      this._throw(STACK_OVERFLOW);
      return false;
    }
    return true;
  }

  push(v) {
    this.s[this.sp++] = v;
  }

  pop() {
    return this.s[--this.sp];
  }

  pick(a) {
    return this.s[this.sp - a - 1];
  }

  lpop() {
    return this.s[--this.sp];
  }

  upop() {
    return this.s[--this.sp] >>> 0;
  }

  udpop() {
    const hi = this.upop();
    const lo = this.upop();
    return BigInt.asUintN(64, (BigInt(hi) << 32n) | BigInt(lo));
  }

  dpush(v) {
    const b = BigInt(v);
    this.push(Number(BigInt.asIntN(32, b & 0xFFFFFFFFn)));
    this.push(Number(BigInt.asIntN(32, (b >> 32n) & 0xFFFFFFFFn)));
  }

  dpop() {
    const hi = this.pop();
    const lo = this.pop();
    return BigInt.asIntN(64, (BigInt(hi) << 32n) | BigInt(lo >>> 0));
  }

  rpush(v) {
    this.r[this.rp++] = v;
  }

  rpop() {
    return this.r[--this.rp];
  }

  rpick(a) {
    return this.r[this.rp - a - 1];
  }

  check_float_stack(n, r) {
    if (this.fp < n) {
      this._throw(FLOAT_STACK_UNDERFLOW);
      return false;
    }
    if (this.fp - n + r > FLOAT_STACK_SIZE) {
      this._throw(FLOAT_STACK_OVERFLOW);
      return false;
    }
    return true;
  }

  f_push(v) {
    this.f[this.fp++] = v;
  }

  f_pop() {
    return this.f[--this.fp];
  }

  f_pick(a) {
    return this.f[this.fp - a - 1];
  }

  to_abs(a, b) {
    const blk = b === undefined ? this.d : b;
    return ((blk << 24) + a) | 0;
  }

  to_rel(a) {
    return a & 0x00ffffff;
  }

  block(a) {
    return a >> 24;
  }

  b_store(a, v) {
    this.m[a >> 24][a & 0x00ffffff] = v & 0xff;
  }

  b_fetch(a) {
    return this.m[a >> 24][a & 0x00ffffff];
  }

  c_store(a, v) {
    this.m[a >> 24][a & 0x00ffffff] = v & 0xff;
  }

  c_fetch(a) {
    return this.m[a >> 24][a & 0x00ffffff];
  }

  store(a, v) {
    this.mv[a >> 24].setInt32(a & 0x00ffffff, v | 0, true);
  }

  fetch(a) {
    return this.mv[a >> 24].getInt32(a & 0x00ffffff, true);
  }

  f_store(a, v) {
    this.mv[a >> 24].setFloat64(a & 0x00ffffff, v, true);
  }

  f_fetch(a) {
    return this.mv[a >> 24].getFloat64(a & 0x00ffffff, true);
  }

  s_f_store(a, v) {
    this.mv[a >> 24].setFloat32(a & 0x00ffffff, v, true);
  }

  s_f_fetch(a) {
    return this.mv[a >> 24].getFloat32(a & 0x00ffffff, true);
  }

  d_f_store(a, v) {
    this.mv[a >> 24].setFloat64(a & 0x00ffffff, v, true);
  }

  d_f_fetch(a) {
    return this.mv[a >> 24].getFloat64(a & 0x00ffffff, true);
  }

  fromString(s) {
    const bytes = this.host.encode(s);
    if (this.ts_pos + bytes.length > this.m[this.ts].length) this.ts_pos = 0;
    const addr = this.to_abs(this.ts_pos, this.ts);
    this.m[this.ts].set(bytes, this.ts_pos);
    this.ts_pos += bytes.length;
    return addr;
  }

  toString(a, l) {
    const rel = a & 0x00ffffff;
    return this.host.decode(this.m[a >> 24].subarray(rel, rel + l));
  }

  write_path(addr, s) {
    const bytes = this.host.encode(s);
    for (let i = 0; i < bytes.length; i++) this.c_store(addr + i * suCHAR, bytes[i]);
  }

  putByteBuffer(buf) {
    for (let i = 0; i < 256; i++) {
      if (this.m[i] === null) {
        this.m[i] = buf;
        this.mv[i] = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
        return i;
      }
    }
    return -1;
  }

  removeByteBuffer(i) {
    this.m[i] = null;
    this.mv[i] = null;
  }

  putObject(obj) {
    for (let i = 1; i < this.o.length; i++) {
      if (this.o[i] === null) {
        this.o[i] = obj;
        return i;
      }
    }
    return -1;
  }

  removeObject(i) {
    this.o[i] = null;
  }

  op() {
    const o = this.fetch(this.ip);
    this.ip += sCELL;
    return o;
  }

  f_op() {
    const n = this.f_fetch(this.ip);
    this.ip += sFCELL;
    return n;
  }

  do_prim(q) {
    this.p[-1 - q](this);
  }

  call(q) {
    if (this.ip >= 0 || this.rp > 0) this.rpush(this.ip);
    this.ip = q;
  }

  execute(q) {
    if (q < 0) this.do_prim(q);
    else this.call(q);
  }

  inner() {
    const t = this.rp;
    while (t <= this.rp && this.ip >= 0) this.execute(this.op());
  }

  eval(q) {
    this.execute(q);
    if (q > 0) this.inner();
  }

  debug(debug_xt) {
    this.push(this.ip);
    this.eval(debug_xt);
  }

  debug_inner(debug_xt) {
    const t = this.rp;
    while (t <= this.rp && this.ip >= 0) {
      this.debug(debug_xt);
      this.execute(this.op());
    }
  }

  _debug_() {
    if (!this.check_data_stack(4, 0)) return;
    const post_xt = this.pop();
    const inner_xt = this.pop();
    const pre_xt = this.pop();
    const q = this.pop();
    this.debug(pre_xt);
    this.execute(q);
    if (q > 0) this.debug_inner(inner_xt);
    this.debug(post_xt);
  }

  _catch(q) {
    const tsp = this.sp;
    const trp = this.rp;
    const tip = this.ip;
    this.ep = this.ep + 1;
    try {
      this.eval(q);
      this.push(0);
    } catch (x) {
      this.sp = tsp;
      this.rp = trp;
      this.ip = tip;
      if (x instanceof SlothError) this.push(x.value);
      else this.push(-1000);
    }
    this.ep = this.ep - 1;
  }

  _throw(v) {
    if (v !== 0) {
      if (this.ep === 0) {
        this.host.writeError('EXCEPTION: ' + v + '\n');
        this.host.writeError(
          'BUFFER: ' + this.toString(this.user_get(IBUF), this.user_get(ILEN)) + '\n'
        );
        this.host.writeError(
          'TOKEN: ' +
            this.toString(
              this.user_get(IBUF) + this.user_get(IPOS) * suCHAR,
              this.user_get(ILEN) - this.user_get(IPOS)
            ) +
            '\n'
        );
      }
      throw new SlothError(v);
    }
  }

  set(a, v) {
    this.store(a, v);
  }

  get(a) {
    return this.fetch(a);
  }

  user_set(rel_a, v) {
    this.mv[this.u].setInt32(rel_a & 0x00ffffff, v | 0, true);
  }

  user_get(rel_a) {
    return this.mv[this.u].getInt32(rel_a & 0x00ffffff, true);
  }

  here() {
    return this.get(HERE);
  }

  allot(v) {
    this.set(HERE, this.here() + v);
  }

  aligned(a, sz = sCELL) {
    return ((a + (sz - 1)) & ~(sz - 1)) | 0;
  }

  _align_() {
    this.set(HERE, this.aligned(this.here()));
  }

  comma(v) {
    this.store(this.here(), v);
    this.allot(sCELL);
  }

  c_comma(v) {
    this.c_store(this.here(), v);
    this.allot(suCHAR);
  }

  f_comma(v) {
    this.f_store(this.here(), v);
    this.allot(sFCELL);
  }

  compile(xt) {
    this.comma(xt);
  }

  literal(n) {
    this.comma(this.get_xt(this.find_word('(LIT)')));
    this.comma(n);
  }

  f_literal(f) {
    this.comma(this.get_xt(this.find_word('(FLIT)')));
    this.f_comma(f);
  }

  get_latest() {
    return this.fetch(this.user_get(CURRENT));
  }

  set_latest(w) {
    this.store(this.user_get(CURRENT), w);
  }

  get_link(w) {
    return this.fetch(w);
  }

  get_xt(w) {
    return this.fetch(w + sCELL);
  }

  set_xt(w, xt) {
    this.store(w + sCELL, xt);
  }

  get_flags(w) {
    return this.c_fetch(w + 2 * sCELL);
  }

  set_flags(w, v) {
    this.c_store(w + 2 * sCELL, v);
  }

  has_flag(w, v) {
    return (this.get_flags(w) & v) === v;
  }

  set_flag(w, v) {
    this.c_store(w + 2 * sCELL, this.get_flags(w) | v);
  }

  unset_flag(w, v) {
    this.c_store(w + 2 * sCELL, this.get_flags(w) & ~v);
  }

  get_namelen(w) {
    return this.c_fetch(w + 2 * sCELL + suCHAR);
  }

  get_name_addr(w) {
    return w + 2 * sCELL + 2 * suCHAR;
  }

  header(n, l) {
    if (l <= 0) {
      this._throw(ZERO_LENGTH_NAME);
      return 0;
    }
    if (l > 0xffff) {
      this._throw(NAME_TOO_LONG);
      return 0;
    }
    this._align_();
    const w = this.here();
    this.comma(this.get_latest());
    this.set_latest(w);
    this.comma(0);
    this.c_comma(0);
    this.c_comma(l);
    for (let i = 0; i < l; i++) this.c_comma(this.c_fetch(n + i * suCHAR));
    this._align_();
    this.store(w + sCELL, this.here());
    return w;
  }

  header_name(name) {
    return this.header(this.fromString(name), this.host.encode(name).length);
  }

  _exit_() {
    this.ip = this.rp > 0 ? this.rpop() : -1;
  }

  _lit_() {
    this.push(this.op());
  }

  _rip_() {
    const tip = this.ip;
    const o = this.op();
    this.push((tip + o - sCELL) | 0);
  }

  _f_lit_() {
    if (!this.check_float_stack(0, 1)) return;
    this.f_push(this.f_op());
  }

  _branch_() {
    const a = this.ip;
    const o = this.op();
    this.ip = (a + o) | 0;
  }

  _zbranch_() {
    const a = this.ip;
    if (this.pop() === 0) {
      const o = this.op();
      this.ip = (a + o) | 0;
    } else {
      this.ip = (a + sCELL) | 0;
    }
  }

  _string_() {
    const l = this.op();
    if (!this.check_data_stack(0, 2)) return;
    this.push(this.ip);
    this.push(l);
    this.ip = this.aligned(this.ip + l + 1);
  }

  _c_string_() {
    const l = this.c_fetch(this.ip);
    if (!this.check_data_stack(0, 1)) return;
    this.push(this.ip);
    this.ip = this.aligned(this.ip + l + 2);
  }

  _quotation_() {
    const d = this.op();
    this.push(this.ip);
    this.ip = (this.ip + d) | 0;
  }

  _start_quotation_() {
    const state = this.user_get(STATE);
    this.user_set(STATE, state <= 0 ? state - 1 : state + 1);
    if (this.user_get(STATE) === -1) this.push(this.here() + 2 * sCELL);
    this.push(this.user_get(LATESTXT));
    this.compile(this.get_xt(this.find_word('(QUOTATION)')));
    this.push(this.here());
    this.comma(0);
    this.user_set(LATESTXT, this.here());
  }

  _end_quotation_() {
    const s = this.user_get(STATE);
    const a = this.pop();
    this.compile(this.get_xt(this.find_word('EXIT')));
    this.store(a, (this.here() - a - sCELL) | 0);
    this.user_set(LATESTXT, this.pop());
    this.user_set(STATE, s < 0 ? s + 1 : s - 1);
  }

  _environment_() {
    if (!this.check_data_stack(1, 1)) return;
    switch (this.pop()) {
      case 0:
        this.push(64);
        break;
      case 1:
        break;
      case 2:
        break;
      case 3:
        this.push(8);
        break;
      case 4:
        this.push(0);
        break;
      case 5:
        this.push(255);
        break;
      case 6:
        break;
      case 7:
        break;
      case 8:
        break;
      case 9:
        break;
      case 10:
        this.push(RETURN_STACK_SIZE);
        break;
      case 11:
        this.push(STACK_SIZE);
        break;
      case 12:
        this.push(this.has_float ? FLOAT_STACK_SIZE : -1);
        break;
      case 100:
        this.push(this.has_float ? -1 : 0);
        break;
      case -1:
        this.push(5);
        break;
      case -2:
        this.push(this.KEY_ENTER);
        break;
      case -3:
        this.push(this.KEY_BACKSPACE);
        break;
    }
  }

  _word_() {
    if (!this.check_data_stack(1, 1)) return;
    const c = this.pop();
    const ibuf = this.user_get(IBUF);
    const ilen = this.user_get(ILEN);
    let ipos = this.user_get(IPOS);
    if (c === 32) {
      while (ipos < ilen && this.c_fetch(ibuf + ipos * suCHAR) <= c) ipos++;
    } else {
      while (ipos < ilen && this.c_fetch(ibuf + ipos * suCHAR) === c) ipos++;
    }
    const start = ibuf + ipos * suCHAR;
    if (c === 32) {
      while (ipos < ilen && this.c_fetch(ibuf + ipos * suCHAR) > c) ipos++;
    } else {
      while (ipos < ilen && this.c_fetch(ibuf + ipos * suCHAR) !== c) ipos++;
    }
    const end = ibuf + ipos * suCHAR;
    const len = (end - start) / suCHAR;
    if (len > 0xffff) {
      this._throw(PARSED_STRING_OVERFLOW);
      return;
    }
    this.c_store(this.here() + CBUF, len);
    for (let i = 0; i < len; i++) {
      this.c_store(this.here() + CBUF + suCHAR + i * suCHAR, this.c_fetch(start + i * suCHAR));
    }
    this.push(this.here() + CBUF);
    if (ipos < ilen) ipos++;
    this.user_set(IPOS, ipos);
  }

  _file_position_() {
    if (!this.check_data_stack(1, 3)) return;
    const fid = this.pop();
    const file = this.o[fid];
    if (file && typeof file.position === 'function') {
      this.dpush(BigInt(Math.trunc(file.position())));
      this.push(0);
    } else {
      this.dpush(0n);
      this.push(-37);
    }
  }

  _read_line_() {
    if (!this.check_data_stack(3, 3)) return;
    const fid = this.pop();
    const u1 = this.pop();
    const caddr = this.pop();
    const file = this.o[fid];
    if (!file || typeof file.readByte !== 'function') {
      this.push(0);
      this.push(0);
      this.push(-37);
      return;
    }
    if (u1 === 0) {
      this.push(0);
      this.push(-1);
      this.push(0);
      return;
    }
    let count = 0;
    let read_any = false;
    while (count < u1) {
      const b = file.readByte();
      if (b < 0) break;
      read_any = true;
      if (b === 10) break;
      if (b === 13) {
        const p = file.position();
        const n = file.readByte();
        if (n !== 10 && n !== -1) file.seek(p);
        break;
      }
      this.c_store(caddr + count * suCHAR, b);
      count++;
    }
    this.push(count);
    this.push(read_any ? -1 : 0);
    this.push(0);
  }

  _interpret_() {
    while (this.user_get(IPOS) < this.user_get(ILEN)) {
      this.push(32);
      this._word_();
      const cs = this.pick(0);
      const tok = cs + suCHAR;
      const tlen = this.c_fetch(cs);
      if (tlen === 0) {
        this.pop();
        break;
      }
      this._find_();
      const flag = this.pop();
      if (flag !== 0) {
        if (this.user_get(STATE) === 0 || (this.user_get(STATE) !== 0 && flag === 1)) {
          this.eval(this.pop());
        } else {
          this.compile(this.pop());
        }
      } else {
        this.pop();
        if (tlen === 3 && this.c_fetch(tok) === 39 && this.c_fetch(tok + 2 * suCHAR) === 39) {
          if (this.user_get(STATE) === 0) this.push(this.c_fetch(tok + suCHAR));
          else this.literal(this.c_fetch(tok + suCHAR));
        } else {
          let is_double = false;
          let t = tlen;
          let a = tok;
          let temp_base = this.user_get(BASE);
          if (t > 0 && this.c_fetch(a + (t - 1) * suCHAR) === 46) {
            t--;
            is_double = true;
          }
          if (t > 0 && this.c_fetch(a) === 35) {
            temp_base = 10;
            t--;
            a += suCHAR;
          } else if (t > 0 && this.c_fetch(a) === 36) {
            temp_base = 16;
            t--;
            a += suCHAR;
          } else if (t > 0 && this.c_fetch(a) === 37) {
            temp_base = 2;
            t--;
            a += suCHAR;
          }
          let s = '';
          for (let i = 0; i < t; i++) s += String.fromCharCode(this.c_fetch(a + i * suCHAR));
          const bi = parse_int(s, temp_base);
          if (bi !== null) {
            const n = Number(BigInt.asIntN(32, bi));
            if (this.user_get(STATE) === 0) {
              this.push(n);
              if (is_double) this.push(bi < 0n ? -1 : 0);
            } else {
              this.literal(n);
              if (is_double) this.literal(bi < 0n ? -1 : 0);
            }
          } else if (this.user_get(BASE) !== 10) {
            this._throw(UNDEFINED_WORD);
          } else {
            const r = parse_float(s);
            if (r === null) this._throw(UNDEFINED_WORD);
            else if (this.user_get(STATE) === 0) this.f_push(r);
            else this.f_literal(r);
          }
        }
      }
    }
  }

  _bye_() {
    this.host.writeString('\n');
    this.host.exit(0);
  }

  _unused_() {
    if (!this.check_data_stack(0, 1)) return;
    this.push(this.m[this.d].length - this.here());
  }

  _move_() {
    if (!this.check_data_stack(3, 0)) return;
    const u = this.pop();
    const addr2 = this.pop();
    const addr1 = this.pop();
    if (addr1 >= addr2) {
      for (let i = 0; i < u; i++) this.b_store(addr2 + i, this.b_fetch(addr1 + i));
    } else {
      for (let i = u - 1; i >= 0; i--) this.b_store(addr2 + i, this.b_fetch(addr1 + i));
    }
  }

  _emit_() {
    this.host.write(new Uint8Array([this.pop() & 0xff]));
  }

  _key_() {
    if (this.non_tty) {
      this.push(this.KEY_ENTER);
      return;
    }
    this.push(this.host.readByte());
  }

  _and_() {
    const v = this.pop();
    this.push(this.pop() & v);
  }

  _invert_() {
    this.push(~this.pop());
  }

  _l_shift_() {
    const n = this.pop();
    this.push(this.pop() << n);
  }

  _minus_() {
    const n = this.pop();
    this.push((this.pop() - n) | 0);
  }

  _plus_() {
    const n = this.pop();
    this.push((this.pop() + n) | 0);
  }

  _r_shift_() {
    const n = this.pop();
    this.push(this.pop() >>> n);
  }

  _star_() {
    const n = this.pop();
    this.push(Math.imul(this.pop(), n));
  }

  _two_slash_() {
    this.push(this.pop() >> 1);
  }

  _u_m_star_() {
    const b = this.upop();
    const a = this.upop();
    this.dpush(BigInt(a) * BigInt(b));
  }

  _u_m_slash_mod_() {
    const u = BigInt(this.upop());
    const d = this.udpop();
    if (u === 0n) {
      this._throw(DIVISION_BY_ZERO);
      return;
    }
    this.push(Number(BigInt.asIntN(32, d % u)));
    this.push(Number(BigInt.asIntN(32, d / u)));
  }

  _c_fetch_() {
    if (!this.check_data_stack(1, 1)) return;
    this.push(this.c_fetch(this.pop()));
  }

  _c_store_() {
    if (!this.check_data_stack(2, 0)) return;
    const a = this.pop();
    this.c_store(a, this.pop());
  }

  _fetch_() {
    if (!this.check_data_stack(1, 1)) return;
    this.push(this.fetch(this.pop()));
  }

  _store_() {
    if (!this.check_data_stack(2, 0)) return;
    const a = this.pop();
    this.store(a, this.pop());
  }

  _equals_() {
    const n = this.pop();
    this.push(this.pop() === n ? -1 : 0);
  }

  _less_than_() {
    const n = this.pop();
    this.push(this.pop() < n ? -1 : 0);
  }

  _colon_() {
    if (this.user_get(STATE) !== 0) {
      this._throw(COMPILER_NESTING);
      return;
    }
    this.push(32);
    this._word_();
    const cs = this.pick(0);
    const tok = cs + suCHAR;
    const tlen = this.c_fetch(this.pop());
    this.header(tok, tlen);
    this.user_set(LATESTXT, this.get_xt(this.get_latest()));
    this.set_flag(this.get_latest(), HIDDEN);
    this.user_set(STATE, 1);
  }

  _colon_no_name_() {
    if (this.user_get(STATE) !== 0) {
      this._throw(COMPILER_NESTING);
      return;
    }
    if (!this.check_data_stack(0, 1)) return;
    this.push(this.here());
    this.user_set(LATESTXT, this.here());
    this.user_set(STATE, 1);
  }

  _semicolon_() {
    this.compile(this.get_xt(this.find_word('EXIT')));
    this.user_set(STATE, 0);
    if (this.get_xt(this.get_latest()) === this.user_get(LATESTXT)) {
      this.unset_flag(this.get_latest(), HIDDEN);
    }
  }

  _recurse_() {
    this.compile(this.user_get(LATESTXT));
  }

  _catch_() {
    this._catch(this.pop());
  }

  _throw_() {
    const e = this.pop();
    if (e === -2 && this.ep === 0) {
      const l = this.pop();
      const a = this.pop();
      let buf = '';
      for (let i = 0; i < l; i++) buf += String.fromCharCode(this.c_fetch(a + i * suCHAR));
      this.host.writeError('Error: ' + buf + '\n');
    }
    this._throw(e);
  }

  _drop_() {
    if (!this.check_data_stack(1, 0)) return;
    this.pop();
  }

  _dup_() {
    if (!this.check_data_stack(1, 2)) return;
    this.push(this.pick(0));
  }

  _over_() {
    if (!this.check_data_stack(2, 3)) return;
    this.push(this.pick(1));
  }

  _to_r_() {
    this.rpush(this.pop());
  }

  _r_from_() {
    this.push(this.rpop());
  }

  _swap_() {
    if (!this.check_data_stack(2, 2)) return;
    const a = this.pop();
    const b = this.pop();
    this.push(a);
    this.push(b);
  }

  _allot_() {
    if (!this.check_data_stack(1, 0)) return;
    this.allot(this.pop());
  }

  _cells_() {
    if (!this.check_data_stack(1, 1)) return;
    this.push((this.pop() * sCELL) | 0);
  }

  _chars_() {
    this.push((this.pop() * suCHAR) | 0);
  }

  _compile_comma_() {
    if (!this.check_data_stack(1, 0)) return;
    this.compile(this.pop());
  }

  _create_name_() {
    if (!this.check_data_stack(2, 0)) return;
    const tlen = this.pop();
    this.header(this.pop(), tlen);
    this.compile(this.get_xt(this.find_word('(RIP)')));
    this.compile(4 * sCELL);
    this.compile(this.get_xt(this.find_word('EXIT')));
    this.compile(this.get_xt(this.find_word('EXIT')));
  }

  _create_() {
    this.push(32);
    this._word_();
    const c = this.pop();
    this.push(c + suCHAR);
    this.push(this.c_fetch(c));
    this._create_name_();
  }

  do_does(a) {
    this.store(this.get_xt(this.get_latest()) + 2 * sCELL, a);
  }

  _do_does_() {
    if (!this.check_data_stack(1, 0)) return;
    this.do_does(this.pop());
  }

  _does_() {
    this.literal(this.here() + 4 * sCELL);
    this.compile(this.get_xt(this.find_word('(DOES)')));
    this.compile(this.get_xt(this.find_word('EXIT')));
  }

  _evaluate_() {
    if (!this.check_data_stack(2, 0)) return;
    const l = this.pop();
    const a = this.pop();
    const previbuf = this.user_get(IBUF);
    const previpos = this.user_get(IPOS);
    const previlen = this.user_get(ILEN);
    const prevsourceid = this.user_get(SOURCE_ID);
    this.user_set(SOURCE_ID, -1);
    this.user_set(IBUF, a);
    this.user_set(IPOS, 0);
    this.user_set(ILEN, l);
    this._catch(this.user_get(INTERPRET));
    this.user_set(SOURCE_ID, prevsourceid);
    this.user_set(IBUF, previbuf);
    this.user_set(IPOS, previpos);
    this.user_set(ILEN, previlen);
    const e = this.pop();
    if (e !== 0) this._throw(e);
  }

  _execute_() {
    if (!this.check_data_stack(1, 0)) return;
    this.eval(this.pop());
  }

  _here_() {
    if (!this.check_data_stack(0, 1)) return;
    this.push(this.here());
  }

  _immediate_() {
    this.set_flag(this.get_latest(), IMMEDIATE);
  }

  _postpone_() {
    this.push(32);
    this._word_();
    const cs = this.pick(0);
    const tok = cs + suCHAR;
    const tlen = this.c_fetch(cs);
    if (tlen === 0) {
      this.pop();
      return;
    }
    this._find_();
    const i = this.pop();
    const xt = this.pop();
    if (i === 0) {
      return;
    } else if (i === -1) {
      this.literal(xt);
      this.compile(this.get_xt(this.find_word('COMPILE,')));
    } else {
      this.compile(xt);
    }
  }

  _source_() {
    if (!this.check_data_stack(0, 2)) return;
    this.push(this.user_get(IBUF));
    this.push(this.user_get(ILEN));
  }

  _refill_() {
    const source_id = this.user_get(SOURCE_ID);
    if (source_id === -1) {
      this.push(0);
      return;
    }
    if (source_id === 0) {
      this.push(this.user_get(IBUF));
      this.push(80);
      this.eval(this.get_xt(this.find_word('ACCEPT')));
      this.user_set(ILEN, this.pop());
      this.user_set(IPOS, 0);
      this.push(-1);
      return;
    }
    this.push(source_id);
    this._file_position_();
    this.pop();
    this.user_set(SOURCE_POS, Number(this.dpop()));
    this.push(this.user_get(IBUF));
    this.push(1024);
    this.push(source_id);
    this._read_line_();
    const ior = this.pop();
    const flag = this.pop();
    if (flag !== 0 && ior === 0) {
      this.user_set(ILEN, this.pop());
      this.user_set(IPOS, 0);
      this.push(-1);
    } else {
      this.pop();
      this.push(0);
    }
  }

  _save_input_() {
    if (!this.check_data_stack(0, 6)) return;
    this.push(this.user_get(SOURCE_POS));
    this.push(this.user_get(SOURCE_ID));
    this.push(this.user_get(IBUF));
    this.push(this.user_get(IPOS));
    this.push(this.user_get(ILEN));
    this.push(5);
  }

  _restore_input_() {
    this.pop();
    this.user_set(ILEN, this.pop());
    this.user_set(IPOS, this.pop());
    this.user_set(IBUF, this.pop());
    this.user_set(SOURCE_ID, this.pop());
    this.user_set(SOURCE_POS, this.pop());
    if (this.user_get(SOURCE_ID) > 0) {
      const sourceId = this.user_get(SOURCE_ID);
      const obj = this.o[sourceId];
      if (obj && typeof obj.seek === 'function') {
        obj.seek(this.user_get(SOURCE_POS));
        this.push(this.user_get(IBUF));
        this.push(1024);
        this.push(sourceId);
        this._read_line_();
        this.pop();
        this.pop();
        this.pop();
      }
    }
    this.push(0);
  }

  save_input_and_path() {
    this._save_input_();
    this.push(this.user_get(PATH_START));
    this.push(this.user_get(PATH_END));
    for (let i = 0; i < 8; i++) this._to_r_();
  }

  restore_input_and_path() {
    for (let i = 0; i < 8; i++) this._r_from_();
    this.user_set(PATH_END, this.pop());
    this.user_set(PATH_START, this.pop());
    this._restore_input_();
    this.pop();
  }

  open_included_file(name) {
    const nl = this.host.encode(name).length;
    let pathstart = this.user_get(PATH_START);
    let pathend = this.user_get(PATH_END);
    const path_start_addr = this.to_abs(PATHS, this.u);
    const path_end = this.to_abs(INCLUDED_FILES, this.u);
    const in_region = pathend >= path_start_addr && pathend < path_end;
    let f = null;
    let remember = false;

    if (!in_region || pathend + nl + 1 <= path_end) {
      this.write_path(pathend, name);
      f = this.host.openRead(this.toString(pathend, nl));
      if (f) {
        pathstart = pathend;
        pathend = pathend + nl;
        remember = true;
      }
    }

    if (!f) {
      f = this.host.openRead(this.toString(pathstart, pathend - pathstart) + name);
      if (f) {
        pathend = pathend + nl;
        remember = true;
      } else {
        const root_len = this.user_get(ROOT_PATH_LENGTH);
        f = this.host.openRead(this.toString(this.to_abs(PATHS, this.u), root_len) + name);
      }
    }

    if (f && remember) {
      let p = pathend;
      while (p > pathstart) {
        p -= suCHAR;
        const c = this.c_fetch(p);
        if (c === 47 || c === 92) {
          p += suCHAR;
          break;
        }
      }
      pathend = p;
      this.user_set(PATH_START, pathstart);
      this.user_set(PATH_END, pathend);
    }

    return f;
  }

  add_to_included_files_list(name) {
    const bytes = this.host.encode(name);
    const here = this.here();
    this.comma(this.user_get(INCLUDED_FILES));
    this.user_set(INCLUDED_FILES, here);
    this.comma(bytes.length);
    for (let i = 0; i < bytes.length; i++) this.c_comma(bytes[i]);
    this._align_();
  }

  _included_() {
    const l = this.pop();
    const a = this.pop();
    const name = this.toString(a, l);

    this.save_input_and_path();

    let f;
    try {
      f = this.open_included_file(name);
    } catch (e) {
      f = null;
    }

    if (!f) {
      this.restore_input_and_path();
      this._throw(-38);
      return;
    }

    let linenumber = 0;
    this.add_to_included_files_list(name);

    this.user_set(SOURCE_ID, this.putObject(f));
    const buf_idx = this.putByteBuffer(new Uint8Array(1024 * suCHAR));
    this.user_set(IBUF, this.to_abs(0, buf_idx));
    this.user_set(IPOS, 0);
    this.user_set(ILEN, 1024);

    for (;;) {
      this._refill_();
      if (this.pop() === 0) break;
      this._catch(this.user_get(INTERPRET));
      const e = this.pop();
      if (e !== 0) {
        const pathstart = this.user_get(PATH_START);
        const pathend = this.user_get(PATH_END);
        const path = this.toString(pathstart, pathend - pathstart) + name;
        this.host.writeString('File: ' + path + '\n');
        this.host.writeString(
          'Line (' + linenumber + '): ' + this.toString(this.user_get(IBUF), this.user_get(ILEN)) + '\n'
        );
        this._throw(e);
        return;
      }
      linenumber++;
    }

    f.close();
    this.removeByteBuffer(buf_idx);
    this.removeObject(this.user_get(SOURCE_ID));
    this.restore_input_and_path();
  }

  is_file_included(a1, u1) {
    let name = this.user_get(INCLUDED_FILES);
    while (name !== 0) {
      const u2 = this.fetch(name + sCELL);
      const a2 = name + 2 * sCELL;
      if (this.compare(a1, u1, a2, u2)) return true;
      name = this.fetch(name);
    }
    return false;
  }

  _required_() {
    if (!this.check_data_stack(2, 0)) return;
    const u = this.pop();
    const caddr = this.pop();
    if (!this.is_file_included(caddr, u)) {
      this.push(caddr);
      this.push(u);
      this._included_();
    }
  }

  _require_() {
    this.push(32);
    this._word_();
    const addr = this.pop();
    this.push(addr + suCHAR);
    this.push(this.c_fetch(addr));
    this._required_();
  }

  compare(a1, u1, a2, u2) {
    if (u1 !== u2) return false;
    for (let i = 0; i < u2; i++) {
      let a = this.c_fetch(a1 + i * suCHAR);
      let b = this.c_fetch(a2 + i * suCHAR);
      if (a >= 97 && a <= 122) a -= 32;
      if (b >= 97 && b <= 122) b -= 32;
      if (a !== b) return false;
    }
    return true;
  }

  search_word(n, l) {
    for (let i = -1; i < this.user_get(ORDER); i++) {
      const wl = this.user_get(CONTEXT + i * sCELL);
      if (wl !== 0) {
        let w = this.fetch(wl);
        while (w > 0) {
          if (!this.has_flag(w, HIDDEN) && this.compare(this.get_name_addr(w), this.get_namelen(w), n, l)) {
            return w;
          }
          w = this.get_link(w);
        }
      }
    }
    return 0;
  }

  _find_() {
    if (!this.check_data_stack(1, 2)) return;
    const cstring = this.pop();
    const w = this.search_word(cstring + suCHAR, this.c_fetch(cstring));
    if (w === 0) {
      this.push(cstring);
      this.push(0);
    } else if (this.has_flag(w, IMMEDIATE)) {
      this.push(this.get_xt(w));
      this.push(1);
    } else {
      this.push(this.get_xt(w));
      this.push(-1);
    }
  }

  find_word(name) {
    return this.search_word(this.fromString(name), this.host.encode(name).length);
  }

  primitive(fn) {
    this.p.push(fn);
    return 0 - this.p.length;
  }

  code(name, xt) {
    const w = this.header_name(name);
    this.set_xt(w, xt);
    return xt;
  }

  user_variable(name, d, v) {
    const w = this.header_name(name);
    this.set_xt(w, this.here());
    this.literal(this.to_abs(0, this.u) + d);
    this.compile(this.get_xt(this.find_word('EXIT')));
    this.store(this.to_abs(0, this.u) + d, v);
  }

  _empty_rs_() {
    this.rp = 0;
  }

  _self_() {
    if (!this.check_data_stack(0, 1)) return;
    if (this.selfObject === 0) this.selfObject = this.putObject(this);
    this.push(this.selfObject);
  }

  bootstrap_kernel() {
    this.code('EXIT', this.primitive((vm) => vm._exit_()));
    this.code('(LIT)', this.primitive((vm) => vm._lit_()));
    this.code('(RIP)', this.primitive((vm) => vm._rip_()));
    this.code('(BRANCH)', this.primitive((vm) => vm._branch_()));
    this.code('(?BRANCH)', this.primitive((vm) => vm._zbranch_()));

    this.user_variable('(CURRENT)', CURRENT, this.to_abs(FORTH_WL));
    this.user_variable('#ORDER', ORDER, 2);
    this.user_variable('(LOCALS-WORDLIST)', LOCALS_WORDLIST, 0);
    this.user_variable('CONTEXT', CONTEXT, this.to_abs(FORTH_WL));
    this.user_variable('BASE', BASE, 10);
    this.user_variable('STATE', STATE, 0);
    this.user_variable('(IBUF)', IBUF, 0);
    this.user_variable('>IN', IPOS, 0);
    this.user_variable('(ILEN)', ILEN, 0);
    this.user_variable('(SOURCE-ID)', SOURCE_ID, 0);
    this.user_variable('(SOURCE-POS)', SOURCE_POS, 0);
    this.user_variable('(LATESTXT)', LATESTXT, 0);
    this.user_variable('(INTERPRET)', INTERPRET, this.primitive((vm) => vm._interpret_()));

    this.user_variable('(SLOTH_ROOT_PATH_LENGTH)', ROOT_PATH_LENGTH, 0);
    this.user_variable('(SLOTH_PATH_START)', PATH_START, this.to_abs(PATHS, this.u));
    this.user_variable('(SLOTH_PATH_END)', PATH_END, this.to_abs(PATHS, this.u));
    this.user_variable('(SLOTH_PATHS)', PATHS, 0);

    this.user_variable('(INCLUDED-FILES)', INCLUDED_FILES, 0);

    this.code('DROP', this.primitive((vm) => vm._drop_()));
    this.code('DUP', this.primitive((vm) => vm._dup_()));
    this.code('OVER', this.primitive((vm) => vm._over_()));
    this.code('>R', this.primitive((vm) => vm._to_r_()));
    this.code('R>', this.primitive((vm) => vm._r_from_()));
    this.code('SWAP', this.primitive((vm) => vm._swap_()));

    this.code('C@', this.primitive((vm) => vm._c_fetch_()));
    this.code('C!', this.primitive((vm) => vm._c_store_()));
    this.code('@', this.primitive((vm) => vm._fetch_()));
    this.code('!', this.primitive((vm) => vm._store_()));

    this.code('CELLS', this.primitive((vm) => vm._cells_()));
    this.code('CHARS', this.primitive((vm) => vm._chars_()));

    this.code('HERE', this.primitive((vm) => vm._here_()));
    this.code('ALIGN', this.primitive((vm) => vm._align_()));
    this.code('ALLOT', this.primitive((vm) => vm._allot_()));
    this.code('UNUSED', this.primitive((vm) => vm._unused_()));

    this.code('CATCH', this.primitive((vm) => vm._catch_()));
    this.code('THROW', this.primitive((vm) => vm._throw_()));

    this.code('INVERT', this.primitive((vm) => vm._invert_()));
    this.code('AND', this.primitive((vm) => vm._and_()));
    this.code('LSHIFT', this.primitive((vm) => vm._l_shift_()));
    this.code('-', this.primitive((vm) => vm._minus_()));
    this.code('+', this.primitive((vm) => vm._plus_()));
    this.code('RSHIFT', this.primitive((vm) => vm._r_shift_()));
    this.code('*', this.primitive((vm) => vm._star_()));
    this.code('2/', this.primitive((vm) => vm._two_slash_()));
    this.code('UM*', this.primitive((vm) => vm._u_m_star_()));
    this.code('UM/MOD', this.primitive((vm) => vm._u_m_slash_mod_()));

    this.code('=', this.primitive((vm) => vm._equals_()));
    this.code('<', this.primitive((vm) => vm._less_than_()));

    this.code('(STRING)', this.primitive((vm) => vm._string_()));
    this.code('(CSTRING)', this.primitive((vm) => vm._c_string_()));
    this.code('MOVE', this.primitive((vm) => vm._move_()));

    this.code('EMIT', this.primitive((vm) => vm._emit_()));
    this.code('KEY', this.primitive((vm) => vm._key_()));
    this.code('SOURCE', this.primitive((vm) => vm._source_()));
    this.code('WORD', this.primitive((vm) => vm._word_()));
    this.code('REFILL', this.primitive((vm) => vm._refill_()));
    this.code('SAVE-INPUT', this.primitive((vm) => vm._save_input_()));
    this.code('RESTORE-INPUT', this.primitive((vm) => vm._restore_input_()));
    this.code('INCLUDED', this.primitive((vm) => vm._included_()));

    this.code('FIND', this.primitive((vm) => vm._find_()));

    this.code('(QUOTATION)', this.primitive((vm) => vm._quotation_()));
    this.code('[:', this.primitive((vm) => vm._start_quotation_()));
    this._immediate_();
    this.code(';]', this.primitive((vm) => vm._end_quotation_()));
    this._immediate_();

    this.code('BYE', this.primitive((vm) => vm._bye_()));

    this.code(':', this.primitive((vm) => vm._colon_()));
    this.code(':NONAME', this.primitive((vm) => vm._colon_no_name_()));
    this.code(';', this.primitive((vm) => vm._semicolon_()));
    this._immediate_();
    this.code('RECURSE', this.primitive((vm) => vm._recurse_()));
    this._immediate_();
    this.code('IMMEDIATE', this.primitive((vm) => vm._immediate_()));
    this.code('POSTPONE', this.primitive((vm) => vm._postpone_()));
    this._immediate_();

    this.code('COMPILE,', this.primitive((vm) => vm._compile_comma_()));
    this.code('CREATE-NAME', this.primitive((vm) => vm._create_name_()));
    this.code('CREATE', this.primitive((vm) => vm._create_()));
    this.code('(DOES)', this.primitive((vm) => vm._do_does_()));
    this.code('DOES>', this.primitive((vm) => vm._does_()));
    this._immediate_();

    this.code('EVALUATE', this.primitive((vm) => vm._evaluate_()));
    this.code('EXECUTE', this.primitive((vm) => vm._execute_()));
    this.code('DEBUG', this.primitive((vm) => vm._debug_()));

    this.code('(ENVIRONMENT)', this.primitive((vm) => vm._environment_()));

    this.code('(SELF)', this.primitive((vm) => vm._self_()));
    this.code('(DICT)', this.primitive((vm) => vm.push(vm.to_abs(0))));
    this.code('(EMPTY-RETURN-STACK)', this.primitive((vm) => {
      vm.rp = 0;
    }));
  }

  bootstrap() {
    this.bootstrap_kernel();
  }

  evaluate(c) {
    const bytes = this.host.encode(c);
    const buf = new Uint8Array(bytes.length || 1);
    buf.set(bytes);
    const idx = this.putByteBuffer(buf);
    this.push(this.to_abs(0, idx));
    this.push(bytes.length);
    this._evaluate_();
    this.removeByteBuffer(idx);
  }

  include(f) {
    if (this.user_get(ROOT_PATH_LENGTH) === 0) this.set_root_path('');
    this.push(this.fromString(f));
    this.push(this.host.encode(f).length);
    this._catch(this.get_xt(this.find_word('INCLUDED')));
    return this.pop();
  }

  set_root_path(path) {
    if (path.length === 0) path = this.host.cwd();
    const cwd = this.host.cwd();
    const cap = (INCLUDED_FILES - PATHS) / suCHAR;
    const paths = this.to_abs(PATHS, this.u);
    const root = path + '/4th/';
    const root_len = this.host.encode(root).length;
    if (root_len + this.host.encode(cwd).length > cap) {
      this.user_set(ROOT_PATH_LENGTH, 0);
      this.user_set(PATH_START, paths);
      this.user_set(PATH_END, paths);
      return;
    }
    this.write_path(paths, root);
    this.user_set(ROOT_PATH_LENGTH, root_len);
    const start = paths + root_len * suCHAR;
    this.write_path(start, cwd);
    this.user_set(PATH_START, start);
    this.user_set(PATH_END, start + this.host.encode(cwd).length * suCHAR);
  }
}
