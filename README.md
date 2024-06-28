# Sloth programming language/framework

Sloth is a programming language/framework designed for interactive programming
across a wide range of platforms.

The language leverages the simplicity and power of the Forth virtual machine to
provide a versatile and efficient programming environment.

Initially Sloth will support C and Java backends.

## Key Features

- Based on Forth: Utilizes Forth's dual-stack, linear memory virtual machine
for efficient computation from microcontrollers to large computers.
- Interactive programming: Designed for interactive use, allowing immediate 
feedback and iteration.
- Extensible: Easy to extend with additional backends and functionalities.
- Performance: Words can be implemented on the host for performance-critical
sections.

## Project goals

- Cross-language/cross-platform: Make Sloth availables on as many platforms and
programming languages as possible.
- Easily hackable: Every part of Sloth has to be easily understood by only one
developer, allowing modification as required for each use case.

## FAQ

* Why the name Sloth?

Initially it comes from "SLOw forTH". And I like the animal. So it stayed.
Later, I thought about "Scripting Language Of The Hell/Heavens", and it looked
funny.

* Why Forth?

- Interactive programming languages: Forth/Lisp/Smalltalk/Python/J
- Programming languages that work on microcontrollers: C/Forth/Python
- Programming languages easy to implement by one person: Forth

Forth is the only language that fulfills the three requirements.

## Documentation

TODO

## Tutorials

TODO

## Contributing

TODO

## License

TODO

## Contact

TODO

--

By leveraging the power and simplicity of Forth, Sloth aims to provide a robust
and flexible programming environment for a wide range of applications. Join us
in developing and expanding Sloth to unlock its full potential!



# SLOTH (SLOw forTH / Scripting Language Of The Heaven/Hell)

Sloth is a programming language that tries to abstract away from hardware 
allowing its implementation on a lot of platforms.

The main idea is to reuse knowledge by not needing to constantly change the
language.

Forth is perfect to be the base because its typeless and actions are 
initiated by calling a word that is parameterless. You just have a computer
in a state, call a word and end in a new state. Everything can happen in that
word, and that's where the magic happends, because you can abstract every
word.

## Problems with ANS words

Basically, it's complicated.

Take DOES> for example, as by the standard it "replaces the execution 
semantics of the most recent definition...". In Gerry Jackson test suite this
there is this strange test (it's called weird):

	T{ : WEIRD: CREATE DOES> 1 + DOES> 2 + ; -> }T
	T{ WEIRD: W1 -> }T
	T{ ' W1 >BODY -> HERE }T
	T{ W1 -> HERE 1 + }T
	T{ W1 -> HERE 2 + }T

That means that executing WEIRD: W1 creates a definition called W1 and
replaces its execution semantics with 1 + DOES> 2 + so when called will
execute 1 + and replace its execution semantics with 2 +.

Everything seems correct, and logical. But it's overly complicated to think
about it!! And that leads to a Big Forth implementation. It's not possible
to implement a small Forth and follow the ANS standard. It's not about the
number of words that need to be implemented, it's how they are related that
makes it big.

If the above definition of WEIRD: has to work, that limits the amount of 
"keep it simple" that one can apply to the language.


## Basic words

### Stacks

DROP DUP OVER SWAP >R R@ R>

### Memory

! @ C! C@ HERE HERE! UNUSED CELL CHAR

### Comparison

< = > 

### Arithmetic

+ - * / MOD RSHIFT

### Logic/Bit

AND OR XOR INVERT LSHIFT

### Input source

IBUF IPOS ILEN IBUF! IPOS! ILEN! REFILL

### Input/Output

KEY EMIT INCLUDED

### Strings

S"

### Execution and defining words (compiler?)

EXECUTE THROW CATCH CREATE DOES> : ; POSTPONE IMMEDIATE RECURSE

### Simple control flow

AHEAD IF THEN BEGIN REPEAT AGAIN ELSE WHILE

### Quotations and combinators

[ ] CHOOSE






# Current idea

3 interpreters:

* String rewriting interpreter, written in Sloth/0
* Sloth/0 interpreter, bootstrapped from Java/C/ASM...
* Inner (VM) interpreter

# String rewriting interpreter:

Has two data constructs, normal form and input sequence:

stack : astack : normal : input

First, an longest-leftmost strategy is used to apply the rules to the input.
Second, when no more rules can be applied (and there has been no error) we're
left with the normal form. The normal form is sent to the Sloth/0
interpreter/compiler.
Third, VM code is executed (if needed)
