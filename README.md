# Sloth — Universal Forth Engine

**Portable, multi-language ANS Forth for desktops, mobile,
embedded, and scripting.**

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

