#include "vm.h"

/* This are used by Claude's FM/MOD implementation */
#include <stddef.h>
#include <limits.h> /* for CHAR_BIT */

/* -- getch multiplatform implementation (for KEY) ----- */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
int getch() {
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}
#endif 

/* Commands that can help you start or end work sessions */

void _environment_q(X* x) { 
	/* TODO */
}

/* Manipulating stack items */

void _dup(X* x) { push(x, pick(x, 0)); }
void _rot(X* x) { 
	CELL a = pop(x); 
	CELL b = pop(x); 
	CELL c = pop(x); 
	push(x, b); 
	push(x, a); 
	push(x, c); 
}
void _nip(X* x) { CELL v = pop(x); pop(x); push(x, v); }
void _tuck(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a); push(x, b); push(x, a); }
void _roll(X* x) { /* TODO */ }
void _question_dup(X* x) { if (pick(x, 0) != 0) _dup(x); }
void _r_fetch(X* x) { push(x, rpick(x, 0)); }
void _two_drop(X* x) { pop(x); pop(x); }
void _two_dup(X* x) { push(x, pick(x, 1)); push(x, pick(x, 1)); }
void _two_swap(X* x) { 
	CELL a = pop(x); 
	CELL b = pop(x); 
	CELL c = pop(x); 
	CELL d = pop(x); 
	push(x, b); 
	push(x, a); 
	push(x, d); 
	push(x, c); 
}
void _two_to_r(X* x) { 
	CELL a = pop(x); 
	rpush(x, pop(x)); 
	rpush(x, a); 
}
void _two_r_fetch(X* x) { 
	push(x, rpick(x, 1)); 
	push(x, rpick(x, 0)); 
}
void _two_r_from(X* x) { 
	CELL a = rpop(x); 
	push(x, rpop(x)); 
	push(x, a); 
}
void _two_over(X* x) { 
	push(x, pick(x, 3)); 
	push(x, pick(x, 3)); 
}/* Helper function for _to_number */
void _digit(X* x) {
	_to_r(x);
	_dup(x); push(x, 'a'); _less_than(x); _zero_equals(x); /* NOT */
	if (pop(x)) {
		push(x, 'a'); _minus(x); push(x, 'A'); _plus(x);
	}
	_dup(x); _dup(x); push(x, 'A'); _one_minus(x); _greater_than(x);
	if (pop(x)) {
		push(x, 'A'); _minus(x); push(x, '9'); _plus(x); _one_plus(x);
	} else {
		_dup(x); push(x, '9'); _greater_than(x);
		if (pop(x)) {
			_drop(x); push(x, 0);
		}
	}
	push(x, '0'); _minus(x);
	_dup(x); _r_from(x); _less_than(x);
	if (pop(x)) {
		_dup(x); _one_plus(x); _zero_greater_than(x);
		if (pop(x)) {
			_nip(x); _true(x);
		} else {
			_drop(x); _false(x);
		}
	} else {
		_drop(x); _false(x);
	}
}

/* Code adapted from pForth Forth code */
void _to_number(X* x) {
	_to_r(x);
	do { /* BEGIN */
		_r_fetch(x); _zero_greater_than(x);
		if (pop(x)) {
			_dup(x); _c_fetch(x); _base(x); _fetch(x);
			_digit(x);
			if (pop(x)) {
				_true(x);
			} else {
				_drop(x); _false(x);
			}
		} else {
			_false(x);
		}
		if (pop(x) == 0) break; /* WHILE */
		_swap(x); _to_r(x);
		_swap(x); _base(x); _fetch(x);
		_u_m_star(x); _drop(x);
		_rot(x); _base(x); _fetch(x);
		_u_m_star(x);
		_d_plus(x);
		_r_from(x); _one_plus(x);
		_r_from(x); _one_minus(x); _to_r(x);
	} while(1);
	_r_from(x);
}

/* Source code preprocessing, interpreting & auditing commands */

void _dot_paren(X* x) { 
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != ')') {
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, IPOS) != get(x, ILEN)) 
		set(x, IPOS, get(x, IPOS) + 1);
}

/* More input/output operations */

