#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<assert.h>
#include<limits.h> /* for CHAR_BIT */

#ifdef SLOTH_FOATING_POINT
#include<math.h>
#endif

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#endif

/* -- getch multiplatform definition and implementation  */

#ifdef WINDOWS
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

/* ----------------------------------------------------- */
/* ---------------- Virtual machine -------------------- */
/* ----------------------------------------------------- */
/* This is the 2nd reference implementation of the SLOTH */
/* Virtual Machine.                                      */
/* ----------------------------------------------------- */
/* This API defines how the virtual machine works and    */
/* allows access to its internals from the host.         */
/* ----------------------------------------------------- */
/* It uses a table of primitives (C functions that can   */
/* be called from Forth) that the bootstrapped           */
/* programming language can use to interact with the     */
/* virtual machine.                                      */
/* ----------------------------------------------------- */

/* -- Virtual Machine constants ------------------------ */

typedef int8_t BYTE; /* BYTEs are needed for MOVE */
typedef uint8_t uCHAR; /* CHARs are always unsigned */
typedef intptr_t CELL;
typedef uintptr_t uCELL;
#define suCHAR sizeof(uCHAR)

#ifdef SLOTH_FLOATING_POINT

	typedef double FCELL;
	typedef float SFCELL;
	typedef double DFCELL;

	#define sFCELL sizeof(FCELL)
	#define sSFCELL sizeof(SFCELL)
	#define sDFCELL sizeof(DFCELL)
	
	#define sFCELL_BITS sFCELL*8

#endif

#define ALIGNED(a, t) (((a) + ((t) - 1)) & ~((t) - 1))

#if UINTPTR_MAX == UINT64_MAX
	#define sCELL 8 
	#define CELL_BITS 64
	#define hCELL_MASK 0xFFFFFFFF
	#define hCELL_BITS 32
#endif
#if UINTPTR_MAX == UINT32_MAX
	#define sCELL 4
	#define CELL_BITS 32
	#define hCELL_MASK 0xFFFF
	#define hCELL_BITS 16 
#endif
#if UINTPTR_MAX == UINT16_MAX
	#define sCELL 2
	#define CELL_BITS 16
	#define hCELL_MASK 0xFF
	#define hCELL_BITS 8
#endif

#ifndef SLOTH_STACK_SIZE
#define SLOTH_STACK_SIZE 64
#endif

#ifndef SLOTH_RETURN_STACK_SIZE
#define SLOTH_RETURN_STACK_SIZE 64
#endif

#ifndef SLOTH_FLOAT_STACK_SIZE
#define SLOTH_FLOAT_STACK_SIZE 64
#endif

/* -- Virtual Machine context definition --------------- */

struct sloth_VM;
typedef void (*F)(struct sloth_VM*);

typedef struct sloth_PRIMITIVES {
	CELL pz;
	CELL last;
	F *p;
} sloth_P;

typedef struct sloth_VM { 
	CELL d, dz;	/* Dict base address, dict size */
	CELL u, uz; /* User area base address and size */
	CELL ip;

	CELL s[SLOTH_STACK_SIZE], sp;
	CELL r[SLOTH_RETURN_STACK_SIZE], rp;

	#ifdef SLOTH_FLOATING_POINT

		FCELL f[SLOTH_FLOAT_STACK_SIZE]; 
		CELL fp;

	#endif

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	sloth_P *p;
} X;

/* -- Exceptions --------------------------------------- */

#define SLOTH_STACK_OVERFLOW					-3
#define SLOTH_STACK_UNDERFLOW					-4
#define SLOTH_RETURN_STACK_OVERFLOW		-5
#define SLOTH_RETURN_STACK_UNDERFLOW	-6

/* -- Displacement of counted string buffer from here -- */

/* TODO CBuffer could be just another space of the normal */
/* strings circular buffer. */
#define SLOTH_CBUF							64*suCHAR

/* -- Dictionary variables ----------------------------- */

#define SLOTH_HERE							0	
#define SLOTH_INTERNAL_WL				1*sCELL
#define SLOTH_FORTH_WL					2*sCELL

/* -- User area variables and buffers ------------------ */

#define SLOTH_CURRENT						0*sCELL
#define SLOTH_ORDER							1*sCELL
#define SLOTH_LOCALS_WORDLIST		2*sCELL
#define SLOTH_CONTEXT						3*sCELL
/* There are 16 CELLS reserved to search order */
#define SLOTH_BASE							19*sCELL
#define SLOTH_STATE							20*sCELL
#define SLOTH_IBUF							21*sCELL
#define SLOTH_IPOS							22*sCELL
#define SLOTH_ILEN							23*sCELL
#define SLOTH_SOURCE_ID					24*sCELL
#define SLOTH_SOURCE_POS				25*sCELL
#define SLOTH_LATESTXT					26*sCELL
#define SLOTH_INTERPRET					27*sCELL

