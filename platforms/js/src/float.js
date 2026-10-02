import { suCHAR, sFCELL, sSFCELL, sDFCELL, HERE, PRECISION, parse_float } from './sloth.js';

const _buf = new ArrayBuffer(8);
const _dv = new DataView(_buf);

function bits(v) {
  _dv.setFloat64(0, v, false);
  return _dv.getBigUint64(0, false);
}

function from_bits(b) {
  _dv.setBigUint64(0, b, false);
  return _dv.getFloat64(0, false);
}

function fnegate(v) {
  if (Number.isNaN(v)) return from_bits(bits(v) ^ 0x8000000000000000n);
  return -v;
}

function fabs(v) {
  if (Number.isNaN(v)) return from_bits(bits(v) & 0x7fffffffffffffffn);
  return Math.abs(v);
}

function rint(v) {
  if (!Number.isFinite(v)) return v;
  const f = Math.floor(v);
  const diff = v - f;
  let r;
  if (diff > 0.5) r = f + 1;
  else if (diff < 0.5) r = f;
  else r = f % 2 === 0 ? f : f + 1;
  if (r === 0 && v < 0) return -0;
  return r;
}

const POW5 = [];
function pow5(k) {
  if (POW5[k] === undefined) POW5[k] = 5n ** BigInt(k);
  return POW5[k];
}

function decompose(abs) {
  _dv.setFloat64(0, abs, false);
  const u64 = _dv.getBigUint64(0, false);
  const exp_bits = Number((u64 >> 52n) & 0x7ffn);
  const frac = u64 & 0xfffffffffffffn;
  if (exp_bits === 0) return [frac, -1074];
  return [frac | (1n << 52n), exp_bits - 1075];
}

function represent(r, u) {
  if (Number.isNaN(r) || !Number.isFinite(r)) {
    const marker = Number.isNaN(r) ? 'nan' : r > 0 ? '+infinity' : '-infinity';
    let digits = '';
    for (let i = 0; i < u; i++) digits += i < marker.length ? marker[i] : ' ';
    return { n: 0, flag1: 0, flag2: 0, digits };
  }
  const negative = r < 0 || (r === 0 && 1 / r === -Infinity);
  const abs = Math.abs(r);
  if (abs === 0) return { n: 0, flag1: negative ? -1 : 0, flag2: -1, digits: '0'.repeat(u) };

  const [m, e] = decompose(abs);
  let n_str;
  let exp10;
  if (e >= 0) {
    n_str = (m << BigInt(e)).toString();
    exp10 = 0;
  } else {
    const k = -e;
    n_str = (m * pow5(k)).toString();
    exp10 = -k;
  }

  const l = n_str.length;
  let n = exp10 + l;
  let digits;
  if (l <= u) {
    digits = n_str + '0'.repeat(u - l);
  } else {
    const shift = l - u;
    const N = BigInt(n_str);
    const pow = 10n ** BigInt(shift);
    let q = N / pow;
    const rem = N % pow;
    const half = pow / 2n;
    if (rem > half) q += 1n;
    else if (rem === half && q % 2n !== 0n) q += 1n;
    if (q >= 10n ** BigInt(u)) {
      q /= 10n;
      n += 1;
    }
    digits = q.toString();
  }
  while (digits.length < u) digits = '0' + digits;
  if (digits.length > u) digits = digits.slice(0, u);
  return { n, flag1: negative ? -1 : 0, flag2: -1, digits };
}

function exp_suffix(ex) {
  const sign = ex[0] === '-' ? '-' : '+';
  let d = ex.replace(/^[+-]/, '');
  if (d.length < 2) d = '0' + d;
  return sign + d;
}

function fmt_f(r, decimals) {
  return r.toFixed(decimals);
}

function fmt_e(r, frac) {
  if (Number.isNaN(r)) return 'NaN';
  if (!Number.isFinite(r)) return r > 0 ? 'Infinity' : '-Infinity';
  const s = r.toExponential(frac);
  const idx = s.indexOf('e');
  return s.slice(0, idx) + 'E' + exp_suffix(s.slice(idx + 1));
}

