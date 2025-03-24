#include "sloth.h"

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Constants --------------------------------------- */

/* Displacement of counted string buffer from here */
#define CBUF					64	/* Counted string buffer */
#define SBUF1					128	/* First string buffer */
#define SBUF2					256	/* Second string buffer */
#define NBUF					384	/* Pictured numeric output buffer */

/* Relative addresses of variables accessed both from C */
/* and Forth. */

#define HERE					0	
#define BASE					sCELL
#define LATEST				2*sCELL
#define LATESTXT			3*sCELL
#define STATE					4*sCELL
#define IBUF					5*sCELL
#define IPOS					6*sCELL
#define ILEN					7*sCELL
#define SOURCE_ID			8*sCELL
#define HLD						9*sCELL
#define IX						10*sCELL
#define JX						11*sCELL
#define KX						12*sCELL
#define LX						13*sCELL

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v);
CELL get(X* x, CELL a);
void cset(X* x, CELL a, CHAR v);
CHAR cget(X* x, CELL a);

/* Memory management */

CELL here(X* x);
void allot(X* x, CELL v);
CELL aligned(CELL a);
void align(X* x);

/* Compilation */

void comma(X* x, CELL v);
void ccomma(X* x, CHAR v);
void compile(X* x, CELL xt);
void literal(X* x, CELL n);

/* Findind words */

int compare_without_case(X* x, CELL w, CELL t, CELL l);
CELL find_word(X* x, char* name);

/* Sum of double numbers */

void _d_plus(X* x);

/* Including files */

void _included(X* x);

/* Printing the stack */

void _dot_s(X* x);

/* Headers */

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags CHAR					@ NT + 3*sCELL */
/* Namelen CHAR				@ NT + 3*sCELL + sCHAR */
/* Name CHAR*namelen	@ NT + 3*sCELL + 2*sCHAR */

CELL header(X* x, CELL n, CELL l);
CELL get_link(X* x, CELL w);
CELL get_xt(X* x, CELL w);
void set_xt(X* x, CELL w, CELL xt);
CHAR get_flags(X* x, CELL w);
CELL has_flag(X* x, CELL w, CELL v);
CHAR get_namelen(X* x, CELL w);
CELL get_name_addr(X* x, CELL w);

/* Setting flags */

#define HIDDEN				1
#define IMMEDIATE			2

void set_flag(X* x, CELL w, CHAR v);
void unset_flag(X* x, CELL w, CHAR v);

/* -- Outer interpreter ------------------------------- */

void _interpret(X*);

/* -- Primitives -------------------------------------- */

void _lit(X* x);
void _rip(X* x);
void _compile(X* x);
void _branch(X* x);
void _zbranch(X* x);
void _string(X* x);
void _quotation(X* x);
void _do_does(X* x);

/* -- Quotations (not in ANS Forth yet) ---------------- */

void _start_quotation(X* x);
void _end_quotation(X* x);

/* -- ANS Forth CORE words ----------------------------- */

/* Commands that help you start or end work sessions */

void _environment_query(X*);

/* Commands to inspect memory, debug & view code */

void _depth(X*);

/* Commands that change compilation & interpretation settings */

void _base(X*);
void _decimal(X*);

/* Source code preprocessing, interpreting & auditing commands */

/* Comment-introducing operations */

void _paren(X*);

/* String operations */

void _count(X*);
void _fill(X*);
void _hold(X*);
void _move(X*);
void _to_number(X*);
void _less_number_sign(X*);
void _number_sign_greater(X*);
void _number_sign(X*);
void _number_sign_s(X*);
void _sign(X*);

/* Disk input/output operations using files or block buffers */

/* More input/output operations */

void _accept(X*);
void _cr(X*);
void _dot(X*);
void _dot_quote(X*);
void _emit(X*);
void _key(X*);
void _space(X*);
void _spaces(X*);
void _type(X*);
void _u_dot(X*);

/* Arithmetic and logical operations */

void _abs(X*);
void _and(X*);
void _f_m_slash_mod(X*);
void _invert(X*);
void _l_shift(X*);
void _m_star(X*);
void _max(X*);
void _min(X*);
void _minus(X*);
void _mod(X*);
void _star_slash_mod(X*);
void _slash_mod(X*);
void _negate(X*);
void _one_plus(X*);
void _one_minus(X*);
void _or(X*);
void _plus(X*);
void _plus_store(X*);
void _r_shift(X*);
void _slash(X*);
void _s_m_slash_rem(X*);
void _star(X*);
void _star_slash(X*);
void _two_star(X*);
void _two_slash(X*);
void _u_m_star(X*);
void _u_m_slash_mod(X*);
void _xor(X*);

/* Number-type conversion operators */

void _s_to_d(X*);

/* Commands to define data structures */

void _constant(X*);
void _variable(X*);

/* Memory-stack transfer operations */

void _c_fetch(X*);
void _c_store(X*);
void _fetch(X*);
void _two_fetch(X*);
void _store(X*);
void _two_store(X*);

/* Comparison operations */

void _equals(X*);
void _greater_than(X*);
void _less_than(X*);
void _u_less_than(X*);
void _zero_equals(X*);
void _zero_less_than(X*);

/* System constants & facilities for generating ASCII values */

void _bl(X*);
void _char(X*);
void _bracket_char(X*);

/* Loop helpers (non ANS) */

void ipush(X* x);
void ipop(X* x);
void _doloop(X* x);

/* Forming definite loops */

void _do(X*);
void _i(X*);
void _j(X*);
void _leave(X*);
void _unloop(X*);
void _loop(X*);
void _plus_loop(X*);

/* Forming indefinite loops (compiling-mode only) */

void _begin(X*);
void _until(X*);
void _while(X*);
void _repeat(X*);

/* More facilities for defining routines (compiling-mode only) */

void _abort(X*);
void _abort_quote(X*);
void _colon(X*);
void _semicolon(X*);
void _exit_(X*);
void _if(X*);
void _else(X*);
void _then(X*);
void _left_bracket(X*);
void _quit(X*);
void _recurse(X*);
void _right_bracket(X*);
void _s_quote(X*);

/* Manipulating stack items */

void _drop(X*);
void _two_drop(X*);
void _dup(X*);
void _two_dup(X*);
void _question_dup(X*);
void _over(X*);
void _two_over(X*);
void _to_r(X*);
void _r_from(X*);
void _r_fetch(X*);
void _rot(X*);
void _swap(X*);
void _two_swap(X*);

/* Constructing compiler and interpreter system extensions */

void _align(X*);
void _aligned(X*);
void _allot(X*);
void _to_body(X*);
void _c_comma(X*);
void _cell_plus(X*);
void _cells(X*);
void _char_plus(X*);
void _chars(X*);
void _comma(X*);
void _create(X*);
void _does(X*);
void _evaluate(X*);
void _execute(X*);
void _find(X*);
void _here(X*);
void _immediate(X*);
void _to_in(X*);
void _bracket_tick(X*);
void _literal(X*);
void _postpone(X*);
void _source(X*);
void _state(X*);
void _tick(X*);
void _word(X*);
