#include "sloth.h"

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
void _exit(X*);
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