function _f_align_(x) {
  x.set(HERE, x.aligned(x.here(), sFCELL));
}

function _f_aligned_(x) {
  x.push(x.aligned(x.pop(), sFCELL));
}

function _f_literal_(x) {
  if (!x.check_float_stack(1, 0)) return;
  x.f_literal(x.f_pop());
}

function _s_f_aligned_(x) {
  x.push(x.aligned(x.pop(), sSFCELL));
}

function _d_f_aligned_(x) {
  x.push(x.aligned(x.pop(), sDFCELL));
}

function _floats_(x) {
  x.push(x.pop() * sFCELL);
}

function _s_floats_(x) {
  x.push(x.pop() * sSFCELL);
}

function _d_floats_(x) {
  x.push(x.pop() * sDFCELL);
}

function _float_plus_(x) {
  x.push(x.pop() + sFCELL);
}

function _f_depth_(x) {
  if (!x.check_data_stack(0, 1)) return;
  x.push(x.fp);
}

function _f_drop_(x) {
  if (!x.check_float_stack(1, 0)) return;
  x.f_pop();
}

function _f_dup_(x) {
  if (!x.check_float_stack(1, 2)) return;
  x.f_push(x.f_pick(0));
}

function _f_over_(x) {
  if (!x.check_float_stack(2, 3)) return;
  x.f_push(x.f_pick(1));
}

function _f_rot_(x) {
  if (!x.check_float_stack(3, 3)) return;
  const c = x.f_pop();
  const b = x.f_pop();
  const a = x.f_pop();
  x.f_push(b);
  x.f_push(c);
  x.f_push(a);
}

function _f_swap_(x) {
  if (!x.check_float_stack(2, 2)) return;
  const b = x.f_pop();
  const a = x.f_pop();
  x.f_push(b);
  x.f_push(a);
}

function _f_less_than_(x) {
  if (!x.check_float_stack(2, 0)) return;
  if (!x.check_data_stack(0, 1)) return;
  const b = x.f_pop();
  const a = x.f_pop();
  x.push(a < b ? -1 : 0);
}

function _f_zero_less_than_(x) {
  if (!x.check_float_stack(1, 0)) return;
  if (!x.check_data_stack(0, 1)) return;
  x.push(x.f_pop() < 0 ? -1 : 0);
}

function _f_zero_equals_(x) {
  if (!x.check_float_stack(1, 0)) return;
  if (!x.check_data_stack(0, 1)) return;
  x.push(x.f_pop() === 0 ? -1 : 0);
}

function _f_fetch_(x) {
  if (!x.check_data_stack(1, 0)) return;
  if (!x.check_float_stack(0, 1)) return;
  x.f_push(x.f_fetch(x.pop()));
}

function _f_store_(x) {
  if (!x.check_data_stack(1, 0)) return;
  if (!x.check_float_stack(1, 0)) return;
  x.f_store(x.pop(), x.f_pop());
}

function _s_f_fetch_(x) {
  if (!x.check_data_stack(1, 0)) return;
  if (!x.check_float_stack(0, 1)) return;
  x.f_push(x.s_f_fetch(x.pop()));
}

function _s_f_store_(x) {
  if (!x.check_data_stack(1, 0)) return;
  if (!x.check_float_stack(1, 0)) return;
  x.s_f_store(x.pop(), x.f_pop());
}

function _d_f_fetch_(x) {
  if (!x.check_data_stack(1, 0)) return;
  if (!x.check_float_stack(0, 1)) return;
  x.f_push(x.d_f_fetch(x.pop()));
}

function _d_f_store_(x) {
  if (!x.check_data_stack(1, 0)) return;
  if (!x.check_float_stack(1, 0)) return;
  x.d_f_store(x.pop(), x.f_pop());
}

function _d_to_f_(x) {
  if (!x.check_data_stack(2, 0)) return;
  if (!x.check_float_stack(0, 1)) return;
  x.f_push(Number(x.dpop()));
}

