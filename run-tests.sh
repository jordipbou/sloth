#!/bin/sh
# Runs every Sloth test on Linux: the C unit tests and Forth 2012
# suite (through CTest) and the Java JUnit tests and Forth 2012 suite.
# It runs every suite even if one fails and reports at the end.
# Set SLOTH_TEST_BASH=1 to also run the (slow) bash kernel self-tests
# and the C-vs-bash differential check.
# See README.md "Testing".

set -u

here=$(cd "$(dirname "$0")" && pwd)
cd "$here"

fail=0

echo "== C =="
cmake -S platforms/c -B build -G "Ninja Multi-Config" || fail=1
cmake --build build --config Debug || fail=1
cmake --build build --config Release || fail=1
ctest --test-dir build -C Debug --output-on-failure || fail=1
ctest --test-dir build -C Release --output-on-failure || fail=1

echo "== Java =="
if ! command -v java >/dev/null 2>&1 && [ -n "${JAVA_HOME:-}" ] && [ -x "$JAVA_HOME/bin/java" ]; then
	PATH="$JAVA_HOME/bin:$PATH"
fi

if command -v java >/dev/null 2>&1; then
	cd platforms/java
	if [ -x ./gradlew ]; then g=./gradlew; else g="sh gradlew"; fi
	$g test || fail=1
	$g run --args=--test || fail=1
	cd "$here"
else
	echo "java not found on PATH; skipping Java tests" >&2
	fail=1
fi

echo "== JavaScript =="
if command -v node >/dev/null 2>&1; then
	(cd platforms/js && node --test) || fail=1
	if [ -x build/Debug/sloth ]; then
		./build/Debug/sloth platforms/js/test/differential.4th > build/js-differential-c.out
		node platforms/js/src/repl.js platforms/js/test/differential.4th > build/js-differential-js.out
		diff -u build/js-differential-c.out build/js-differential-js.out || fail=1
	else
		echo "C Debug binary not found; skipping the JavaScript differential check" >&2
	fi
else
	echo "node not found on PATH; skipping JavaScript tests" >&2
fi

echo "== Bash =="
if [ "${SLOTH_TEST_BASH:-0}" = "1" ]; then
	if command -v bash >/dev/null 2>&1; then
		bash platforms/bash/sloth.bash --test || fail=1
		if [ -x build/Debug/sloth ]; then
			./build/Debug/sloth platforms/js/test/differential.4th > build/bash-differential-c.out
			bash platforms/bash/sloth.bash platforms/js/test/differential.4th > build/bash-differential-sh.out
			diff -u build/bash-differential-c.out build/bash-differential-sh.out || fail=1
		else
			echo "C Debug binary not found; skipping the bash differential check" >&2
		fi
	else
		echo "bash not found on PATH; skipping bash tests" >&2
	fi
else
	echo "SLOTH_TEST_BASH not set; skipping the (slow) bash tests" >&2
fi

if [ "$fail" -ne 0 ]; then
	echo "Some tests failed."
	exit 1
fi

echo "All tests passed."