void _cr(X* x) { printf("\n"); }
void _space(X* x) { printf(" "); }
void _spaces(X* x) { 
	CELL i, u = pop(x); 
	for (i = 0; i < u; i++) printf(" ");
}
void _type(X* x) { 
	CELL l = pop(x); 
	CELL a = pop(x);
	CELL i;
	CHAR c;
	for (i = a; i < a + l; i++) {
		c = cfetch(x, i);
		if (c >= 32 && c <= 126) printf("%c", c);
	}
}void _dot_quote(X* x) { 
	CELL u, addr, i;
	push(x, '"'); 
	_parse(x); 
	u = pop(x);
	addr = pop(x);
	compile(x, get_xt(x, find_word(x, "(STRING)")));
	comma(x, u);
	for (i = 0; i < u; i++) {
		ccomma(x, cfetch(x, addr + i));
	}
	align(x);
	compile(x, get_xt(x, find_word(x, "TYPE")));
}

void _accept(X* x) {
	CELL n = pop(x);
	CELL addr = pop(x);
	CELL i;
	CHAR c;
	for (i = 0; i < n; i++) {
		_key(x);
		_dup(x); _emit(x);
		c = (CHAR)pop(x);
		if (c < 32) { break; }
		cstore(x, addr + i, c);
	}
	push(x, i);
}

void _dot(X* x) { printf("%ld ", pop(x)); }
void _dot_r(X* x) { /* TODO */ }
void _u_dot(X* x) { printf("%lu ", (uCELL)pop(x)); }
void _u_dot_r(X* x) { /* TODO */ }

/* Forming definite loops */

void _do(X* x) { literal(x, 1); _start_quotation(x); }
void _question_do(X* x) { literal(x, 0); _start_quotation(x); }
void _i(X* x) { push(x, get(x, IX)); }
void _j(X* x) { push(x, get(x, JX)); }
void _leave(X* x) { set(x, LX, 1); p_exit(x); }
void _loop(X* x) { 
	literal(x, 1); 
	_end_quotation(x); 
	compile(x, get_xt(x, find_word(x, "(DOLOOP)"))); 
}
void _plus_loop(X* x) { 
	_end_quotation(x); 
	compile(x, get_xt(x, find_word(x, "(DOLOOP)"))); 
}

/* Forming indefinite loops (compiling-mode only) */

void _begin(X* x) { _here(x); }
void _again(X* x) { 
	compile(x, get_xt(x, find_word(x, "(BRANCH)"))); 
	_here(x); _minus(x); _comma(x); 
}
void _until(X* x) { 
	compile(x, get_xt(x, find_word(x, "(?BRANCH)"))); 
	_here(x); _minus(x); _comma(x); 
}
void _while(X* x) { _if(x); _swap(x); }
void _repeat(X* x) { _again(x); _then(x); }/* Commands that change compilation & interpretation settings */

void _decimal(X* x) { set(x, BASE, 10); }
void _hex(X* x) { set(x, BASE, 16); }
void _marker(X* x) { /* TODO */ }
void _base(X* x) { push(x, to_abs(x, BASE)); }

/* String operations */

void _count(X* x) { CELL a = pop(x); push(x, a + 1); push(x, cfetch(x, a)); }
void _fill(X* x) { 
	CHAR c = (CHAR)pop(x);
	CELL u = pop(x);
	CELL addr = pop(x);
	if (u > 0) {
		CELL i;
		for (i = 0; i < u; i++) {
			cstore(x, addr + i, c);
		}
	}
}
void _erase(X* x) { /* TODO */ }
void _cmove(X* x) { /* TODO */ }
void _cmove_up(X* x) { /* TODO */ }

void _hold(X* x) { 
	set(x, HLD, get(x, HLD) - 1);
	cstore(x, get(x, HLD), pop(x));
}

void _less_number_sign(X* x) {
	set(x, HLD, to_abs(x, here(x) + NBUF));
}
void _number_sign_greater(X* x) { 
	pop(x); pop(x);
	push(x, get(x, HLD));
	push(x, to_abs(x, here(x) + NBUF) - get(x, HLD));
}
/* Code adapted from lbForth */
void _number_sign(X* x) {
	CELL r;
	push(x, 0);
	push(x, get(x, BASE));
	_u_m_slash_mod(x);
	_to_r(x);
	push(x, get(x, BASE));
	_u_m_slash_mod(x);
	_r_from(x);
	_rot(x);
	r = pop(x);
	if (r > 9) r += 7;
	r += 48;
	push(x, r); _hold(x);
}
void _number_sign_s(X* x) {
	do {
		_number_sign(x);
		_two_dup(x);
		_or(x);
		_zero_equals(x);
	} while(pop(x) == 0);
}
void _sign(X* x) { 
	if (pop(x) < 0) {
		push(x, '-'); _hold(x);
	} 
}