function _f_to_d_(x) {
  if (!x.check_float_stack(1, 0)) return;
  if (!x.check_data_stack(0, 2)) return;
  const v = x.f_pop();
  let b;
  if (Number.isNaN(v)) b = 0n;
  else if (v >= 9223372036854775808) b = 9223372036854775807n;
  else if (v < -9223372036854775808) b = -9223372036854775808n;
  else b = BigInt(Math.trunc(v));
  x.dpush(b);
}

function _f_plus_(x) {
  if (!x.check_float_stack(2, 1)) return;
  x.f_push(x.f_pop() + x.f_pop());
}

function _f_minus_(x) {
  if (!x.check_float_stack(2, 1)) return;
  const b = x.f_pop();
  x.f_push(x.f_pop() - b);
}

function _f_star_(x) {
  if (!x.check_float_stack(2, 1)) return;
  x.f_push(x.f_pop() * x.f_pop());
}

function _f_star_star_(x) {
  if (!x.check_float_stack(2, 1)) return;
  const b = x.f_pop();
  x.f_push(Math.pow(x.f_pop(), b));
}

function _f_slash_(x) {
  if (!x.check_float_stack(2, 1)) return;
  const b = x.f_pop();
  x.f_push(x.f_pop() / b);
}

function _floor_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.floor(x.f_pop()));
}

function _f_round_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(rint(x.f_pop()));
}

function _f_max_(x) {
  if (!x.check_float_stack(2, 1)) return;
  const b = x.f_pop();
  const a = x.f_pop();
  x.f_push(a > b ? a : b);
}

function _f_min_(x) {
  if (!x.check_float_stack(2, 1)) return;
  const b = x.f_pop();
  const a = x.f_pop();
  x.f_push(a < b ? a : b);
}

function _f_abs_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(fabs(x.f_pop()));
}

function _f_negate_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(fnegate(x.f_pop()));
}

function _f_proximate_(x) {
  if (!x.check_float_stack(3, 0)) return;
  if (!x.check_data_stack(0, 1)) return;
  const r3 = x.f_pop();
  const r2 = x.f_pop();
  const r1 = x.f_pop();
  if (Number.isNaN(r3)) {
    x.push(0);
  } else if (r3 > 0) {
    x.push(Math.abs(r1 - r2) < r3 ? -1 : 0);
  } else if (r3 < 0) {
    x.push(Math.abs(r1 - r2) < Math.abs(r3) * (Math.abs(r1) + Math.abs(r2)) ? -1 : 0);
  } else {
    x.push(bits(r1) === bits(r2) ? -1 : 0);
  }
}

function _f_sqrt_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.sqrt(x.f_pop()));
}

function _f_l_n_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.log(x.f_pop()));
}

function _f_exp_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.exp(x.f_pop()));
}

function _f_exp_m_one_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.exp(x.f_pop()) - 1);
}

function _f_log_ten_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.log10(x.f_pop()));
}

function _f_l_n_p_one_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.log(x.f_pop() + 1));
}

function _f_a_log_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.pow(10, x.f_pop()));
}

function _f_sine_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.sin(x.f_pop()));
}

function _f_a_sine_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.asin(x.f_pop()));
}

function _f_cos_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.cos(x.f_pop()));
}

function _f_a_cos_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.acos(x.f_pop()));
}

function _f_sine_cos_(x) {
  if (!x.check_float_stack(1, 2)) return;
  const r = x.f_pop();
  x.f_push(Math.sin(r));
  x.f_push(Math.cos(r));
}

function _f_tan_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.tan(x.f_pop()));
}

function _f_a_tan_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.atan(x.f_pop()));
}

function _f_atan2_(x) {
  if (!x.check_float_stack(2, 1)) return;
  const b = x.f_pop();
  x.f_push(Math.atan2(x.f_pop(), b));
}

function _f_sin_h_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.sinh(x.f_pop()));
}

function _f_cos_h_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.cosh(x.f_pop()));
}

function _f_tan_h_(x) {
  if (!x.check_float_stack(1, 1)) return;
  x.f_push(Math.tanh(x.f_pop()));
}

