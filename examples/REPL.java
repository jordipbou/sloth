package examples;

import java.io.*;
import java.nio.ByteBuffer;
import java.util.function.Consumer;

import org.jline.terminal.Terminal;
import org.jline.terminal.TerminalBuilder;
import org.jline.utils.NonBlockingReader;
import org.jline.reader.LineReader;
import org.jline.reader.LineReaderBuilder;

import org.jordipbou.sloth.Dodo;

public class REPL {
	public static void main(String[] args) throws IOException {
		Terminal terminal = TerminalBuilder.terminal();
		terminal.enterRawMode();
		NonBlockingReader reader = terminal.reader();

		Dodo x = new Dodo(64, 64, 64, 64, 64, ByteBuffer.allocateDirect(16 * 1024 * 1024));
		x.bootstrap();
		x.primitive("EMIT", (vm) -> System.out.printf("%c", (byte)vm.pop()));
		x.v(Dodo.EMIT, x.latest().xt);
		x.primitive("KEY", (vm) -> {
			try { 
				vm.push(reader.read()); 
			} catch (IOException ex) { 
				vm.push(-1); 
			};
		});
		x.v(Dodo.KEY, x.latest().xt);

		x.primitive("PRINTLN", (vm) -> System.out.println(vm.opop()));
		x.primitive("STR>STRING", (vm) -> vm.opush(vm.data_to_str()));

		LineReader lineReader = LineReaderBuilder.builder().terminal(terminal).build();

    while (true) {
			x.evaluate(lineReader.readLine());
			x.dot_s(); 
			System.out.println(" ok.");
    }     
	}
}
