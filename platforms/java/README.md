# Sloth in Java

Java implementation for Android, WearOS and use as a Java scripting language. It uses memory "blocks" for the [continuous address space](../README.md#memory-model).

## Requirements

- A JDK on `PATH` (or reachable through `JAVA_HOME`).
- The Gradle wrapper (`gradlew`) is included in this directory.

## Running

From `platforms/java`:

```sh
./gradlew run                       # interactive REPL
./gradlew run --args=program.4th    # run a file
./gradlew run --args=--test         # Forth 2012 suite, baseline 0
```

The file path is resolved relative to `platforms/java`. The application main class is `io.github.jordipbou.Sloth.REPL`.

## Testing

```sh
./gradlew test                  # JUnit unit tests
./gradlew run --args=--test     # Forth 2012 suite
```

The Java implementation rounds half to even, so its `--test` baseline is `0`: unlike the C implementation, it passes the `-0.4999E FROUND` case.

The repository-wide test scripts are described in the [main README](../../README.md#testing).

## Notes

- The `4th/` directory is added to the application resources (`build.gradle`), so `ans.4th` is available at runtime.
- From WSL, Gradle cannot run from a UNC working directory. `run-tests-windows.sh` copies `platforms/java`, `4th/` and `forth2012-test-suite/` to `C:\build\sloth-java` and runs the tests there.
