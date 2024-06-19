# SLOTH (SLOw forTH / Scripting Language Of The Heaven/Hell)

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
