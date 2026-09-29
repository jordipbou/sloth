package io.github.jordipbou.Sloth;

public class REPL {
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

		if (args.length == 0) {
			x.repl();
		} else if (args.length == 1 && args[0].equals("--test")) {
			x.include("../../forth2012-test-suite/src/runtests.fth");
			x.include("../../forth2012-test-suite/src/fp/runfptests.fth");
		} else {
			x.include(args[0]);
		}
	}
}
