const FAM_R_O = 1;
const FAM_R_W = 2;
const FAM_W_O = 3;
const FAM_BIN = 16;

const IOR = -37;

function open_fam(x, name, fam, create) {
  const f = fam & 0x0f;
  if (create) {
    if (f === FAM_R_O) {
      const w = x.host.openMode(name, 'w');
      if (w) w.close();
      return x.host.openMode(name, 'r');
    }
    return x.host.openMode(name, 'w+');
  }
  if (f === FAM_W_O) return x.host.openMode(name, 'w+');
  if (!x.host.exists(name)) return null;
  if (f === FAM_R_O) return x.host.openMode(name, 'r');
  return x.host.openMode(name, 'r+');
}

function _bin_(x) {
  if (!x.check_data_stack(1, 1)) return;
  x.push(x.pop() | FAM_BIN);
}

function _r_slash_o_(x) {
  if (!x.check_data_stack(0, 1)) return;
  x.push(FAM_R_O);
}

function _r_slash_w_(x) {
  if (!x.check_data_stack(0, 1)) return;
  x.push(FAM_R_W);
}

function _w_slash_o_(x) {
  if (!x.check_data_stack(0, 1)) return;
  x.push(FAM_W_O);
}

function _create_file_(x) {
  if (!x.check_data_stack(3, 2)) return;
  const fam = x.pop();
  const u = x.pop();
  const caddr = x.pop();
  const file = open_fam(x, x.toString(caddr, u), fam, true);
  if (!file) {
    x.push(0);
    x.push(IOR);
    return;
  }
  x.push(x.putObject(file));
  x.push(0);
}

function _open_file_(x) {
  if (!x.check_data_stack(3, 2)) return;
  const fam = x.pop();
  const u = x.pop();
  const caddr = x.pop();
  const file = open_fam(x, x.toString(caddr, u), fam, false);
  if (!file) {
    x.push(0);
    x.push(IOR);
    return;
  }
  x.push(x.putObject(file));
  x.push(0);
}

function _close_file_(x) {
  if (!x.check_data_stack(1, 1)) return;
  const fid = x.pop();
  const file = x.o[fid];
  if (!file || typeof file.close !== 'function') {
    x.push(IOR);
    return;
  }
  file.close();
  x.removeObject(fid);
  x.push(0);
}

function _file_size_(x) {
  if (!x.check_data_stack(1, 3)) return;
  const fid = x.pop();
  const file = x.o[fid];
  if (!file || typeof file.size !== 'function') {
    x.dpush(0n);
    x.push(IOR);
    return;
  }
  x.dpush(BigInt(file.size()));
  x.push(0);
}

function _reposition_file_(x) {
  if (!x.check_data_stack(3, 1)) return;
  const fid = x.pop();
  const hi = x.upop();
  const lo = x.upop();
  const file = x.o[fid];
  if (!file || typeof file.seek !== 'function') {
    x.push(IOR);
    return;
  }
  file.seek(Number((BigInt(hi) << 32n) | BigInt(lo)));
  x.push(0);
}

function _flush_file_(x) {
  if (!x.check_data_stack(1, 1)) return;
  x.pop();
  x.push(0);
}

function _resize_file_(x) {
  if (!x.check_data_stack(3, 1)) return;
  const fid = x.pop();
  const hi = x.upop();
  const lo = x.upop();
  const file = x.o[fid];
  if (!file || typeof file.truncate !== 'function') {
    x.push(IOR);
    return;
  }
  file.truncate(Number((BigInt(hi) << 32n) | BigInt(lo)));
  x.push(0);
}

function _delete_file_(x) {
  if (!x.check_data_stack(2, 1)) return;
  const u = x.pop();
  const caddr = x.pop();
  x.push(x.host.remove(x.toString(caddr, u)) ? 0 : IOR);
}

function _rename_file_(x) {
  if (!x.check_data_stack(4, 1)) return;
  const u2 = x.pop();
  const caddr2 = x.pop();
  const u1 = x.pop();
  const caddr1 = x.pop();
  x.push(x.host.rename(x.toString(caddr1, u1), x.toString(caddr2, u2)) ? 0 : IOR);
}

function _file_status_(x) {
  if (!x.check_data_stack(2, 2)) return;
  const u = x.pop();
  const caddr = x.pop();
  x.push(0);
  x.push(x.host.exists(x.toString(caddr, u)) ? 0 : IOR);
}

function _read_file_(x) {
  if (!x.check_data_stack(3, 2)) return;
  const fid = x.pop();
  const u1 = x.pop();
  const caddr = x.pop();
  const file = x.o[fid];
  if (!file || typeof file.read !== 'function') {
    x.push(0);
    x.push(IOR);
    return;
  }
  const blk = x.m[caddr >> 24];
  const rel = caddr & 0xffffff;
  const n = file.read(blk, rel, Math.min(u1, blk.length - rel));
  x.push(n);
  x.push(0);
}

function _write_file_(x) {
  if (!x.check_data_stack(3, 1)) return;
  const fid = x.pop();
  const u = x.pop();
  const caddr = x.pop();
  const file = x.o[fid];
  if (!file || typeof file.write !== 'function') {
    x.push(IOR);
    return;
  }
  const blk = x.m[caddr >> 24];
  const rel = caddr & 0xffffff;
  file.write(blk.subarray(rel, rel + u));
  x.push(0);
}

function _write_line_(x) {
  if (!x.check_data_stack(3, 1)) return;
  const fid = x.pop();
  const u = x.pop();
  const caddr = x.pop();
  const file = x.o[fid];
  if (!file || typeof file.write !== 'function') {
    x.push(IOR);
    return;
  }
  const blk = x.m[caddr >> 24];
  const rel = caddr & 0xffffff;
  const bytes = new Uint8Array(u + 1);
  bytes.set(blk.subarray(rel, rel + u));
  bytes[u] = 10;
  file.write(bytes);
  x.push(0);
}

export function bootstrap(x) {
  const p = (fn) => x.primitive(fn);
  x.code('BIN', p(_bin_));
  x.code('R/O', p(_r_slash_o_));
  x.code('R/W', p(_r_slash_w_));
  x.code('W/O', p(_w_slash_o_));
  x.code('CREATE-FILE', p(_create_file_));
  x.code('OPEN-FILE', p(_open_file_));
  x.code('CLOSE-FILE', p(_close_file_));
  x.code('FILE-SIZE', p(_file_size_));
  x.code('FILE-POSITION', p((vm) => vm._file_position_()));
  x.code('REPOSITION-FILE', p(_reposition_file_));
  x.code('FLUSH-FILE', p(_flush_file_));
  x.code('RESIZE-FILE', p(_resize_file_));
  x.code('DELETE-FILE', p(_delete_file_));
  x.code('RENAME-FILE', p(_rename_file_));
  x.code('FILE-STATUS', p(_file_status_));
  x.code('READ-FILE', p(_read_file_));
  x.code('READ-LINE', p((vm) => vm._read_line_()));
  x.code('WRITE-FILE', p(_write_file_));
  x.code('WRITE-LINE', p(_write_line_));
  x.code('REQUIRED', p((vm) => vm._required_()));
  x.code('REQUIRE', p((vm) => vm._require_()));
}