/* System constants & facilities for generating ASCII values */

void _bl(X* x) { push(x, 32); }
void _char(X* x) {
	push(x, 32); _word(x);
	push(x, cfetch(x, pop(x) + 1));
}
void _bracket_char(X* x) { 
	push(x, 32); _word(x);
	literal(x, cfetch(x, pop(x) + 1));
}
void _false(X* x) { push(x, 0); }
void _true(X* x) { push(x, -1); }

/* Commands to define data structures */

void _variable(X* x) { _create(x); push(x, 0); _comma(x); }
void _constant(X* x) { 
	CELL tok, tlen, v = pop(x);
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	literal(x, v); 
	compile(x, get_xt(x, find_word(x, "EXIT")));
}
void _buffer_colon(X* x) { _create(x); _allot(x); }
void _value(X* x) { /* TODO */ }

/* More facilities for defining routines (compiling-mode only) */

void _if(X* x) { 
	compile(x, get_xt(x, find_word(x, "(?BRANCH)"))); 
	_here(x); comma(x, 0); 
}
void _then(X* x) { _here(x); _over(x); _minus(x); _swap(x); _store(x); }
void _else(X* x) { _ahead(x); _swap(x); _then(x); }
void _s_quote(X* x) { 
	CELL i;
	/* Parsing */
	CELL l = 0;
	CELL a = get(x, IBUF) + get(x, IPOS);
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != '"') {
		l++;
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, STATE) == 0) {
		/* TODO: Should change between SBUF1 and SBUF2 */
		for (i = 0; i < l; i++) {
			cset(x, here(x) + SBUF1 + i, cfetch(x, a + i));
		}
		push(x, to_abs(x, here(x) + SBUF1));
		push(x, l);
	} else {
		compile(x, get_xt(x, find_word(x, "(STRING)")));
		comma(x, l);
		for (i = a; i < (a + l); i++) ccomma(x, cfetch(x, i));
		align(x);
	}
	if (get(x, IPOS) < get(x, ILEN))
		set(x, IPOS, get(x, IPOS) + 1);
}
void _c_quote(X* x) { /* TODO */ }
void _left_bracket(X* x) { set(x, STATE, 0); }
void _right_bracket(X* x) { set(x, STATE, 1); }
void _quit(X* x) { /* TODO */ }
void _abort(X* x) { /* TODO */ }
void _abort_quote(X* x) { /* TODO */ }

/* Constructing compiler and interpreter system extensions */

void _cell_plus(X* x) { push(x, pop(x) + sCELL); }
void _char_plus(X* x) { push(x, pop(x) + 1); }
void _c_comma(X* x) { ccomma(x, pop(x)); }
void _comma(X* x) { comma(x, pop(x)); }
void _literal(X* x) { literal(x, pop(x)); }
void _aligned(X* x) { /* TODO */ }
void _align(X* x) { /* TODO */ }
void _tick(X* x) {
	CELL tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pick(x, 0));
	if (tlen == 0) { pop(x); return; }
	_find(x);
	pop(x);
	push(x, pop(x));
}
void _bracket_tick(X* x) { _tick(x); literal(x, pop(x)); }
void _to_body(X* x) { push(x, to_abs(x, pop(x) + 4*sCELL)); }
void _to(X* x) { /* TODO */ }
void _parse(X* x) {
	CHAR c = (CHAR)pop(x);
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	push(x, ibuf + ipos);
	while (ipos < ilen && cfetch(x, ibuf + ipos) != c) ipos++;
	push(x, ibuf + ipos - pick(x, 0));
	if (ipos < ilen) ipos++;
	set(x, IPOS, ipos);
}
void _state(X* x) { push(x, to_abs(x, STATE)); }
void _source_id(X* x) { /* TODO */ }
void _pad(X* x) { _here(x); push(x, PAD); _plus(x); }
void _restore_input(X* x) { /* TODO */ }
void _save_input(X* x) { /* TODO */ }

/* Comment-introducing operations */

