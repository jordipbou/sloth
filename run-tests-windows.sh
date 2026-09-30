#!/bin/sh
# Runs every Sloth test on Windows from WSL, through powershell.exe.
# It runs every suite even if one fails and reports at the end.
#
# The C build lives on a Windows path (C:/build/sloth) because the
# Windows tools do not handle //wsl.localhost paths well as build
# directories; the source is read through the wsl.localhost UNC path.
#
# Java cannot run from a UNC working directory (Gradle refuses it),
# so platforms/java, 4th/ and forth2012-test-suite/ are copied to
# C:\build\sloth-java and run there.
#
# See README.md "Testing" and ~/hub/org/ref.knowledge.org.

set -u

here=$(cd "$(dirname "$0")" && pwd)
cd "$here"

src=$(wslpath -w "$here")
build='C:/build/sloth'
jbuild='C:\build\sloth-java'

fail=0

echo "== C =="
powershell.exe -NoProfile -Command "cmake -S '$src/platforms/c' -B $build -G 'Ninja Multi-Config'" || fail=1
powershell.exe -NoProfile -Command "cmake --build $build --config Debug" || fail=1
powershell.exe -NoProfile -Command "cmake --build $build --config Release" || fail=1
powershell.exe -NoProfile -Command "ctest --test-dir $build -C Debug --output-on-failure" || fail=1
powershell.exe -NoProfile -Command "ctest --test-dir $build -C Release --output-on-failure" || fail=1

echo "== Java =="
if powershell.exe -NoProfile -Command "java -version" >/dev/null 2>&1; then
	powershell.exe -NoProfile -Command "robocopy '$src\platforms\java' '$jbuild\platforms\java' /E /XD build .gradle /NFL /NDL /NJH /NJS /NP | Out-Null; robocopy '$src\4th' '$jbuild\4th' /E /NFL /NDL /NJH /NJS /NP | Out-Null; robocopy '$src\forth2012-test-suite' '$jbuild\forth2012-test-suite' /E /NFL /NDL /NJH /NJS /NP | Out-Null; Set-Location '$jbuild\platforms\java'; .\gradlew.bat test; \$t = \$LASTEXITCODE; .\gradlew.bat run --args=--test; \$r = \$LASTEXITCODE; if (\$t) { exit \$t } else { exit \$r }" || fail=1
else
	echo "java not found on the Windows PATH; skipping Java tests" >&2
fi

if [ "$fail" -ne 0 ]; then
	echo "Some tests failed."
	exit 1
fi

echo "All tests passed."
