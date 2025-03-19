/* ---------------------------------------------------- */
/* ------------------ SLOTH Forth --------------------- */
/* ---------------------------------------------------- */

#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>

/* This were used by Claude's FM/MOD implementation */
#include <stddef.h>
#include <limits.h>

/* ---------------------------------------------------- */
/* ---------------- Virtual machine ------------------- */
/* ---------------------------------------------------- */
/* This is the reference implementation of the SLOTH    */
/* Virtual Machine.                                     */
/* ---------------------------------------------------- */
/* This API defines how the virtual machine works and   */
/* allow access to its internals from the host.         */
/* ---------------------------------------------------- */
/* It uses a table of primitives (C functions that can  */
/* be called from Forth) that the bootstrapped          */
/* programming language can use to interact with the    */
/* virtual machine.                                     */
/* ---------------------------------------------------- */

typedef int8_t CHAR;
typedef intptr_t CELL;
typedef uintptr_t uCELL;

#if UINTPTR_MAX == UINT64_MAX
	#define hCELL_MASK 0xFFFFFFFF
	#define hCELL_BITS 32
#endif
#if UINTPTR_MAX == UINT32_MAX
	#define hCELL_MASK 0xFFFF
	#define hCELL_BITS 16 
#endif
#if UINTPTR_MAX == UINT16_MAX
	#define hCELL_MASK 0xFF
	#define hCELL_BITS 8
#endif

#define sCELL sizeof(CELL)
#define sCHAR sizeof(CHAR)
#define CELL_BITS sCELL*8

/* Predefined sizes */
/* TODO: they should be modifiable before context creation */
/* or before including this file */
#define STACK_SIZE 64
#define RETURN_STACK_SIZE 64
#define DSIZE 65536
#define PSIZE 512

struct VM;
typedef void (*F)(struct VM*);

typedef struct PRIMITIVES {
	CELL sz;
	CELL last;
	F *p;
} P;

typedef struct VM { 
	CELL s[STACK_SIZE], sp;
	CELL r[RETURN_STACK_SIZE], rp;
	CELL ip;
	CELL d, sz;	/* Dict base address, dict size */

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	P *p;
} X;

/* -- Initialization of Sloth context ------------------ */

void init(X* x, CELL d, CELL sz);

/* -- Data stack --------------------------------------- */

void push(X* x, CELL v);
CELL pop(X* x);
CELL pick(X* x, CELL a);

/* -- Return stack ------------------------------------- */

void rpush(X* x, CELL v);
CELL rpop(X* x);
CELL rpick(X* x, CELL a);

/* -- Memory ------------------------------------------- */

/* 
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void store(X* x, CELL a, CELL v);
CELL fetch(X* x, CELL a);
void cstore(X* x, CELL a, CHAR v);
CHAR cfetch(X* x, CELL a);

/*
The next two functions allow transforming from relative to
absolute addresses.
*/
CELL to_abs(X* x, CELL a);
CELL to_rel(X* x, CELL a);

/* -- Inner interpreter -------------------------------- */

CELL op(X* x);
void do_prim(X* x, CELL p);
void call(X* x, CELL q);
void execute(X* x, CELL q);
void inner(X* x);
void eval(X* x, CELL q);

/* -- Exceptions --------------------------------------- */

void catch(X* x, CELL q);
void throw(X* x, CELL e);

/* -- Loading and saving already bootstrapped images --- */

void load_image(X* x, char* filename);
void save_image(X* x, char* filename);
