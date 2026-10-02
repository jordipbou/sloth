import { test } from 'node:test';
import assert from 'node:assert';
import { Sloth } from '../src/sloth.js';
import { nodeHost } from '../src/host.js';
import * as File from '../src/file.js';
import * as Memory from '../src/memory.js';
import * as Float from '../src/float.js';

function newVm() {
  const x = new Sloth(524288, 1024, 1024, nodeHost);
  x.bootstrap();
  File.bootstrap(x);
  Memory.bootstrap(x);
  Float.bootstrap(x);
  return x;
}

test('REPRESENT rounds to u significant digits', () => {
  const x = newVm();
  x.evaluate('CREATE FBUF 20 ALLOT');
  x.evaluate('FBUF');
  const addr = x.pop();
  const rep = (expr, u) => {
    x.evaluate(expr + ' FBUF ' + u + ' REPRESENT');
    const flag2 = x.pop();
    const flag1 = x.pop();
    const n = x.pop();
    let s = '';
    for (let i = 0; i < u; i++) s += String.fromCharCode(x.c_fetch(addr + i));
    return { n, flag1, flag2, s };
  };
  assert.deepEqual(rep('1E', 5), { n: 1, flag1: 0, flag2: -1, s: '10000' });
  assert.deepEqual(rep('-1E', 5), { n: 1, flag1: -1, flag2: -1, s: '10000' });
  assert.deepEqual(rep('100E 3E F/', 5), { n: 2, flag1: 0, flag2: -1, s: '33333' });
  assert.deepEqual(rep('0.02E 3E F/', 5), { n: -2, flag1: 0, flag2: -1, s: '66667' });
  assert.deepEqual(rep('0E', 5), { n: 0, flag1: 0, flag2: -1, s: '00000' });
});

test('FROUND rounds half to even, keeps negative zero', () => {
  const x = newVm();
  x.evaluate('-0.4999E FROUND');
  const r = x.f_pop();
  assert.equal(r, 0);
  assert.equal(1 / r, -Infinity);
  x.evaluate('0.5E FROUND');
  assert.equal(x.f_pop(), 0);
  x.evaluate('2.5E FROUND');
  assert.equal(x.f_pop(), 2);
  x.evaluate('1.5E FROUND');
  assert.equal(x.f_pop(), 2);
});

test('F~ exact form distinguishes NaN sign', () => {
  const x = newVm();
  x.evaluate('1E 1E 0E F~');
  assert.equal(x.pop(), -1);
  x.evaluate('1E 2E 0E F~');
  assert.equal(x.pop(), 0);
  x.evaluate('0E 0E F/ 0E 0E F/ 0E F~');
  assert.equal(x.pop(), -1);
  x.evaluate('0E 0E F/ 0E 0E F/ FNEGATE 0E F~');
  assert.equal(x.pop(), 0);
});
