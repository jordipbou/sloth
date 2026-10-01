import { test } from 'node:test';
import assert from 'node:assert';
import { Sloth } from '../src/sloth.js';
import { nodeHost } from '../src/host.js';
import * as Memory from '../src/memory.js';

function newVm() {
  const x = new Sloth(65536, 1024, 1024, nodeHost);
  x.bootstrap();
  Memory.bootstrap(x);
  return x;
}

test('memory word set', () => {
  const x = newVm();
  x.evaluate('16 ALLOCATE DROP 5 OVER ! @');
  assert.equal(x.pop(), 5);
  x.evaluate('16 ALLOCATE DROP 200 OVER B! B@');
  assert.equal(x.pop(), 200);
  x.evaluate('16 ALLOCATE DROP 300 OVER W! W@');
  assert.equal(x.pop(), 300);
  x.evaluate('16 ALLOCATE DROP 70000 OVER L! L@');
  assert.equal(x.pop(), 70000);
  x.evaluate('16 ALLOCATE DROP 123456789 OVER X! X@');
  assert.equal(x.pop(), 123456789);
  x.evaluate('16 ALLOCATE THROW FREE');
  assert.equal(x.pop(), 0);
  x.evaluate('16 ALLOCATE DROP 32 RESIZE');
  assert.equal(x.pop(), 0);
});