function _f_a_sine_h_(x) {
  if (!x.check_float_stack(1, 1)) return;
  const r = x.f_pop();
  if (r === 0) x.f_push(0);
  else if (r > 0) x.f_push(Math.log(r + Math.sqrt(r * r + 1)));
  else x.f_push(-Math.log(-r + Math.sqrt(r * r + 1)));
}

function _f_a_cos_h_(x) {
  if (!x.check_float_stack(1, 1)) return;
  const r = x.f_pop();
  if (r < 1) x.f_push(NaN);
  else x.f_push(Math.log(r + Math.sqrt(r * r - 1)));
}

function _to_float_(x) {
  if (!x.check_data_stack(2, 1)) return;
  if (!x.check_float_stack(0, 1)) return;
  const tlen = x.pop();
  const tok = x.pop();
  if (tlen === 0 || x.c_fetch(tok) === 32) {
    for (let i = 0; i < tlen; i++) {
      if (x.c_fetch(tok + i * suCHAR) !== 32) {
        x.push(0);
        return;
      }
    }
    x.push(-1);
    x.f_push(0);
    return;
  }
  if (x.c_fetch(tok + (tlen - 1) * suCHAR) === 32) {
    x.push(0);
    return;
  }
  let buf = '';
  let marker = 0;
  for (let i = 0; i < tlen; i++) {
    const c = x.c_fetch(tok + i * suCHAR);
    if (!(c >= 48 && c <= 57) && c !== 43 && c !== 45 && c !== 68 && c !== 100 && c !== 69 && c !== 101 && c !== 46) {
      x.push(0);
      return;
    }
    if (c === 68 || c === 100 || c === 69 || c === 101) {
      if (marker === 0) marker = 1;
      else {
        x.push(0);
        return;
      }
    }
    if (i !== 0 && (c === 43 || c === 45)) {
      const prev = x.c_fetch(tok + (i - 1) * suCHAR);
      if (prev !== 69 && prev !== 101) buf += 'E';
    }
    buf += String.fromCharCode(c);
  }
  const v = parse_float(buf);
  if (v === null) {
    x.push(0);
    return;
  }
  x.f_push(v);
  x.push(-1);
}

function _represent_(x) {
  if (!x.check_float_stack(1, 0)) return;
  if (!x.check_data_stack(2, 3)) return;
  const u = x.pop();
  const addr = x.pop();
  const r = x.f_pop();
  const { n, flag1, flag2, digits } = represent(r, u);
  for (let i = 0; i < u; i++) {
    x.c_store(addr + i * suCHAR, i < digits.length ? digits.charCodeAt(i) : 48);
  }
  x.push(n);
  x.push(flag1);
  x.push(flag2);
}

function _f_dot_(x) {
  if (!x.check_float_stack(1, 0)) return;
  const r = x.f_pop();
  const precision = x.user_get(PRECISION);
  const int_digits = r === 0 ? 1 : Math.trunc(Math.log10(Math.abs(r))) + 1;
  let out;
  if (r === Math.floor(r)) out = fmt_f(r, 0) + '. ';
  else if (Math.floor(r) === 0 || Math.floor(r) === -1) out = fmt_f(r, precision) + ' ';
  else out = fmt_f(r, precision - int_digits) + ' ';
  x.host.writeString(out);
}

function _f_s_dot_(x) {
  if (!x.check_float_stack(1, 0)) return;
  const precision = x.user_get(PRECISION);
  x.host.writeString(fmt_e(x.f_pop(), precision - 1) + ' ');
}

function _f_e_dot_(x) {
  if (!x.check_float_stack(1, 0)) return;
  const r = x.f_pop();
  if (Number.isNaN(r)) {
    x.host.writeString('NaN ');
    return;
  }
  if (!Number.isFinite(r)) {
    x.host.writeString(r > 0 ? 'Inf ' : '-Inf ');
    return;
  }
  if (r === 0) {
    x.host.writeString('0.0E+00 ');
    return;
  }
  const exp = Math.floor(Math.log10(Math.abs(r)) / 3) * 3;
  const scaled = r / Math.pow(10, exp);
  x.host.writeString(fmt_f(scaled, 3) + 'E' + exp_suffix(String(exp)) + ' ');
}

