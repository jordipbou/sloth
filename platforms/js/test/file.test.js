import { test } from 'node:test';
import assert from 'node:assert';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, writeFileSync, existsSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = fileURLToPath(new URL('..', import.meta.url));

function run(script) {
  const dir = mkdtempSync(join(tmpdir(), 'slothjs-'));
  const file = join(dir, 'script.4th');
  writeFileSync(file, script.join('\n') + '\n');
  return execFileSync(process.execPath, ['src/repl.js', file], {
    cwd: HERE,
    encoding: 'utf8',
  });
}

test('file word set create/write-line/read-line/close/delete', () => {
  const dir = mkdtempSync(join(tmpdir(), 'slothjs-'));
  const data = join(dir, 'f.txt');
  const out = run([
    'CREATE BUF 256 ALLOT',
    `S" ${data}" W/O CREATE-FILE THROW`,
    'DUP S" hello world" ROT WRITE-LINE THROW',
    'CLOSE-FILE THROW',
    `S" ${data}" R/O OPEN-FILE THROW`,
    'DUP BUF 256 ROT READ-LINE THROW',
    'ROT CLOSE-FILE THROW',
    'DROP BUF SWAP TYPE CR',
    `S" ${data}" DELETE-FILE THROW`,
  ]);
  assert.equal(out, 'hello world\n');
  assert.equal(existsSync(data), false);
});

test('file word set write-file/read-file/file-size/reposition/rename/status', () => {
  const dir = mkdtempSync(join(tmpdir(), 'slothjs-'));
  const a = join(dir, 'a.txt');
  const b = join(dir, 'b.txt');
  const out = run([
    'CREATE BUF 64 ALLOT',
    `S" ${a}" W/O CREATE-FILE THROW`,
    'DUP S" abcdef" ROT WRITE-FILE THROW',
    'DUP FILE-SIZE THROW D.',
    'DUP 0 0 ROT REPOSITION-FILE THROW',
    'DUP BUF 64 ROT READ-FILE THROW SWAP CLOSE-FILE THROW',
    'BUF SWAP TYPE CR',
    `S" ${a}" S" ${b}" RENAME-FILE THROW`,
    `S" ${b}" FILE-STATUS THROW DROP`,
    `S" ${b}" DELETE-FILE THROW`,
  ]);
  assert.equal(out, '6 abcdef\n');
  assert.equal(existsSync(a), false);
  assert.equal(existsSync(b), false);
});