#define SLOTH_ROOT_PATH_LENGTH	28*sCELL
#define SLOTH_PATH_START				29*sCELL
#define SLOTH_PATH_END					30*sCELL
/* Continuous space to store path strings */
#define SLOTH_PATHS							31*sCELL

/* Space between SLOTH_PATHS and SLOTH_INCLUDED_FILES */
/* reserved to store paths. */

#define SLOTH_INCLUDED_FILES		95*sCELL

#ifdef SLOTH_FLOATING_POINT

	#define SLOTH_PRECISION				96*sCELL
	#define SLOTH_LAST_USER_VAR		97*sCELL

#else

	#define SLOTH_LAST_USER_VAR		96*sCELL

#endif

/* -- Flags for word status ---------------------------- */

#define SLOTH_HIDDEN					1
#define SLOTH_IMMEDIATE				2

/* -- Data and return stack ---------------------------- */

void sloth_push(X* x, CELL v);
CELL sloth_pop(X* x);
void sloth_rpush(X* x, CELL v);
CELL sloth_rpop(X* x);

void sloth_dup_(X* x);
void sloth_drop_(X* x);
void sloth_over_(X* x);
void sloth_swap_(X* x);
void sloth_to_r_(X* x);
void sloth_r_from_(X* x);

#ifdef SLOTH_FLOATING_POINT

	/* -- Floating point stack ----------------------------- */

	void sloth_fpush(X* x, FCELL v);
	FCELL sloth_fpop(X* x);
	FCELL sloth_fpick(X* x, CELL a);

#endif

/* -- Memory management -------------------------------- */

/* Transform from relative to absolute addresses. */
CELL sloth_to_abs(X* x, CELL a);
CELL sloth_to_rel(X* x, CELL a);

/* STORE/FETCH/CSTORE/cfetch work on absolute address units, */
/* not just inside SLOTH dictionary (memory block). */
void sloth_b_store(X* x, CELL a, BYTE v);
BYTE sloth_b_fetch(X* x, CELL a);
void sloth_c_store(X* x, CELL a, uCHAR v);
uCHAR sloth_c_fetch(X* x, CELL a);
void sloth_store(X* x, CELL a, CELL v);
CELL sloth_fetch(X* x, CELL a);

#ifdef SLOTH_FLOATING_POINT

	void sloth_fstore(X* x, CELL a, FCELL v);
	FCELL sloth_ffetch(X* x, CELL a);
	void sloth_sfstore(X* x, CELL a, SFCELL v);
	SFCELL sloth_sffetch(X* x, CELL a);
	void sloth_dfstore(X* x, CELL a, DFCELL v);
	DFCELL sloth_dffetch(X* x, CELL a);

#endif

/* Setting and getting cells in memory */

void sloth_set(X* x, CELL a, CELL v);
CELL sloth_get(X* x, CELL a);

void sloth_user_set(X* x, CELL a, CELL v);
CELL sloth_user_get(X* x, CELL a);

/* Working with the HERE pointer */

CELL sloth_here(X* x);
void sloth_allot(X* x, CELL v);
CELL sloth_aligned(CELL a);

void sloth_here_(X* x);
void sloth_allot_(X* x);
void sloth_align_(X* x);

/* Moving data from stack to dictionary and viceversa */

void sloth_c_fetch_(X* x);
void sloth_c_store_(X* x);
void sloth_fetch_(X* x);
void sloth_store_(X* x);

void sloth_cells_(X* x);
void sloth_chars_(X* x);

/* Inspecting memory */

void sloth_unused_(X* x);

/* -- Basic compilation -------------------------------- */

void sloth_comma(X* x, CELL v);
void sloth_c_comma(X* x, uCHAR v);

#ifdef SLOTH_FLOATING_POINT

	void sloth_fcomma(X* x, FCELL v);

#endif

/* -- Headers ------------------------------------------ */

CELL sloth_get_latest(X* x);
void sloth_set_latest(X* x, CELL w);

CELL sloth_get_link(X* x, CELL w);

CELL sloth_get_xt(X* x, CELL w);
void sloth_set_xt(X* x, CELL w, CELL xt);

uCHAR sloth_get_flags(X* x, CELL w);

void sloth_set_flags(X* x, CELL w, uCHAR v);

CELL sloth_has_flag(X* x, CELL w, CELL v);

void sloth_set_flag(X* x, CELL w, uCHAR v);

void sloth_unset_flag(X* x, CELL w, uCHAR v);

uCHAR sloth_get_namelen(X* x, CELL w);

CELL sloth_get_name_addr(X* x, CELL w);

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

CELL sloth_header(X* x, CELL n, CELL l);

