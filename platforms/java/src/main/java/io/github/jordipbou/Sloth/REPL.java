package io.github.jordipbou.Sloth;

public class REPL {
	public static final void main(String[] args) {
		Sloth x = new Sloth(524288, 1024);

		x.bootstrap_kernel();

		x.evaluate("12 11 +");
		System.out.printf("%d", x.pop());
	}
}
