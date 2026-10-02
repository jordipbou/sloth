import fs from 'node:fs';
import { isatty } from 'node:tty';

export function encode(s) {
  return new TextEncoder().encode(s);
}

export function decode(b) {
  return new TextDecoder().decode(b);
}

export class NodeFile {
  constructor(fd) {
    this.fd = fd;
    this.pos = 0;
    this.closed = false;
  }

  readByte() {
    const b = Buffer.allocUnsafe(1);
    const n = fs.readSync(this.fd, b, 0, 1, this.pos);
    if (n === 0) return -1;
    this.pos += 1;
    return b[0];
  }

  read(into, off, len) {
    const b = Buffer.allocUnsafe(len);
    const n = fs.readSync(this.fd, b, 0, len, this.pos);
    for (let i = 0; i < n; i++) into[off + i] = b[i];
    this.pos += n;
    return n;
  }

  write(bytes) {
    const b = Buffer.from(bytes);
    const n = fs.writeSync(this.fd, b, 0, b.length, this.pos);
    this.pos += n;
    return n;
  }

  seek(p) {
    this.pos = Number(p);
  }

  position() {
    return this.pos;
  }

  size() {
    return fs.fstatSync(this.fd).size;
  }

  truncate(n) {
    fs.ftruncateSync(this.fd, Number(n));
  }

  close() {
    if (!this.closed) {
      fs.closeSync(this.fd);
      this.closed = true;
    }
  }
}

export const nodeHost = {
  encode,
  decode,
  cwd: () => process.cwd(),
  os: () => process.platform,
  isTTY: () => isatty(0),
  write(bytes) {
    process.stdout.write(Buffer.from(bytes));
  },
  writeString(s) {
    process.stdout.write(s);
  },
  writeError(s) {
    process.stderr.write(s);
  },
  readByte() {
    const b = Buffer.allocUnsafe(1);
    try {
      const n = fs.readSync(0, b, 0, 1, null);
      return n === 0 ? -1 : b[0];
    } catch {
      return -1;
    }
  },
  openRead(path) {
    try {
      return new NodeFile(fs.openSync(path, 'r'));
    } catch {
      return null;
    }
  },
  openMode(path, mode) {
    try {
      return new NodeFile(fs.openSync(path, mode));
    } catch {
      return null;
    }
  },
  openReadWrite(path, create) {
    try {
      return new NodeFile(fs.openSync(path, create ? 'w+' : 'r+'));
    } catch {
      return null;
    }
  },
  openWrite(path) {
    try {
      return new NodeFile(fs.openSync(path, 'w'));
    } catch {
      return null;
    }
  },
  exists(path) {
    try {
      fs.accessSync(path);
      return true;
    } catch {
      return false;
    }
  },
  remove(path) {
    try {
      fs.unlinkSync(path);
      return true;
    } catch {
      return false;
    }
  },
  rename(a, b) {
    try {
      fs.renameSync(a, b);
      return true;
    } catch {
      return false;
    }
  },
  exit(code) {
    process.exit(code);
  },
};