void _backslash(X* x) { set(x, IPOS, get(x, ILEN)); }
void _paren(X* x) {
	/* TODO */
	/* ( should be able to work multiline if reading from file */
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != ')') {
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, IPOS) != get(x, ILEN))
		set(x, IPOS, get(x, IPOS) + 1);
}

/* Comparison operations */

void _greater_than(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) > a ? -1 : 0); 
}
void _not_equals(X* x) { CELL a = pop(x); push(x, pop(x) != a ? -1 : 0); }
void _zero_equals(X* x) { push(x, pop(x) == 0 ? -1 : 0); }
void _zero_greater_than(X* x) { push(x, pop(x) > 0 ? -1 : 0); }
void _zero_not_equals(X* x) { push(x, pop(x) != 0 ? -1 : 0); }
void _zero_less_than(X* x) { push(x, pop(x) < 0 ? -1 : 0); }
void _u_less_than(X* x) { 
	uCELL b = (uCELL)pop(x);
	uCELL a = (uCELL)pop(x);
	push(x, a < b ? -1 : 0);
}
void _u_greater_than(X* x) { 
	uCELL b = (uCELL)pop(x);
	uCELL a = (uCELL)pop(x);
	push(x, a > b ? -1 : 0);
}
void _within(X* x) { /* TODO */ }

/* Arithmetic and logical operations */

void _mod(X* x) { CELL a = pop(x); push(x, pop(x) % a); }
void _slash(X* x) { CELL a = pop(x); push(x, pop(x) / a); }
void _star_slash(X* x) { _star_slash_mod(x); _nip(x); }
void _slash_mod(X* x) { 
	CELL b = pop(x), a = pop(x);
	push(x, a%b);
	push(x, a/b);
}
void _one_plus(X* x) { push(x, pop(x) + 1); }
void _one_minus(X* x) { push(x, pop(x) - 1); }
void _two_star(X* x) { push(x, 2*pop(x)); }
void _abs(X* x) { CELL v = pop(x); push(x, v < 0 ? (0-v) : v); }
void _negate(X* x) { push(x, 0 - pop(x)); }
void _or(X* x) { CELL a = pop(x); push(x, pop(x) | a); }
void _xor(X* x) { CELL a = pop(x); push(x, pop(x) ^ a); }
void _min(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a < b ? a : b); }
void _max(X* x) { CELL a = pop(x); CELL b = pop(x); push(x, a > b ? a : b); }
void _plus_store(X* x) {
	CELL a = pop(x);
	CELL n = pop(x);
	store(x, a, fetch(x, a) + n);
}

void _star_slash_mod(X* x) { 
	_to_r(x); 
	_m_star(x); 
	_r_from(x); 
	_s_m_slash_rem(x); 
}

void _f_m_slash_mod(X* x) {
	CELL n1 = pop(x), d1_hi = pop(x), d1_lo = pop(x);

	uCELL q, r;
    
  /* Track signs */
  CELL d_neg = (d1_hi < 0);
  CELL n_neg = (n1 < 0);
    
  /* Use unsigned arithmetic for the algorithm */
  uCELL abs_n1 = n_neg ? (uCELL)-n1 : (uCELL)n1;
  uCELL abs_d_hi, abs_d_lo;
    
  /* Get absolute value of double-word dividend */
  if (d_neg) {
    /* Two's complement negation for double word */
    abs_d_lo = (uCELL)-d1_lo;
    abs_d_hi = (uCELL)(~d1_hi + (abs_d_lo == 0));
  } else {
    abs_d_hi = (uCELL)d1_hi;
    abs_d_lo = (uCELL)d1_lo;
  }
    
  /* Initialize quotient and remainder */
  q = 0;
  r = 0;
    
  /* Process 128 bits of the dividend (64 high bits + 64 low bits) 
  * using the standard long division algorithm
  */
    
  /* First the high bits */
  {
    CELL i;
    for (i = sCELL * CHAR_BIT - 1; i >= 0; i--) {
      /* Shift remainder left by 1 bit and bring in next bit of dividend */
      r = (r << 1) | ((abs_d_hi >> i) & 1);
            
      /* If remainder >= divisor, subtract and set quotient bit */
      if (r >= abs_n1) {
        r -= abs_n1;
        q |= (1UL << i);
      }
    }
  }
    
  /* Then the low bits */
  {
    CELL i;
    for (i = sCELL * CHAR_BIT - 1; i >= 0; i--) {
      /* Shift remainder left by 1 bit and bring in next bit of dividend */
      r = (r << 1) | ((abs_d_lo >> i) & 1);
            
      /* If remainder >= divisor, subtract and set quotient bit */
      if (r >= abs_n1) {
        r -= abs_n1;
        q |= (1UL << i);
      }
    }
  }
    
  /* Apply sign to results */
  {
    CELL quotient = (d_neg != n_neg) ? -(CELL)q : q;
    CELL remainder = d_neg ? -(CELL)r : r;
    
    /* Adjust for floored division semantics */
    if ((d_neg != n_neg) && remainder != 0) {
        quotient--;
        remainder += n1;
    }
    
    /* Push results back onto the stack */
		push(x, remainder);
		push(x, quotient);
  }
}

