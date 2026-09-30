package io.github.jordipbou.Sloth;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;

public class REPL {
	/* Failures that cannot be checked automatically: the Core ACCEPT */
	/* test (ACCEPT is a stub in Sloth). The Java FP suite passes the */
	/* -0.4999E FROUND case that C counts, so the baseline is 1. */
	static final int FAILURES_ALLOWED = 1;

	private static int count(String text, String needle) {
		int n = 0, i = 0;
		while ((i = text.indexOf(needle, i)) != -1) { n++; i += needle.length(); }
		return n;
	}

	public static final void main(String[] args) {
		Sloth x = new Sloth(524288, 1024);

		x.bootstrap();
		x.bootstrap_file();
		Memory.bootstrap(x);

		x.set_root_path("../..");

		int ior = x.include("ans.4th");
		if (ior != 0) {
			System.out.println("Fatal error: ans.4th can not be included (IOR " + ior + ").");
			System.exit(-1);
		}

		int errors = 0;

		if (args.length == 0) {
			x.repl();
		} else if (args.length == 1 && args[0].equals("--test")) {
			/* The interactive tests (e.g. ACCEPT) read KEY, which */
			/* reads System.in. Give it an empty stream so read() */
			/* returns EOF and the suite never blocks on a terminal. */
			/* Redefining the KEY word is not enough: ACCEPT was */
			/* already compiled against the original KEY. */
			System.setIn(new ByteArrayInputStream(new byte[0]));

			/* The Forth suite has no reliable error counter for the */
			/* FP tests, so the failures are counted from the output. */
			PrintStream real = System.out;
			ByteArrayOutputStream buffer = new ByteArrayOutputStream();
			System.setOut(new PrintStream(buffer, true));

			if (x.include("../../forth2012-test-suite/src/runtests.fth") != 0) errors++;
			if (x.include("../../forth2012-test-suite/src/fp/runfptests.fth") != 0) errors++;

			System.out.flush();
			System.setOut(real);

			String text = new String(buffer.toByteArray(), StandardCharsets.ISO_8859_1);
			int failures = count(text, "INCORRECT RESULT")
					+ count(text, "WRONG NUMBER OF RESULTS");

			real.print(text);
			real.println("Java test result: " + failures
					+ " failures (baseline " + FAILURES_ALLOWED + ")");

			if (errors != 0 || failures > FAILURES_ALLOWED) {
				real.println("Test failures detected.");
				System.exit(1);
			}
		} else {
			x.include(args[0]);
		}

		System.exit(errors != 0 ? 1 : 0);
	}
}
