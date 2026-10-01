import readline from 'node:readline';
import { fileURLToPath } from 'node:url';
import { Sloth, STATE } from './sloth.js';
import { nodeHost } from './host.js';
import * as File from './file.js';
import * as Memory from './memory.js';
import * as Float from './float.js';

const ROOT = fileURLToPath(new URL('../../..', import.meta.url)).replace(/\/$/, '');

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

async function main() {
  const args = process.argv.slice(2);

  if (args.length === 1 && (args[0] === '--test' || args[0] === '-t')) {
    process.stderr.write('JavaScript --test is implemented in F6.\n');
    process.exit(2);
  }

  const x = makeVm();

  if (args.length === 0) {
    await repl(x);
  } else {
    const ior = x.include(args[0]);
    process.exit(ior !== 0 ? 1 : 0);
  }
}

main();
