- This commentary is very explicative:

https://forth-standard.org/standard/core/POSTPONE#reply-600

"Sure - as I said, precisely explained but essentially incomprehensible."

It refers to how difficult is to understand that POSTPONE compiles immediate
words but compiles the compilation of a non immediate word.





The ideas that led to the development of SLOTH were:

* A very simple implementation for a scripting language very easily embedabble
* Reuse an existing language (Forth) that happens to be the only interactive
  programming language without garbage collector that can make realtime
  applications (reusing a language allows using another implementations of
  that language -like VfxForth or Mecrisp Stellaris)

But trying to implement ANS Forth has only led to frustration. Constantly.
Having to implement everything that the standard requires only brings to
not allow innovation.

Things that I hate about ANS Forth:

* Doubles. They are supposed to be implemented in an extension (Doubles) but
  they are everywhere in core. You cannot implement . without implementing 
  doubles. And . is one of the most basic words for interactivity.
* colonsys/loopsys and all that is supposed to be put on a control stack for
  compilation. Its incredible difficult to understand what is needed and why
  without having a previous implementation.
  And the problem is that they mix implementation details with functionality.
* DO/?DO/UNLOOP/LEAVELOOP/+LOOP. This for me its the worst. Its utterly 
  difficult to implement. The basic control structures of Forth AHEAD/IF/THEN
  BEGIN/REPEAT/AGAIN and its variants ELSE/WHILE/UNTIL are beautiful because
  a word does one thing and its independent of the others.
  But to implement DO/LOOP you need to think on DO/+LOOP also. And its tough.
  UNLOOP is an implementation detail. The standard discourages the use of
  the return stack as a way to create complex control structures, like
  backtracking but, at the same time, says that loop-sys has to be put on
  the return stack, limiting its to do things like >r r> inside a DO/LOOP if
  you want to access I.
* And overall, its too big. Incredible big. It does not seem to, but Forth is
  a small idea, and the standard is a big publication.
