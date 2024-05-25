# SLOTH (SLOw forTH / Scripting Language Of The Heaven/Hell)

SLOTH tries to be a small ANS Forth implementation that is easy to embed
and easy to use.

## Bootstrapping

Several options for bootstrapping have been tried: 

* A small ilo/retroforth like VM with an assembler and loading of a ROM.
* A complete ANS implementation in the host language.
* A minimal interpreter in the host enought to bootstrap from Forth.

At the end, it seems tat the best option for this project is to implement
in the host enough Forth words to have a full wordlist/recognizer based
Forth interpreter implemented in the host.

The main reasons for this decision are:

* Having most of required words in the host allows just one file to be included
  in a project.
* If primitives are added to the host, no modification has to be made to a ROM.
* Using an assembler like pali (from ilo) moves complexity from host
  implementation to assembler implementation. Although the assembler
	implementation can be shared by different hosts, I don't plan to implement
	so many hosts as ilo/retroforth has.
