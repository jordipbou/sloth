const IOR = -59;
const IOR_RESIZE = -61;

function _allocate_(x) {
  if (!x.check_data_stack(1, 2)) return;
  const u = x.pop();
  let idx = -1;
  if (u >= 0) {
    try {
      idx = x.putByteBuffer(new Uint8Array(u));
    } catch {
      idx = -1;
    }
  }
  if (idx < 0) {
    x.push(0);
    x.push(IOR);
  } else {
    x.push(x.to_abs(0, idx));
    x.push(0);
  }
}

function _free_(x) {
  if (!x.check_data_stack(1, 1)) return;
  x.removeByteBuffer(x.pop() >> 24);
  x.push(0);
}

function _resize_(x) {
  if (!x.check_data_stack(2, 2)) return;
  const u = x.pop();
  const addr = x.pop();
  if (u < 0) {
    x.push(addr);
    x.push(IOR_RESIZE);
    return;
  }
  const old = x.m[addr >> 24];
  let nb;
  try {
    nb = new Uint8Array(u);
  } catch {
    x.push(addr);
    x.push(IOR_RESIZE);
    return;
  }
  nb.set(old.subarray(0, Math.min(old.length, u)));
  const new_idx = x.putByteBuffer(nb);
  if (new_idx < 0) {
    x.push(addr);
    x.push(IOR_RESIZE);
    return;
  }
  x.removeByteBuffer(addr >> 24);
  x.push(x.to_abs(0, new_idx));
  x.push(0);
}

function _b_fetch_(x) {
  if (!x.check_data_stack(1, 1)) return;
  x.push(x.b_fetch(x.pop()));
}

function _b_store_(x) {
  if (!x.check_data_stack(2, 0)) return;
  const a = x.pop();
  x.b_store(a, x.pop());
}

function _w_fetch_(x) {
  if (!x.check_data_stack(1, 1)) return;
  const a = x.pop();
  x.push(x.mv[a >> 24].getInt16(a & 0xffffff, true));
}

function _w_store_(x) {
  if (!x.check_data_stack(2, 0)) return;
  const a = x.pop();
  x.mv[a >> 24].setInt16(a & 0xffffff, x.pop(), true);
}

function _l_fetch_(x) {
  if (!x.check_data_stack(1, 1)) return;
  const a = x.pop();
  x.push(x.mv[a >> 24].getInt32(a & 0xffffff, true));
}

function _l_store_(x) {
  if (!x.check_data_stack(2, 0)) return;
  const a = x.pop();
  x.mv[a >> 24].setInt32(a & 0xffffff, x.pop(), true);
}

function _x_fetch_(x) {
  if (!x.check_data_stack(1, 1)) return;
  const a = x.pop();
  x.push(Number(BigInt.asIntN(32, x.mv[a >> 24].getBigInt64(a & 0xffffff, true))));
}

function _x_store_(x) {
  if (!x.check_data_stack(2, 0)) return;
  const a = x.pop();
  x.mv[a >> 24].setBigInt64(a & 0xffffff, BigInt(x.pop()), true);
}

export function bootstrap(x) {
  const p = (fn) => x.primitive(fn);
  x.code('ALLOCATE', p(_allocate_));
  x.code('FREE', p(_free_));
  x.code('RESIZE', p(_resize_));
  x.code('B@', p(_b_fetch_));
  x.code('B!', p(_b_store_));
  x.code('W@', p(_w_fetch_));
  x.code('W!', p(_w_store_));
  x.code('L@', p(_l_fetch_));
  x.code('L!', p(_l_store_));
  x.code('X@', p(_x_fetch_));
  x.code('X!', p(_x_store_));
}
