import { test } from 'node:test';
import assert from 'node:assert';
import { fileURLToPath } from 'node:url';
import { Sloth } from '../src/sloth.js';
import { nodeHost } from '../src/host.js';
import * as File from '../src/file.js';

const ROOT = fileURLToPath(new URL('../../../', import.meta.url));

function newVm(host = nodeHost, dsize = 524288, usize = 1024) {
  const x = new Sloth(dsize, usize, 1024, host);
  x.bootstrap();
  File.bootstrap(x);
  return x;
}

function captureHost() {
  const out = [];
  const err = [];
  const host = {
    ...nodeHost,
    write: (b) => {
      for (const v of b) out.push(v);
    },
    writeString: (s) => {
      for (const ch of Buffer.from(s)) out.push(ch);
    },
    writeError: (s) => {
      for (const ch of Buffer.from(s)) err.push(ch);
    },
  };
  return {
    host,
    text: () => Buffer.from(out).toString('latin1'),
    err: () => Buffer.from(err).toString('latin1'),
  };
}

test('kernel arithmetic', () => {
  const x = newVm();
  x.evaluate('1 2 +');
  assert.equal(x.pop(), 3);
  x.evaluate('10 3 -');
  assert.equal(x.pop(), 7);
  x.evaluate('6 7 *');
  assert.equal(x.pop(), 42);
  x.evaluate('-5');
  assert.equal(x.pop(), -5);
  x.evaluate('$ff #10 +');
  assert.equal(x.pop(), 265);
});

test('colon definitions and EXIT', () => {
  const x = newVm();
  x.evaluate(': SQ DUP * ; 9 SQ');
  assert.equal(x.pop(), 81);
  x.evaluate(': ADD3 3 + ; 4 ADD3');
  assert.equal(x.pop(), 7);
});

test('CREATE data area via RIP', () => {
  const x = newVm();
  x.evaluate('CREATE V 42 V ! V @');
  assert.equal(x.pop(), 42);
});

test('quotations', () => {
  const x = newVm();
  x.evaluate(': T [: 1 2 + ;] EXECUTE ; T');
  assert.equal(x.pop(), 3);
});

test('EMIT writes bytes', () => {
  const cap = captureHost();
  const x = newVm(cap.host);
  x.evaluate('65 EMIT 66 EMIT');
  assert.equal(cap.text(), 'AB');
});

test('loads ans.4th and runs words', () => {
  const cap = captureHost();
  const x = new Sloth(524288, 1024, 1024, cap.host);
  x.bootstrap();
  File.bootstrap(x);
  x.set_root_path(ROOT);
  const ior = x.include('ans.4th');
  assert.equal(ior, 0, 'include failed:\n' + cap.text() + cap.err());
  x.evaluate('1 2 + .');
  assert.equal(cap.text().endsWith('3 '), true, 'output was: ' + JSON.stringify(cap.text()));
  x.evaluate(': SQUARE DUP * ; 12 SQUARE .');
  assert.equal(cap.text().endsWith('144 '), true, 'output was: ' + JSON.stringify(cap.text()));
  x.evaluate(': SUM 0 SWAP 0 DO I + LOOP ; 5 SUM .');
  assert.equal(cap.text().endsWith('10 '), true, 'output was: ' + JSON.stringify(cap.text()));
  x.evaluate('VARIABLE X 7 X ! X @ .');
  assert.equal(cap.text().endsWith('7 '), true, 'output was: ' + JSON.stringify(cap.text()));
  x.evaluate(': SIGNUM DUP 0< IF DROP -1 ELSE 0= IF 0 ELSE 1 THEN THEN ; -5 SIGNUM . 0 SIGNUM . 9 SIGNUM .');
  assert.equal(cap.text().endsWith('1 0 1 '), true, 'output was: ' + JSON.stringify(cap.text()));
  x.evaluate(': CONST CREATE , DOES> @ ; 123 CONST C C .');
  assert.equal(cap.text().endsWith('123 '), true, 'output was: ' + JSON.stringify(cap.text()));
  x.evaluate('S" hi" TYPE');
  assert.equal(cap.text().endsWith('hi'), true, 'output was: ' + JSON.stringify(cap.text()));
});
