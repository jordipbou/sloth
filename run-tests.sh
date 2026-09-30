#!/bin/sh
# Runs every Sloth test on Linux: the C unit tests and Forth 2012
# suite (through CTest) and the Java JUnit tests and Forth 2012 suite.
# It runs every suite even if one fails and reports at the end.
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
else
	echo "java not found on PATH; skipping Java tests" >&2
	fail=1
fi

if [ "$fail" -ne 0 ]; then
	echo "Some tests failed."
	exit 1
fi

echo "All tests passed."
