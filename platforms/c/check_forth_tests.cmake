# Runs the Sloth Forth 2012 and floating point suites and checks the
# number of reported failures against BASELINE. Invoked by CTest:
#
#   cmake -DSLOTH=<exe> [-DWORKDIR=<dir>] [-DBASELINE=1] \
#         -P check_forth_tests.cmake
#
# The suite writes binary data (the ACCEPT test prints its raw input
# buffer), so the output is read as hex and the failure markers are
# counted as byte patterns. This avoids NUL truncation and needs no
# external tools, so it works the same on Linux and Windows.
#
# Baseline 1 = the -0.4999E FROUND difference in the FP suite (it
# differs from the reference). The interactive Core ACCEPT test runs
# with a non-TTY stdin (INPUT_FILE below), so its KEY returns the
# return key without touching the console and the test passes.

if(NOT DEFINED SLOTH)
	message(FATAL_ERROR "SLOTH (path to the sloth executable) is required")
endif()
if(NOT DEFINED BASELINE)
	set(BASELINE 1)
endif()
if(NOT DEFINED WORKDIR)
	get_filename_component(WORKDIR "${SLOTH}" DIRECTORY)
endif()

set(out "${WORKDIR}/sloth-forth-tests.out")

# A non-TTY stdin so the interactive Core ACCEPT test does not block on
# the console (e.g. when run-tests.sh is launched from a terminal); the
# non-TTY KEY then returns the return key and ACCEPT terminates.
set(empty "${WORKDIR}/sloth-forth-tests.in")
file(WRITE "${empty}" "")

execute_process(
	COMMAND "${SLOTH}" --test
	WORKING_DIRECTORY "${WORKDIR}"
	INPUT_FILE "${empty}"
	OUTPUT_FILE "${out}"
	ERROR_QUIET
	RESULT_VARIABLE rc)

file(READ "${out}" hex HEX)
string(TOLOWER "${hex}" hex)

string(REGEX MATCHALL "494e434f525245435420524553554c54" incorrect "${hex}")
string(REGEX MATCHALL "57524f4e47204e554d424552204f4620524553554c5453" wrong "${hex}")
list(LENGTH incorrect n_incorrect)
list(LENGTH wrong n_wrong)
math(EXPR failures "${n_incorrect} + ${n_wrong}")

string(FIND "${hex}" "534c4f54482d544553542d444f4e45" done_at)
if(done_at EQUAL -1 OR NOT rc EQUAL 0)
	message(FATAL_ERROR "Forth suite did not run to completion (exit ${rc}).")
endif()

if(failures GREATER BASELINE)
	message(FATAL_ERROR
		"Forth suite reported ${failures} failures, baseline is ${BASELINE}.")
endif()

message(STATUS "Forth suite: ${failures} failures (baseline ${BASELINE}).")