function _f_dot_s_(x) {
  let out = 'F:<' + x.fp + '> ';
  for (let i = 0; i < x.fp; i++) out += fmt_f(x.f[i], 6) + ' ';
  x.host.writeString(out);
}

export function bootstrap(x) {
  const p = (fn) => x.primitive(fn);
  x.user_variable('(PRECISION)', PRECISION, 15);

  x.code('(FLIT)', p((vm) => vm._f_lit_()));
  x.code('FALIGN', p(_f_align_));
  x.code('FALIGNED', p(_f_aligned_));
  x.code('FLITERAL', p(_f_literal_));
  x._immediate_();
  x.code('FLOATS', p(_floats_));
  x.code('FLOAT+', p(_float_plus_));

  x.code('SFALIGNED', p(_s_f_aligned_));
  x.code('DFALIGNED', p(_d_f_aligned_));

  x.code('SFLOATS', p(_s_floats_));
  x.code('DFLOATS', p(_d_floats_));

  x.code('FDEPTH', p(_f_depth_));
  x.code('FDROP', p(_f_drop_));
  x.code('FDUP', p(_f_dup_));
  x.code('FOVER', p(_f_over_));
  x.code('FROT', p(_f_rot_));
  x.code('FSWAP', p(_f_swap_));

  x.code('F<', p(_f_less_than_));
  x.code('F0<', p(_f_zero_less_than_));
  x.code('F0=', p(_f_zero_equals_));

  x.code('F@', p(_f_fetch_));
  x.code('F!', p(_f_store_));
  x.code('SF@', p(_s_f_fetch_));
  x.code('SF!', p(_s_f_store_));
  x.code('DF@', p(_d_f_fetch_));
  x.code('DF!', p(_d_f_store_));

  x.code('D>F', p(_d_to_f_));
  x.code('F>D', p(_f_to_d_));

  x.code('FABS', p(_f_abs_));
  x.code('F+', p(_f_plus_));
  x.code('F-', p(_f_minus_));
  x.code('F*', p(_f_star_));
  x.code('F**', p(_f_star_star_));
  x.code('F/', p(_f_slash_));
  x.code('FLOOR', p(_floor_));
  x.code('FMAX', p(_f_max_));
  x.code('FMIN', p(_f_min_));
  x.code('FNEGATE', p(_f_negate_));
  x.code('FROUND', p(_f_round_));
  x.code('F~', p(_f_proximate_));
  x.code('FATAN2', p(_f_atan2_));
  x.code('FSQRT', p(_f_sqrt_));
  x.code('FLN', p(_f_l_n_));
  x.code('FSIN', p(_f_sine_));
  x.code('FCOS', p(_f_cos_));
  x.code('FSINCOS', p(_f_sine_cos_));
  x.code('FTAN', p(_f_tan_));
  x.code('FASIN', p(_f_a_sine_));
  x.code('FACOS', p(_f_a_cos_));
  x.code('FATAN', p(_f_a_tan_));
  x.code('FEXP', p(_f_exp_));
  x.code('FEXPM1', p(_f_exp_m_one_));
  x.code('FLOG', p(_f_log_ten_));
  x.code('FLNP1', p(_f_l_n_p_one_));
  x.code('FALOG', p(_f_a_log_));
  x.code('FSINH', p(_f_sin_h_));
  x.code('FCOSH', p(_f_cos_h_));
  x.code('FTANH', p(_f_tan_h_));
  x.code('FASINH', p(_f_a_sine_h_));
  x.code('FACOSH', p(_f_a_cos_h_));

  x.code('>FLOAT', p(_to_float_));
  x.code('REPRESENT', p(_represent_));

  x.code('F.', p(_f_dot_));
  x.code('FS.', p(_f_s_dot_));
  x.code('FE.', p(_f_e_dot_));
  x.code('F.S', p(_f_dot_s_));

  x.has_float = true;
}