/* -- Inner interpreter -------------------------------- */

CELL sloth_op(X* x);
void sloth__do_prim(X* x, CELL p);
void sloth__call(X* x, CELL q);
void sloth__execute(X* x, CELL q);
void sloth__inner(X* x);

void sloth_eval(X* x, CELL q);

#ifdef SLOTH_FLOATING_POINT

	FCELL sloth_fop(X* x);

#endif

/* -- Tracing inner interpreter ------------------------ */

void sloth__debug(X* x, CELL debug_xt);
void sloth__debug_inner(X* x, CELL debug_xt);

void sloth_debug_(X* x);

/* Inner interpreter primitives */

void sloth_exit_(X* x);

void sloth_lit_(X* x);
void sloth_rip_(X* x);

#ifdef SLOTH_FLOATING_POINT
	
	void sloth_flit_(X* x);

#endif

void sloth_branch_(X* x);
void sloth_zbranch_(X* x);

/* -- Exceptions --------------------------------------- */

void sloth_catch(X* x, CELL q);
void sloth_throw(X* x, CELL e);

void sloth_catch_(X* x);
void sloth_throw_(X* x);

/* -- Arithmetic and logical operations ---------------- */

void sloth_and_(X* x);
void sloth_invert_(X* x);
void sloth_l_shift_(X* x);
void sloth_minus_(X* x);
void sloth_plus_(X* x);
void sloth_r_shift_(X* x);
void sloth_star_(X* x);
void sloth_two_slash_(X* x);
void sloth_u_m_star_(X* x);
void sloth_u_m_slash_mod_(X* x);

/* -- Comparison operations ---------------------------- */

void sloth_equals_(X* x);
void sloth_less_than_(X* x);

/* -- Strings ------------------------------------------ */

void sloth_string_(X* x);

/* TODO (CSTRING) is used only by CLITERAL that is used only */
/* by C" that is never used. It could be taken from here by */
/* making a Forth version that stores a normal string literal */
/* and copies it to a transient region or to the CBuffer, */
/* or even to the normal string buffer. */
void sloth_c_string_(X* x);

/* MOVE moves address units, not characters */
void sloth_move_(X* x);

/* -- Searching for words ------------------------------ */

/* Compare without case */
int sloth__compare(X* x, CELL a1, uCELL u1, CELL a2, uCELL u2);
CELL sloth__search_word(X* x, CELL n, int l);
CELL sloth_find_word(X* x, char* name);

void sloth_find_(X* x);

/* -- More compilation --------------------------------- */

void sloth_compile(X* x, CELL xt);
void sloth_literal(X* x, CELL n);

#ifdef SLOTH_FLOATING_POINT

	void sloth_fliteral(X* x, FCELL n);

#endif

/* -- Quotations --------------------------------------- */

void sloth_quotation_(X* x);
void sloth_start_quotation_(X* x);

void sloth_end_quotation_(X* x);

/* -- End work session --------------------------------- */

void sloth_bye_(X* x);

/* -- Input/output and parsing operations -------------- */

/* Unicode does not work correctly on Windows cmd.exe or */
/* Windows Terminal because Windows uses UTF-16 by default. */
void sloth_emit_(X* x);
void sloth_key_(X* x);

void sloth_source_(X* x);
void sloth_word_(X* x);

#ifndef SLOTH_NO_FILES
void sloth_file_position_(X* x);
void sloth_read_line_(X* x);
#endif

/* TODO Refill could be implemented in ans.4th. */
/* As I have created the api functions READ-LINE and */
/* FILE-POSITION. */
/* Except for the comments on the few first lines (that can */
/* be removed without problems as \ is the fourth definition */
/* found), REFILL is not used again until the definition of */
/* ( in line 196 */
void sloth_refill_(X* x);

/* TODO Could SAVE-INPUT and RESTORE-INPUT be implemented */
/* in ANS Forth? SAVE-INPUT surely... */
void sloth_save_input_(X* x);
void sloth_restore_input_(X* x);

#ifndef SLOTH_NO_FILES
void sloth__save_input_and_path(X* x);
void sloth__restore_input_and_path(X* x);
FILE* sloth__open_included_file(X* x, char* a, int l);
void sloth__add_to_included_files_list(X* x, char* a, int l);

/* TODO INCLUDED can not be implemented in ANS Forth because */
/* its needed to include ans.4th itself. */
/* But this function is very complex and that will make it */
/* error prone when porting to another language. Try to */
/* simplify it and use READ-LINE, etc. from Forth */

/* INCLUDED is a complex function because it tries to find */
/* the indicated file in several directories. */
/* It first tries to open it as an absolute path/current */
/* directory. If its not possible to open it, it reuses the */
/* last path from the previous opened file. */
void sloth_included_(X* x);
#endif

