import { test } from 'node:test';
import assert from 'node:assert';
import { execFileSync } from 'node:child_process';
import { writeFileSync, mkdtempSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = fileURLToPath(new URL('..', import.meta.url));

test('repl.js runs a script file', () => {
  const dir = mkdtempSync(join(tmpdir(), 'slothjs-'));
  const file = join(dir, 'script.4th');
  writeFileSync(file, ': SQ DUP * ; 12 SQ . CR\n');
  const out = execFileSync(process.execPath, ['src/repl.js', file], {
    cwd: HERE,
    encoding: 'utf8',
  });
  assert.equal(out, '144 \n');
});

test('repl.js --test passes the Forth 2012 suite at baseline 0', () => {
  const out = execFileSync(process.execPath, ['src/repl.js', '--test'], {
    cwd: HERE,
    encoding: 'utf8',
  });
  assert.match(out, /Forth tests completed/);
  assert.match(out, /FP tests finished/);
  assert.match(out, /SLOTH-TEST-DONE/);
  assert.match(out, /JavaScript test result: 0 failures \(baseline 0\)/);
  assert.equal((out.match(/INCORRECT RESULT/g) || []).length, 0);
  assert.equal((out.match(/WRONG NUMBER OF RESULTS/g) || []).length, 0);
});
