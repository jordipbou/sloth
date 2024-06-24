# 2024/06/23

It occured to me that Sloth is not a VM, but an word based assembler.
The VM is important to make it work, but should not be in the specification.
Every word can be rewritten by other words and every word can be made
a primitive.
That's the important thing about Sloth.

# 2024/06/19

Input bufer needs its own reserved memory space to not fuck up compilations.

Also, the input buffer has to be big enough to manage nested includes.

# 2024/06/18

I'm having a hard time understanding parsing and input buffer.

Forth is based on a defined input string (c-addr u) and a parsing position
(>in).

I'm trying to use a system that can do what Rebol can do. And I thought
that any string should be parsed (not just input buffer).

But maybe is not an input buffer, but a parsing buffer.

I had first eliminated >in, thinking that it would be easier to make all
parsing words accept a source and return parsed strings ane rest of the source.

But in the meantime I have added to trace the ability to see the current token
and rest of the input buffer, and it has become very useful information.

Also, trying to implement ?: I've found that I don't have access to the start
of the input buffer and, as such, I can not go back.

That makes it necessary to use >in and then, it just does not makes sense
to pass source as a string to the words, better set source to a string (saving
the previous input buffer).

# ----

Ideas for next session:

* Think how to use ibuf/ilen/tok/tlen and if I need to pass things around
  for parsing (cumbersome and not trace friendly).

# 2024/06/18

Going back to previous Forth implementation but with transient region.

How to manage parsing and thinking about parsing strings gets weird.

I need new vocabulary to parsing things (like in Rebol):

PARSE/TOKEN -> ( str<input> -- str<token> str<input without token> )
: SOURCE ibuf ilen ;
: SOURCE! ilen! ibuf! ;
: TOKEN source parse/token source! ; ( -- c-addr u ) Like parse-name

: ?: source token find [ 2drop 0 ilen! ] [ 1 /string ] choose ;


# 2024/06/18

I've tried some implementation ideas, trying to simplify Forth to the maximum 
by just allowing quotations and some words implemented directly in the outer
interpreter. That was made to add a string rewriting layer in top of that to
manage syntaxes, and just use the 'Forth' as a virtual machine.

It works well, but to make it I changed some ideas I had first:

* No input buffer, just contiguous/transient regions

	This is a very interesting idea, but input buffer was put on transient in
	another experimental state where strings where used directly from input
	buffer. Copying the input buffer to transient allowed to maintain those
	strings live for a certain time.

	But input buffer crashes with compilation on transient region, as compilation
	target gets sliced.

	So this is not a good idea. It seems better to have:

	input buffer + contiguous region + transient region

* No parsing words

	This was to simplify, but as the input buffer is always there, losing 
	parsing words does not seem like a good idea. Everything has to be added
	to the outer interpreter. That means no extensibility. I don't like it.
	It's like breaking one of Forth strengths.

* No immediate words

	This is a consequence of the previous idea. Same thing, loses one of Forth
	strengths.

* Just quotations, no other control structure

	This is not a problem, it can be done in a traditional Forth also. But 
	losing branch and zbranch makes no sense.
