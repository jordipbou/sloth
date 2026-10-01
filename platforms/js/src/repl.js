import readline from 'node:readline';
import { fileURLToPath } from 'node:url';
import { Sloth, STATE } from './sloth.js';
import { nodeHost } from './host.js';
import * as File from './file.js';
import * as Memory from './memory.js';
import * as Float from './float.js';

const ROOT = fileURLToPath(new URL('../../..', import.meta.url)).replace(/\/$/, '');

function count(text, needle) {
  let n = 0;
  let i = 0;
  while ((i = text.indexOf(needle, i)) !== -1) {
    n++;
    i += needle.length;
  }
  return n;
}

function makeVm() {
  const x = new Sloth(524288, 1024, 1024, nodeHost);
  x.bootstrap();
  File.bootstrap(x);
  Memory.bootstrap(x);
  Float.bootstrap(x);
  x.set_root_path(ROOT);
  if (x.include('ans.4th')) {
    process.stderr.write('Fatal error: ans.4th can not be included.\n');
    process.exit(-1);
  }
  return x;
}

function status(x) {
  if (x.user_get(STATE) !== 0) return ' Compiling';
  if (x.sp > 0) return ' OK ' + x.sp;
  return ' OK';
}

async function repl(x) {
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    prompt: 'ok> ',
    terminal: process.stdin.isTTY === true,
  });
  rl.prompt();
  for await (const line of rl) {
    try {
      x.evaluate(line);
      process.stdout.write(status(x) + '\n');
    } catch (e) {
      process.stdout.write('< ' + e.message + ' >\n');
    }
    rl.prompt();
  }
  process.stdout.write('\n');
}

function runTest(x) {
  const host = x.host;
  const write = host.write.bind(host);
  const writeString = host.writeString.bind(host);
  let text = '';
  host.write = (b) => {
    for (const v of b) text += String.fromCharCode(v);
    write(b);
  };
  host.writeString = (s) => {
    text += s;
    writeString(s);
  };

  let errors = 0;
  if (x.include(ROOT + '/forth2012-test-suite/src/runtests.fth') !== 0) errors++;
  if (x.include(ROOT + '/forth2012-test-suite/src/fp/runfptests.fth') !== 0) errors++;

  const failures =
    count(text, 'INCORRECT RESULT') + count(text, 'WRONG NUMBER OF RESULTS');

  if (errors === 0) host.writeString('SLOTH-TEST-DONE\n');
  host.writeString('JavaScript test result: ' + failures + ' failures (baseline 0)\n');

  process.exit(errors !== 0 || failures > 0 ? 1 : 0);
}

async function main() {
  const args = process.argv.slice(2);

  const x = makeVm();

  if (args.length === 1 && (args[0] === '--test' || args[0] === '-t')) {
    runTest(x);
  } else if (args.length === 0) {
    await repl(x);
  } else {
    const ior = x.include(args[0]);
    process.exit(ior !== 0 ? 1 : 0);
  }
}

main();
