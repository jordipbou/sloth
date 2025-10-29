# Sloth — Universal Forth Engine

**Portable, multi-language ANS Forth for desktops, embedded, 
and scripting.**

---

**Sloth** is a portable, multi-language, cross-platform Forth 
system.

It provides the minimal foundation needed to bootstrap 
**ANS Forth** and test it in any environment — desktop, mobile, 
or embedded.

It’s built on the idea that **Forth itself** — simple, extensible, 
and close to the machine — should be available anywhere.  
And not just across **platforms**, but also across **languages**.  
That’s why **Sloth** is also designed to be **embedded as a 
scripting language** within other applications.

**ANS Forth** also serves as a **common virtual machine and
high-level assembler**, enabling the creation of higher-level 
languages that remain portable, lightweight, and consistent.

---

## Key Features

- **Based on Forth:** Utilizes Forth’s dual-stack, linear memory 
  virtual machine for efficient computation from microcontrollers 
  to large  computers.  
- **Interactive programming:** Designed for interactive use, 
allowing immediate feedback and iteration.  
- **Extensible:** Easy to extend with additional backends and
  functionalities.  
- **Performance:** Words can be implemented on the host for
  performance-critical sections without the need to modify sloth
	code.
- **Embeddable:** Can be used as a scripting language in other
  applications.  
- **Minimal native implementation:** Most of the system is 
  implemented in Forth itself, allowing easy porting to other 
	platforms.  
- **Cross-language / cross-platform:** Sloth aims to run on as many
  platforms and programming languages as possible.  
- **Easily hackable:** Every part of Sloth should be simple enough 
  for one developer to understand and modify for specific use cases.

---

## FAQ

**Why the name Sloth?**  
Sloths are beautiful animals. And I liked the play on words:
**“SLOw forTH.”**  
Later, I thought of **“Scripting Language Of The Hell/Heavens”** —  
and it stuck.

**Why Forth?**  
These are interesting properties of some programming languages:

- **Interactive** like Lisp, Smalltalk, or Python  
- **Low-level** enough for microcontrollers (like C)  
- **Simple** enough to be implemented by a single developer  
- **No garbage collector** for realtime applications

Forth is the only language that fulfills all four.









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
