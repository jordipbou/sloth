# DODO

Dodo is the Java implementation of Sloth (aka SLOw forTH).

# Data types

Cell size: 32 bits (java's int).
Char size: 16 bits (java's char).

# Implementation

Dodo implements the usual data and return stack but also a third stack for Java objects.

A third stack is implemented to allow easy interoperation between Sloth and Java, mapping an stack of Java objects to the linear memory model required by Forth.

This stack is not meant to be used directly inside Forth but from the Java API only.

It allows to store object addresses in variables and to compile object literals.

## Code representation

Cell sized tokens are used to represent either primitives or word addresses in code.

An address (cell size) has two parts, object address and index address.

The objects address is an index to the object stack. It represents an object in the Java world.

The index address is an index inside that object. Depending on the object's type, it will index different types of data. The dictionary and the user area are both ByteBuffer objects accessed at the byte level. The input buffer is just a string object accessed at char level.