/* -- Defining words ----------------------------------- */

void sloth_colon_(X* x);
void sloth_colon_no_name_(X* x);
void sloth_semicolon_(X* x);
void sloth_recurse_(X* x);
void sloth_immediate_(X* x);
void sloth_postpone_(X* x);

void sloth_compile_comma_(X* x);
void sloth_create_(X* x);
void sloth_do_does_(X* x);
void sloth_does_(X* x);

void sloth_evaluate_(X* x);
void sloth_execute_(X* x);

/* -- Outer interpreter -------------------------------- */

void sloth_interpret_(X* x);

/* -- Environment queries ------------------------------ */

void sloth_environment_(X* x);

/* -- Floating point word set ------------------------- */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x);
void sloth_f_aligned_(X* x);
void sloth_f_literal_(X* x);
void sloth_floats_(X* x);
void sloth_float_plus_(X* x);

void sloth_s_f_aligned_(X* x);
void sloth_d_f_aligned_(X* x);

void sloth_s_floats_(X* x);
void sloth_d_floats_(X* x);

/* Manipulating stack items */

void sloth_f_depth_(X* x);
void sloth_f_drop_(X* x);
void sloth_f_dup_(X* x);
void sloth_f_over_(X* x);
void sloth_f_rot_(X* x);
void sloth_f_swap_(X* x);

/* Comparison operations */

void sloth_f_less_than_(X* x);
void sloth_f_zero_less_than_(X* x);
void sloth_f_zero_equals_(X* x);

/* Memory-stack transfer operations */

void sloth_f_fetch_(X* x);
void sloth_f_store_(X* x);
void sloth_s_f_fetch_(X* x);
void sloth_s_f_store_(X* x);
void sloth_d_f_fetch_(X* x);
void sloth_d_f_store_(X* x);

/* Commands to define data structures */

void sloth_f_constant_(X* x);
void sloth_f_variable_(X* x);

/* Number-type conversion operators */

void sloth_d_to_f_(X* x);
void sloth_f_to_d_(X* x);

/* Arithmetic and logical operations */

void sloth_f_abs_(X* x);
void sloth_f_plus_(X* x);
void sloth_f_minus_(X* x);
void sloth_f_star_(X* x);
void sloth_f_star_star_(X* x);
void sloth_f_slash_(X* x);
void sloth_floor_(X* x);
void sloth_f_max_(X* x);
void sloth_f_min_(X* x);
void sloth_f_negate_(X* x);
void sloth_f_round_(X* x);
void sloth_f_proximate_(X* x);
void sloth_f_atan2_(X* x);
void sloth_f_sqrt_(X* x);
void sloth_f_l_n_(X* x);
void sloth_f_sine_(X* x);
void sloth_f_cos_(X* x);
void sloth_f_sine_cos_(X* x);
void sloth_f_tan_(X* x);
void sloth_f_a_sine_(X* x);
void sloth_f_a_cos_(X* x);
void sloth_f_a_tan_(X* x);
void sloth_f_exp_(X* x);
void sloth_f_exp_m_one_(X* x);
void sloth_f_log_ten_(X* x);
void sloth_f_l_n_p_one_(X* x);
void sloth_f_a_log_(X* x);
void sloth_f_sin_h_(X* x);
void sloth_f_cos_h_(X* x);
void sloth_f_tan_h_(X* x);
void sloth_f_a_sine_h_(X* x);
void sloth_f_a_cos_h_(X* x);

/* String/numeric conversion */

void sloth_to_float_(X* x);
void sloth_represent_(X* x);

/* Output operations */

void sloth_f_dot_(X* x);
void sloth_f_s_dot_(X* x);
void sloth_f_e_dot_(X* x);

/* Non ANS floating point helpers */

void sloth_f_dot_s_(X* x);


/* -- Primitives that I don't like too much ------------ */

void sloth_dict_(X* x);
void sloth_empty_return_stack_(X* x);

/* -- Primitive, word and user variable creation ------- */

CELL sloth_primitive(X* x, F f);
CELL sloth_code(X* x, char* name, CELL xt);
void sloth_user_variable(X* x, char* name, CELL d, CELL v);

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_kernel(X* x);
void sloth_bootstrap(X* x);

/* -- Context initialization and destruction ----------- */

void sloth__init(X* x, CELL d, CELL dz, CELL u, CELL uz);
X* sloth_create(int psize, int dsize, int usize);
X* sloth_new();
void sloth_free(X* x);

/* -- Helpers to work with files from C ---------------- */

void sloth_set_root_path(X* x, char* s);
int sloth_include(X* x, char* f);
void sloth_evaluate(X* x, char* s);

/* -- Helper REPL -------------------------------------- */

void sloth_repl(X* x);