/* Memory-stack transfer operations */

void _two_store(X* x) {
	CELL a = pop(x);
	store(x, a, pop(x));
	store(x, a + sCELL, pop(x));
}
void _two_fetch(X* x) { 
	CELL a = pop(x);
	push(x, fetch(x, a + sCELL));
	push(x, fetch(x, a));
}

void bootstrap_core_wordset(X* x) {
	code(x, "2DROP", primitive(x, &_two_drop));
	code(x, "DUP", primitive(x, &_dup));
	code(x, "2DUP", primitive(x, &_two_dup));
	code(x, "?DUP", primitive(x, &_question_dup));
	code(x, "NIP", primitive(x, &_nip));
	code(x, "2OVER", primitive(x, &_two_over));
	code(x, "2>R", primitive(x, &_two_to_r));
	code(x, "2R>", primitive(x, &_two_r_from));
	code(x, "R@", primitive(x, &_r_fetch));
	code(x, "2R@", primitive(x, &_two_r_fetch));
	code(x, "ROLL", primitive(x, &_roll));
	code(x, "ROT", primitive(x, &_rot));
	code(x, "2SWAP", primitive(x, &_two_swap));
	code(x, "TUCK", primitive(x, &_tuck));
	code(x, ">NUMBER", primitive(x, &_to_number));
	code(x, "ENVIRONMENT?", primitive(x, &_environment_q));
	code(x, ".(", primitive(x, &_dot_paren)); _immediate(x);
	code(x, ".\"", primitive(x, &_dot_quote)); _immediate(x);
	code(x, "ACCEPT", primitive(x, &_accept));
	code(x, ".", primitive(x, &_dot));
	code(x, ".R", primitive(x, &_dot_r));
	code(x, "U.", primitive(x, &_u_dot));
	code(x, "U.R", primitive(x, &_u_dot_r));
	code(x, "DO", primitive(x, &_do)); _immediate(x);
	code(x, "?DO", primitive(x, &_question_do)); _immediate(x);
	code(x, "I", primitive(x, &_i));
	code(x, "J", primitive(x, &_j));
	code(x, "LEAVE", primitive(x, &__leave));
	code(x, "LOOP", primitive(x, &_loop)); _immediate(x);
	code(x, "+LOOP", primitive(x, &_plus_loop)); _immediate(x);
	code(x, "BASE", primitive(x, &_base));
	code(x, "DECIMAL", primitive(x, &_decimal));
	code(x, "HEX", primitive(x, &_hex));
	code(x, "MARKER", primitive(x, &_marker));
	code(x, "SPACE", primitive(x, &_space));
	code(x, "SPACES", primitive(x, &_spaces));
	code(x, "TYPE", primitive(x, &_type));
	code(x, "CR", primitive(x, &_cr));
	code(x, "COUNT", primitive(x, &_count));
	code(x, "ERASE", primitive(x, &_erase));
	code(x, "FILL", primitive(x, &_fill));
	code(x, "CMOVE", primitive(x, &_cmove));
	code(x, "CMOVE>", primitive(x, &_cmove_up));
	code(x, "HOLD", primitive(x, &_hold));
	code(x, "<#", primitive(x, &_less_number_sign));
	code(x, "#>", primitive(x, &_number_sign_greater));
	code(x, "#", primitive(x, &_number_sign));
	code(x, "#S", primitive(x, &_number_sign_s));
	code(x, "SIGN", primitive(x, &_sign));
	code(x, "BL", primitive(x, &_bl));
	code(x, "CHAR", primitive(x, &_char));
	code(x, "[CHAR]", primitive(x, &_bracket_char)); _immediate(x);
	code(x, "FALSE", primitive(x, &_false));
	code(x, "TRUE", primitive(x, &_true));
	code(x, "CONSTANT", primitive(x, &_constant));
	code(x, "VALUE", primitive(x, &_value));
	code(x, "VARIABLE", primitive(x, &_variable));
	code(x, "BUFFER:", primitive(x, &_buffer_colon));
	code(x, "BEGIN", primitive(x, &_begin)); _immediate(x);
	code(x, "AGAIN", primitive(x, &_again)); _immediate(x);
	code(x, "UNTIL", primitive(x, &_until)); _immediate(x);
	code(x, "WHILE", primitive(x, &_while)); _immediate(x);
	code(x, "REPEAT", primitive(x, &_repeat)); _immediate(x);
	code(x, "IF", primitive(x, &_if)); _immediate(x);
	code(x, "ELSE", primitive(x, &_else)); _immediate(x);
	code(x, "THEN", primitive(x, &_then)); _immediate(x);
	code(x, "[", primitive(x, &_left_bracket)); _immediate(x);
	code(x, "QUIT", primitive(x, &_quit));
	code(x, "]", primitive(x, &_right_bracket)); _immediate(x);
	code(x, "S\"", primitive(x, &_s_quote)); _immediate(x);
	code(x, "ABORT", primitive(x, &_abort));
	code(x, "ABORT\"", primitive(x, &_abort_quote));
	code(x, "C\"", primitive(x, &_c_quote));
	code(x, "ALIGN", primitive(x, &_align));
	code(x, "ALIGNED", primitive(x, &_aligned));
	code(x, "CELL+", primitive(x, &_cell_plus));
	code(x, "CHAR+", primitive(x, &_char_plus));
	code(x, "C,", primitive(x, &_c_comma));
	code(x, ",", primitive(x, &_comma));
	code(x, "[']", primitive(x, &_bracket_tick)); _immediate(x);
	code(x, "LITERAL", primitive(x, &_literal)); _immediate(x);
	code(x, ">BODY", primitive(x, &_to_body));
	code(x, "PARSE", primitive(x, &_parse));
	code(x, "TO", primitive(x, &_to));
	code(x, "STATE", primitive(x, &_state));
	code(x, "SOURCE-ID", primitive(x, &_source_id));
	code(x, "PAD", primitive(x, &_pad));
	code(x, "RESTORE-INPUT", primitive(x, &_restore_input));
	code(x, "SAVE-INPUT", primitive(x, &_save_input));
	code(x, "\\", primitive(x, &_backslash)); _immediate(x);
	code(x, "(", primitive(x, &_paren)); _immediate(x);
	code(x, ">", primitive(x, &_greater_than));
	code(x, "<>", primitive(x, &_not_equals));
	code(x, "U<", primitive(x, &_u_less_than));
	code(x, "U>", primitive(x, &_u_greater_than));
	code(x, "WITHIN", primitive(x, &_within));
	code(x, "0=", primitive(x, &_zero_equals));
	code(x, "0>", primitive(x, &_zero_greater_than));
	code(x, "0<", primitive(x, &_zero_less_than));
	code(x, "0<>", primitive(x, &_zero_not_equals));
	code(x, "ABS", primitive(x, &_abs));
	code(x, "FM/MOD", primitive(x, &_f_m_slash_mod));
	code(x, "MAX", primitive(x, &_max));
	code(x, "MIN", primitive(x, &_min));
	code(x, "MOD", primitive(x, &_mod));
	code(x, "/MOD", primitive(x, &_slash_mod));
	code(x, "NEGATE", primitive(x, &_negate));
	code(x, "1+", primitive(x, &_one_plus));
	code(x, "1-", primitive(x, &_one_minus));
	code(x, "OR", primitive(x, &_or));
	code(x, "+!", primitive(x, &_plus_store));
	code(x, "/", primitive(x, &_slash));
	code(x, "*/", primitive(x, &_star_slash));
	code(x, "2*", primitive(x, &_two_star));
	code(x, "XOR", primitive(x, &_xor));
	code(x, "2!", primitive(x, &_two_store));
	code(x, "2@", primitive(x, &_two_fetch));
	code(x, "*/MOD", primitive(x, &_star_slash_mod));
}
