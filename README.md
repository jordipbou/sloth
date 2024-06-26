# SLOTH (SLOw forTH / Scripting Language Of The Heaven/Hell)

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

s"

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
