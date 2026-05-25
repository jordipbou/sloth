package io.github.jordipbou.Sloth;

public class REPL {
	public static final void main(String[] args) {
		Sloth x = new Sloth(524288, 1024);

		x.bootstrap();

		x.set_root_path("../../");
		
		x.include("4th/ans.4th");

		if (args.length == 0) {
			x.repl();
		} else if (args.length == 1 && args[0].equals("--test")) {
			x.include("forth2012-test-suite/src/runtests.fth");
		}
	}
}
