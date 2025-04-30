Recognizers
===========

Matthias Trute <mtrute@web.de>
Version 2.1.0 - 2017-09-14

This package is a collection of recognizer
examples. To play with them a simple implementation
is provided. It works unchanged on (old) gforth's,
swiftforth and VFX. They are not (yet) able to 
compile code or are otherwise integrated into 
the underlying system.

Many examples contain test units that illustrate what
the recognizer does.

More information about recognizers can be found
at http://amforth.sourceforge.net/Recognizers.html
and http://forth200x.org

Running the examples with e.g. gforth should not
give any errors beside a possible redefinition of `{`

    $ gforth rec-double-paren.4th 
    redefined {  Gforth 0.7.2, Copyright (C) 1995-2008 Free Software Foundation, Inc.
    Gforth comes with ABSOLUTELY NO WARRANTY; for details type `license'
    Type `bye' to exit
    bye 
    $

The code examples use rev 4 of the recognizer RFD.
Package version 1.x use the rev 3 of it.

`Recognizer.4th`
----------------

This file in combination with `Stack.4th` gives a
playground for making experiments with recognizers. It
has been tested with gforth versions that currently
come with the linux distributions, MPE's vfxlin
and Forth Inc's Swiftforth for Linux.

`rec-num.4th`
--------------

This file contains four recognizers. They deal with
numbers in various formats and use `>NUMBER` for the
actual number conversion. 

`rec-char ( addr len -- n rectype-num  | rectype-null )`

Forth 2012 defined the 'c' syntax for single characters.
That means that `char c` or `[char] c` can be replaced
with `'c'`.

`rec-snum ( addr len -- n rectype-num  | rectype-null )`

This recognizer handles single cell numbers. It
checks whether a strings consists of digits only
and that the value range of the number fits into a
single cell. The sign and base prefix characters
work as expected.

`rec-dnum  ( addr len -- d rectype-dnum  | rectype-null )`

This recognizer handles double cell numbers. It accepts
the standard double number format: digits with a trailing
dot in addition to the base prefix and sign character.

These three recognizers are combined into the `rec-num`
recognizer that can be used to handle all number formats
in one call.

As a special extension, the 0x prefix is used to switch
to hex too. It requires the string wordset (COMPARE) to
be present.

`rec-find.4th`
--------------

This file contains a dictionary lookup recognizer. It uses
`FIND` for the actual work thus uses the search order if
present.

`rec-name.4th`
----------------

Another dictionary lookup recognizer. This one does not
depend on `FIND` and searches the standard forth wordlist
only. It returns name tokens instead of execution tokens.

It requires `TRAVERSE-WORDLIST` and the `NAME>x` words
from Forth 2012 to work. From the future, quotations are 
used too.

`rec-string.4th`
----------------

Uses `"` as the string delimiter. Everything
between two `"` within `SOURCE` is a string.
It can replace the forth command `S"` completly.
Instead of `S" foo"` use `"foo"`. The space
after `S"` is no longer needed, it is now part 
of the string. `S" foo"` and `" foo"`
differ with the leading space in the latter.

The string lives as long as `SOURCE` is 
unchanged! More sophisticated implementations
may use a string stack. Compilation to the 
dictionary is done by `SLITERAL`. Postponing 
throws an exception simply because that
is not specified in the standard.

A typical use is

    "hello world" type
    hello world
    : test "hello world" type ; test
    hello world

that prints the string resp. compiles it and
prints it at runtime.

`rec-notfound.4th`
------------------

This is not really a recognizer but an API wrapper for
the often found `not-found` hook. It discards any input
and calls a deferred word `not-found`. This word is
expected to never return properly by e.g. printing an
error message and throwing an exception.

With this recognizer as the last one in the stack, programs
that use the not-found hook can easily adapted.

`rec-double-paren.4th`
----------------------

Implements the multiline `((` `))` comment block.

The `((` switches the system recognizer stack to
one that searches only one otherwise hidden wordlist.
This wordlist contains only words that are allowed
to be executed in comments. For now only `))` that
switches back to normal operation.

Since the recognizer stack switch is unaffected from
`REFILL` operations, multiline comments work too.

This recognizer needs a system that has recognizers
native support. It does not depend on `SOURCE` however.
It re-uses the REFILL-PARSE-EVALUATE loop found in
the text interpreter. Some tests are provided.

`literacy.4th`
--------------

Simple formatting of source code in the
spirit von Don Knuth' literate programming. You can
combine the real source code and its documentation
in one file that the standard forth interpreter
can handle directly.

Actually testing this recognizer requires a Forth that
supports recognizers already.

Author: Julian Fondren, 2014
License: probably public domain.

`rec-time.4th`
--------------

check and convert for the hh:mm:ss notation returning
a double cell number for the number of seconds it
represents.
