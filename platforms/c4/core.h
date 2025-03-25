#include "sloth.h"

#include "getch.h"

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

#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANT				4

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v) { 
	store(x, to_abs(x, a), v); 
}

CELL get(X* x, CELL a) { 
	return fetch(x, to_abs(x, a)); 
}

void cset(X* x, CELL a, CHAR v) { 
	cstore(x, to_abs(x, a), v); 
}

CHAR cget(X* x, CELL a) { 
	return cfetch(x, to_abs(x, a)); 
}

/* Memory management */

CELL here(X* x) { 
	return get(x, HERE); 
}

void allot(X* x, CELL v) { 
	set(x, HERE, get(x, HERE) + v); 
}

CELL aligned(CELL a) { 
	return (a + (sCELL - 1)) & ~(sCELL - 1); 
}

void align(X* x) { 
	set(x, HERE, (get(x, HERE) + (sCELL - 1)) & ~(sCELL - 1)); 
}

/* Compilation */

void comma(X* x, CELL v) { 
	set(x, here(x), v); 
	allot(x, sCELL); 
}

void ccomma(X* x, CHAR v) { 
	set(x, here(x), v); 
	allot(x, sCHAR); 
}

void compile(X* x, CELL xt) { 
	comma(x, xt); 
}

/* Pre-definition */ CELL find_word(X*, char*);
/* Pre-definition */ CELL get_xt(X*, CELL);

void literal(X* x, CELL n) { 
	compile(x, get_xt(x, find_word(x, "(LIT)")));
	comma(x, n); 
}

/* Sum of double numbers */

/* D+ is used on >NUMBER although is defined in the optional */
/* Double-Number word set */

/* Code adapted from pForth */
void _d_plus(X* x) { 
	uCELL ah, al, bl, bh, sh, sl;
	bh = (uCELL)pop(x);
	bl = (uCELL)pop(x);
	ah = (uCELL)pop(x);
	al = (uCELL)pop(x);
	sh = 0;
	sl = al + bl;
	if (sl < bl) sh = 1;	/* carry */
	sh += ah + bh;
	push(x, sl);
	push(x, sh);
}

/* -- Debugging (this functions are not needed) ------ */

/* Printing the stack */

void _dot_s(X* x) { 
	CELL i;
	printf("<%ld> ", x->sp);
	for (i = 0; i < x->sp; i++) {
		printf("%ld ", x->s[i]);
	}
}

/* Pre-definition */ CELL get_xt(X*, CELL);
/* Pre-definition */ void _word(X*);
/* Pre-definition */ void _find(X*);

void _see(X* x) { 
	CELL tok, tlen, i, xt, op = 0;
	CELL q = 0;
	CELL EXIT, QUOTATION, LIT;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pick(x, 0));
	if (tlen == 0) { pop(x); return; }
	printf("NAME: %.*s\n", (int)tlen, (char*)tok);
	_find(x); 
	i = pop(x);
	printf("IMMEDIATE (1 = YES, -1 = NO): %ld\n", i);
	xt = pop(x);
	printf("XT: %ld\n", xt);
	if (xt > 0) {
		EXIT = get_xt(x, find_word(x, "EXIT"));
		QUOTATION = get_xt(x, find_word(x, "(QUOTATION)"));
		LIT = get_xt(x, find_word(x, "(LIT)"));
		do {
			op = fetch(x, to_abs(x, xt));
			xt += sCELL;
			printf("%ld ", op);
			if (op == EXIT && q == 0) {
				break;
			} else if (op == EXIT && q > 0) {
				q--;
			} else if (op == QUOTATION) {
				q++;
			} else if (op == LIT) {
				op = fetch(x, to_abs(x, xt));
				xt += sCELL;
				printf("%ld ", op);
			} 
		} while (1);
		printf("\n");
	} 
}

/* -- Headers ------------------------------------------ */

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags CHAR					@ NT + 3*sCELL */
/* Namelen CHAR				@ NT + 3*sCELL + sCHAR */
/* Name CHAR*namelen	@ NT + 3*sCELL + 2*sCHAR */

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = here(x); /* NT address */
	comma(x, get(x, LATEST)); /* Store link to latest */
	set(x, LATEST, w); /* Set NT as latest */
	comma(x, 0); /* Reserve space for XT */
	ccomma(x, 0); /* Flags (default flags: 0) */
	ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) ccomma(x, fetch(x, n + i)); /* Name */
	align(x); /* Align XT address */
	set(x, w + sCELL, here(x));
	return w;
}

CELL get_link(X* x, CELL w) { 
	return get(x, w); 
}

CELL get_xt(X* x, CELL w) { 
	return get(x, w + sCELL); 
}

void set_xt(X* x, CELL w, CELL xt) { 
	set(x, w + sCELL, xt); 
}

CHAR get_flags(X* x, CELL w) { 
	return cget(x, w + 2*sCELL); 
}

CELL has_flag(X* x, CELL w, CELL v) { 
	return get_flags(x, w) & v; 
}

CHAR get_namelen(X* x, CELL w) { 
	return cget(x, w + 2*sCELL + sCHAR); 
}

CELL get_name_addr(X* x, CELL w) { 
	return to_abs(x, w + 2*sCELL + 2*sCHAR); 
}

/* Setting flags */

void set_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) | v); 
}

void unset_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 2*sCELL, get_flags(x, w) & ~v); 
}

/* Findind words */

/* helper function to compare a string and a word's name */
/* without case sensitivity. */
int compare_without_case(X* x, CELL w, CELL t, CELL l) {
	int i;
	if (get_namelen(x, w) != l) return 0;
	for (i = 0; i < l; i++) {
		char a = cfetch(x, t + i);
		char b = cfetch(x, get_name_addr(x, w) + i);
		if (a >= 97 && a <= 122) a -= 32;
		if (b >= 97 && b <= 122) b -= 32;
		if (a != b) return 0;
	}
	return 1;
}

CELL find_word(X* x, char* name) {
	CHAR l = strlen(name);
	CELL a = (CELL)name;
	CELL w = get(x, LATEST);
	while (w != 0) {
		if (!has_flag(x, w, HIDDEN) && compare_without_case(x, w, a, l)) break;
		w = get_link(x, w);
	}
	return w;
}

/* -- Outer interpreter -------------------------------- */

void _word(X*);
void _find(X*);

/* INTERPRET is not an ANS word ??!! */
void _interpret(X* x) {
	CELL nt, flag, n;
	char* tok;
	int tlen;
	char buf[15]; char *endptr;
	while (get(x, IPOS) < get(x, ILEN)) {
		push(x, 32); _word(x);
		tok = (char*)(pick(x, 0) + sCHAR);
		tlen = cfetch(x, pick(x, 0));
		if (tlen == 0) { pop(x); return; }
		_find(x);
		if ((flag = pop(x)) != 0) {
			if (get(x, STATE) == 0
			|| (get(x, STATE) != 0 && flag == 1)) {
				eval(x, pop(x));	
			} else {
				compile(x, pop(x));
			}
		} else {
			CELL temp_base = get(x, BASE);
			pop(x);
			if (tlen == 3 && *tok == '\'' && (*(tok + 2)) == '\'') {
				/* Character literal */
				if (get(x, STATE) == 0)	push(x, *(tok + 1));
				else literal(x, *(tok + 1));
			} else {
				if (*tok == '#') {
					temp_base = 10;
					tlen--;
					tok++;
				}	else if (*tok == '$') {
					temp_base = 16;
					tlen--;
					tok++;
				} else if (*tok == '%') {
					temp_base = 2;
					tlen--;
					tok++;
				}
				strncpy(buf, tok, tlen);
				buf[tlen] = 0;
				n = strtol(buf, &endptr, temp_base);	
				if (*endptr == '\0') {
					if (get(x, STATE) == 0) push(x, n);
					else literal(x, n);
				} else {
					/* TODO Word not found, throw an exception? */
					printf("%.*s ?\n", tlen, tok);
				}
			}
		}
	}
}

/* Including files */

void _included(X* x) {
	FILE *f;
	char filename[1024];
	char linebuf[1024];

	CELL previbuf = get(x, IBUF);
	CELL previpos = get(x, IPOS);
	CELL previlen = get(x, ILEN);

	CELL prevsourceid = get(x, SOURCE_ID);

	CELL l = pop(x);
	CELL a = pop(x);

	strncpy(filename, (char*)a, (size_t)l);
	filename[l] = 0;

	f = fopen(filename, "r");

	if (f) {
		set(x, SOURCE_ID, (CELL)f);

		while (fgets(linebuf, 1024, f)) {
			printf(">>>> %s", linebuf);
			set(x, IBUF, (CELL)linebuf);
			set(x, IPOS, 0);
			set(x, ILEN, strlen(linebuf));

			_interpret(x);
		}

		set(x, SOURCE_ID, prevsourceid);

		fclose(f);
	} else {
		printf("ERROR: Can't open file (%.*s)\n", (int)l, (char*)a);
	}

	set(x, IBUF, previbuf);
	set(x, IPOS, previpos);
	set(x, ILEN, previlen);
}

/* -- Primitives -------------------------------------- */

void _exit_(X* x) { 
	x->ip = (x->rp > 0) ? rpop(x) : -1; 
}

/* Pushes the cell next to current IP to the stack */
void _lit(X* x) { 
	push(x, op(x)); 
}

/* Pushes current IP plus the value on the cell next */
/* to current IP to the stack */
void _rip(X* x) { 
	push(x, to_abs(x, x->ip) + op(x) - sCELL); 
}

/* Compiles top of the stack */
void _compile(X* x) { 
	compile(x, pop(x)); 
}

/* Advances IP by the value on the cell next to current IP */
void _branch(X* x) { 
	x->ip += op(x) - sCELL; 
}

/* If top of the stack is zero advances IP by the value on */
/* the cell next to current IP */
void _zbranch(X* x) { 
	x->ip += pop(x) == 0 ? (op(x) - sCELL) : sCELL; 
}

/* Pushes IP to the stack and advances IP by the aligned */
/* value of the cell next to current IP */
void _string(X* x) { 
	CELL l = op(x); 
	push(x, to_abs(x, x->ip)); 
	push(x, l); 
	x->ip = aligned(x->ip + l); 
}

void _quotation(X* x) { 
	CELL d = op(x); 
	push(x, x->ip); 
	x->ip += d; 
}

/* Helper compiled by DOES> that replaces the first EXIT */
/* compiled by CREATE on the new created word with a call */
/* to the code after the DOES> in the CREATE DOES> word */
void _do_does(X* x) {
	set(x, get_xt(x, get(x, LATEST)) + 2*sCELL, pop(x));
}

/* -- Quotations (not in ANS Forth yet) ---------------- */

void _start_quotation(X* x) {
	CELL s = get(x, STATE);
	set(x, STATE, s <= 0 ? s - 1 : s + 1);
	if (get(x, STATE) == -1) push(x, here(x) + 2*sCELL);
	push(x, get(x, LATESTXT));
	compile(x, get_xt(x, find_word(x, "(QUOTATION)")));
	push(x, to_abs(x, here(x)));
	comma(x, 0);
	set(x, LATESTXT, here(x));
}

void _end_quotation(X* x) {
	CELL s = get(x, STATE), a = pop(x);
	compile(x, get_xt(x, find_word(x, "EXIT")));
	store(x, a, to_abs(x, here(x)) - a - sCELL);
	set(x, LATESTXT, pop(x));
	set(x, STATE, s < 0 ? s + 1 : s - 1);
}

/* -- ANS Forth CORE words ----------------------------- */

/* Manipulating stack items */

void _drop(X* x) { 
	pop(x); 
}

void _two_drop(X* x) { 
	pop(x); 
	pop(x); 
}

void _dup(X* x) { 
	push(x, pick(x, 0)); 
}

void _two_dup(X* x) { 
	push(x, pick(x, 1)); 
	push(x, pick(x, 1)); 
}

void _question_dup(X* x) { 
	if (pick(x, 0) != 0) _dup(x); 
}

void _over(X* x) { 
	push(x, pick(x, 1)); 
}

void _two_over(X* x) { 
	push(x, pick(x, 3)); 
	push(x, pick(x, 3)); 
}

void _to_r(X* x) { 
	rpush(x, pop(x)); 
}

void _r_from(X* x) { 
	push(x, rpop(x)); 
}

void _r_fetch(X* x) { 
	push(x, rpick(x, 0)); 
}

void _rot(X* x) { 
	CELL c = pop(x); 
	CELL b = pop(x); 
	CELL a = pop(x); 
	push(x, b); 
	push(x, c); 
	push(x, a); 
}

void _swap(X* x) { 
	CELL b = pop(x); 
	CELL a = pop(x); 
	push(x, b); 
	push(x, a); 
}

void _two_swap(X* x) { 
	CELL d = pop(x); 
	CELL c = pop(x); 
	CELL b = pop(x); 
	CELL a = pop(x); 
	push(x, c); 
	push(x, d); 
	push(x, a); 
	push(x, b); 
}

/* Arithmetic and logical operations */

void _abs(X* x) { 
	CELL v = pop(x); push(x, v < 0 ? (0-v) : v); 
}

void _and(X* x) { 
	CELL v = pop(x); push(x, pop(x) & v); 
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

void _invert(X* x) { 
	push(x, ~pop(x)); 
}

void _l_shift(X* x) { 
	CELL n = pop(x); 
	push(x, pop(x) << n); 
}

/* Code for _m_star has been created by claude.ai */
void _m_star(X* x) {
	CELL b = pop(x), a = pop(x), high, low;

	/* Convert the signed 64-bit integers to unsigned */
	/* for bit manipulation */
	uCELL ua = (uCELL)a;
	uCELL ub = (uCELL)b;

	/* Compute the full 128-bit product using 32 bit pieces */
	uCELL a_low = ua & hCELL_MASK;
	uCELL a_high = ua >> hCELL_BITS;
	uCELL b_low = ub & hCELL_MASK;
	uCELL b_high = ub >> hCELL_BITS;

	/* Multiply the components */
	uCELL low_low = a_low * b_low;
	uCELL low_high = a_low * b_high;
	uCELL high_low = a_high * b_low;
	uCELL high_high = a_high * b_high;

	/* Combine the partial product */
	uCELL carry = ((low_low >> hCELL_BITS) + (low_high & hCELL_MASK) + (high_low & hCELL_MASK)) >> hCELL_BITS;

	/* Calculate the low 64 bits of the result */
	low = (uCELL)(low_low + (low_high << hCELL_BITS) + (high_low << hCELL_BITS));

	/* Calculate the high 64 bits of the result */
	high = (uCELL)(high_high + (low_high >> hCELL_BITS) + (high_low >> hCELL_BITS) + carry);

	/* Adjust for sign */
	if (a < 0) high -= b;
	if (b < 0) high -= a;

	push(x, low);
	push(x, high);
}

void _max(X* x) { 
	CELL b = pop(x); 
	CELL a = pop(x); 
	push(x, a > b ? a : b);
}

void _min(X* x) { 
	CELL b = pop(x); 
	CELL a = pop(x); 
	push(x, a < b ? a : b); 
}

void _minus(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) - a); 
}

void _mod(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) % a); 
}

void _slash_mod(X* x) { 
	CELL b = pop(x), a = pop(x);
	push(x, a%b);
	push(x, a/b);
}

void _negate(X* x) { 
	push(x, 0 - pop(x)); 
}

void _one_plus(X* x) { 
	push(x, pop(x) + 1); 
}

void _one_minus(X* x) { 
	push(x, pop(x) - 1); 
}

void _or(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) | a); 
}

void _plus(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) + a); 
}

void _plus_store(X* x) {
	CELL a = pop(x);
	CELL n = pop(x);
	store(x, a, fetch(x, a) + n);
}

void _r_shift(X* x) { 
	CELL n = pop(x); 
	push(x, ((uCELL)pop(x)) >> n); 
}

void _slash(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) / a); 
}

void _s_m_slash_rem(X* x) {
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
		/* For symmetric division (SM/REM):
		 * 1. Quotient sign depends on operand signs (like normal division)
		 * 2. Remainder has the same sign as the dividend
		 * 3. No adjustment needed - we just truncate toward zero
		 */
		CELL quotient = (d_neg != n_neg) ? -(CELL)q : q;
		CELL remainder = d_neg ? -(CELL)r : r;
		
		/* Push results back onto the stack */
		push(x, remainder);
		push(x, quotient);
	}
}

void _star_slash_mod(X* x) { 
	_to_r(x); 
	_m_star(x); 
	_r_from(x); 
	_s_m_slash_rem(x); 
}

void _star(X* x) { 
	CELL b = pop(x); 
	push(x, pop(x) * b); 
}

void _star_slash(X* x) { 
	_star_slash_mod(x); 
	_swap(x);
	_drop(x);
}

void _two_star(X* x) { 
	push(x, 2*pop(x)); 
}

void _two_slash(X* x) { 
	push(x, pop(x) >> 1); 
}

void _u_m_star(X* x) {
	uCELL b = (uCELL)pop(x), a = (uCELL)pop(x), high, low;

	/* Split each 64-bit integer into 32-bit pieces for multiplication */
	uCELL a_low = a & hCELL_MASK;
	uCELL a_high = a >> hCELL_BITS;
	uCELL b_low = b & hCELL_MASK;
	uCELL b_high = b >> hCELL_BITS;
	
	/* Multiply the 32-bit components */
	uCELL low_low = a_low * b_low;
	uCELL low_high = a_low * b_high;
	uCELL high_low = a_high * b_low;
	uCELL high_high = a_high * b_high;

	uCELL carry; /* Pre-definition */

	/* Intermediate values for calculating the carries */
	uCELL mid = low_low >> hCELL_BITS;
	mid += low_high & hCELL_MASK;
	mid += high_low & hCELL_MASK;
	
	/* Calculate carry for the high part */
	carry = mid >> hCELL_BITS;
	
	/* Calculate the low 64 bits of the result */
	low = (mid << hCELL_BITS) | (low_low & hCELL_MASK);
	
	/* Calculate the high 64 bits of the result */
	high = high_high + (low_high >> hCELL_BITS) + (high_low >> hCELL_BITS) + carry;

	push(x, low);
	push(x, high);
}

/* UM/MOD code taken from pForth */

#define DULT(du1l,du1h,du2l,du2h) ( (du2h<du1h) ? 0 : ( (du2h==du1h) ? (du1l<du2l) : 1) )

void _u_m_slash_mod(X* x) {
	uCELL ah, al, q, di, bl, bh, sl, sh;
	bh = (uCELL)pop(x);
	bl = 0;
	ah = (uCELL)pop(x);
	al = (uCELL)pop(x);
	q = 0;
	for( di=0; di<CELL_BITS; di++ )
	{
	    if( !DULT(al,ah,bl,bh) )
	    {
	        sh = 0;
	        sl = al - bl;
	        if( al < bl ) sh = 1; /* Borrow */
	        sh = ah - bh - sh;
	        ah = sh;
	        al = sl;
	        q |= 1;
	    }
	    q = q << 1;
	    bl = (bl >> 1) | (bh << (CELL_BITS-1));
	    bh = bh >> 1;
	}
	if( !DULT(al,ah,bl,bh) )
	{
	    al = al - bl;
	    q |= 1;
	}
	push(x, al); /* rem */
	push(x, q);
}

void _xor(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) ^ a); 
}

/* Memory-stack transfer operations */

void _c_fetch(X* x) { 
	push(x, cfetch(x, pop(x))); 
}

void _c_store(X* x) { 
	CELL a = pop(x); 
	cstore(x, a, pop(x)); 
}

void _fetch(X* x) { 
	push(x, fetch(x, pop(x))); 
}

void _two_fetch(X* x) { 
	CELL a = pop(x);
	push(x, fetch(x, a + sCELL));
	push(x, fetch(x, a));
}

void _store(X* x) { 
	CELL a = pop(x); 
	store(x, a, pop(x)); 
}

void _two_store(X* x) {
	CELL a = pop(x);
	store(x, a, pop(x));
	store(x, a + sCELL, pop(x));
}

/* Comparison operations */

void _equals(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) == a ? -1 : 0); 
}

void _greater_than(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) > a ? -1 : 0); 
}

void _less_than(X* x) { 
	CELL a = pop(x); 
	push(x, pop(x) < a ? -1 : 0); 
}

void _u_less_than(X* x) { 
	uCELL b = (uCELL)pop(x);
	uCELL a = (uCELL)pop(x);
	push(x, a < b ? -1 : 0);
}

void _zero_equals(X* x) { 
	push(x, pop(x) == 0 ? -1 : 0); 
}

void _zero_less_than(X* x) { 
	push(x, pop(x) < 0 ? -1 : 0); 
}

/* Commands that can help you start or end work sessions */

void _environment_query(X* x) { 
	CELL u = pop(x);
	CHAR *s = (CHAR *)pop(x);
	/*
	if (compare_no_case(x, s, u, "/COUNTED-STRING")) {
		push(x, COUNTED_STRING_MAX_SIZE);
	} else if (compare_no_case(x, s, u, "/HOLD")) {
		push(x, NUMERIC_OUTPUT_STRING_BUFFER_SIZE);
	} else if (compare_no_case(x, s, u, "/PAD")) {
		push(x, SCRATCH_AREA_SIZE);
	} else if (compare_no_case(x, s, u, "ADDRESS-UNIT-BITS")) {
		push(x, BITS_PER_ADDRESS_UNIT);
	} else if (compare_no_case(x, s, u, "FLOORED")) {
		push(x, FLOORED_DIVISION_AS_DEFAULT);
	} else if (compare_no_case(x, s, u, "MAX-CHAR")) {
		push(x, MAX_VALUE_FOR_CHAR);
	} else if (compare_no_case(x, s, u, "MAX-D")) {
		push(x, LARGEST_USABLE_DOUBLE_NUMBER);
	} else if (compare_no_case(x, s, u, "MAX-N")) {
		push(x, LARGEST_USABLE_SIGNED_INTEGER);
	} else if (compare_no_case(x, s, u, "MAX-U")) {
		push(x, LARGEST_USABLE_UNSIGNED_INTEGER);
	} else if (compare_no_case(x, s, u, "MAX-UD")) {
		push(x, LARGEST_USABLE_UNSIGNED_DOUBLE_NUMBER);
	} else if (compare_no_case(x, s, u, "RETURN-STACK-CELLS")) {
		push(x, RETURN_STACK_SIZE);
	} else if (compare_no_case(x, s, u, "STACK-CELLS")) {
		push(x, STACK_SIZE);
	}
	*/
}

/* Commands to inspect memory, debug & view code */

void _depth(X* x) {
	push(x, x->sp);
}

/* Commands that change compilation & interpretation settings */

void _base(X* x) { 
	push(x, to_abs(x, BASE)); 
}

void _decimal(X* x) { 
	set(x, BASE, 10); 
}

/* Source code preprocessing, interpreting & auditing commands */

/* Comment-introducing operations */

void _paren(X* x) {
	while (get(x, IPOS) < get(x, ILEN)
	&& cfetch(x, get(x, IBUF) + get(x, IPOS)) != ')') {
		set(x, IPOS, get(x, IPOS) + 1);
	}
	if (get(x, IPOS) != get(x, ILEN))
		set(x, IPOS, get(x, IPOS) + 1);
}

/* String operations */

void _count(X* x) { 
	CELL a = pop(x); 
	push(x, a + 1); 
	push(x, cfetch(x, a)); 
}

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

void _hold(X* x) { 
	set(x, HLD, get(x, HLD) - 1);
	cstore(x, get(x, HLD), pop(x));
}

void _move(X* x) {
	CELL u = pop(x);
	CELL addr2 = pop(x);
	CELL addr1 = pop(x);
	CELL i;
	if (addr1 >= addr2) {
		for (i = 0; i < u; i++) {
			cstore(x, addr2 + i, cfetch(x, addr1 + i));
		}
	} else {
		for (i = u - 1; i >= 0; i--) {
			cstore(x, addr2 + i, cfetch(x, addr1 + i));
		}
	}
}

/* Helper function for _to_number */
/* TODO: This function should be adapted to real C */
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
	push(x, '0'); 
	_minus(x);
	_dup(x); 
	_r_from(x); 
	_less_than(x);
	if (pop(x)) {
		_dup(x); 
		_one_plus(x); 
		push(x, pop(x) > 0 ? -1 : 0);
		if (pop(x)) {
			_swap(x);
			_drop(x);
			push(x, -1);
		} else {
			_drop(x); 
			push(x, 0);
		}
	} else {
		_drop(x); 
		push(x, 0);
	}
}

/* Code adapted from pForth Forth code */
void _to_number(X* x) {
	_to_r(x);
	do { /* BEGIN */
		_r_fetch(x); 
		push(x, pop(x) > 0 ? -1 : 0);
		if (pop(x)) {
			_dup(x); _c_fetch(x); _base(x); _fetch(x);
			_digit(x);
			if (pop(x)) {
				push(x, -1);
			} else {
				_drop(x); 
				push(x, 0);
			}
		} else {
			push(x, 0);
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

/* Disk input/output operations using files or block buffers */

/* More input/output operations */

void _emit(X* x) { 
	printf("%c", (CHAR)pop(x)); 
}

void _key(X* x) { 
	push(x, getch()); 
}

/* TODO: It would be interesting to manage at least backspace */
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

void _cr(X* x) { 
	printf("\n"); 
}

void _dot(X* x) { 
	printf("%ld ", pop(x)); 
}

void _dot_quote(X* x) { 
	CELL u, addr, i;
	push(x, '"'); 
	_word(x);
	addr = pick(x, 0) + sCHAR;
	u = cfetch(x, pop(x));
	/*
	_parse(x); 
	u = pop(x);
	addr = pop(x);
	*/
	compile(x, get_xt(x, find_word(x, "(STRING)")));
	comma(x, u);
	for (i = 0; i < u; i++) {
		ccomma(x, cfetch(x, addr + i));
	}
	align(x);
	compile(x, get_xt(x, find_word(x, "TYPE")));
}

void _space(X* x) { 
	printf(" "); 
}

void _spaces(X* x) { 
	CELL i, u = pop(x); 
	for (i = 0; i < u; i++) printf(" ");
}

void _type(X* x) { 
	CELL l = pop(x); 
	CELL a = pop(x);
	/* Alternative implementation: printf("%.*s", (int)l, (char*)a); */
	CELL i;
	CHAR c;
	for (i = a; i < a + l; i++) {
		c = cfetch(x, i);
		if (c >= 32 && c <= 126) printf("%c", c);
	}
}

void _u_dot(X* x) { 
	printf("%lu ", (uCELL)pop(x)); 
}

/* Number-type conversion operators */

void _s_to_d(X* x) { 
	push(x, pick(x, 0) < 0 ? -1 : 0); 
}

/* Constructing compiler and interpreter system extensions */

void _align(X* x) { 
	align(x); 
}

void _aligned(X* x) { 
	push(x, aligned(pop(x)));
}

void _allot(X* x) { 
	allot(x, pop(x)); 
}

void _to_body(X* x) { 
	push(x, to_abs(x, pop(x) + 4*sCELL)); 
}

void _c_comma(X* x) { 
	ccomma(x, pop(x)); 
}

void _cell_plus(X* x) { 
	push(x, pop(x) + sCELL); 
}

void _cells(X* x) { 
	push(x, pop(x) * sCELL); 
}

void _char_plus(X* x) { 
	push(x, pop(x) + 1); 
}

void _chars(X* x) {
	/* Nothing to do */
}

void _comma(X* x) { 
	comma(x, pop(x)); 
}

/* CREATE parses the next word in the input buffer, creates */
/* a new header for it and then compiles some code. */
/* The compiled code is 4 CELLS long and has a RIP instruction */
/* a displacement of 4 CELLS and to EXIT instructions. */
/* The RIP instruction will load the address after the last */
/* EXIT instruction onto the stack. That's the address used */
/* by created words. */
/* The first EXIT instruction exists to be replaced with a */
/* call if CREATE DOES> is used. */
/* The last EXIT is the real end of the word. */
void _create(X* x) {
	CELL tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	compile(x, get_xt(x, find_word(x, "(RIP)")));
	compile(x, 4*sCELL); 
	compile(x, get_xt(x, find_word(x, "EXIT")));
	compile(x, get_xt(x, find_word(x, "EXIT")));
}

void _does(X* x) {
	literal(x, here(x) + 4*sCELL);
	compile(x, get_xt(x, find_word(x, "(DOES)")));
	compile(x, get_xt(x, find_word(x, "EXIT")));
}

void _evaluate(X* x) {
	CELL l = pop(x), a = pop(x);

	CELL previbuf = get(x, IBUF);
	CELL previpos = get(x, IPOS);
	CELL previlen = get(x, ILEN);

	CELL prevsourceid = get(x, SOURCE_ID);

	set(x, SOURCE_ID, -1);

	set(x, IBUF, a);
	set(x, IPOS, 0);
	set(x, ILEN, l);

	_interpret(x);

	set(x, SOURCE_ID, prevsourceid);

	set(x, IBUF, previbuf);
	set(x, IPOS, previpos);
	set(x, ILEN, previlen);
}

void _execute(X* x) { 
	eval(x, pop(x)); 
}

void _find(X* x) {
	/* let's get the address and length from the counted */
	/* string on the stack. */
	CELL cstring = pop(x);
	CHAR l = cfetch(x, cstring);
	CELL a = cstring + sCHAR;
	/* let's find the word, starting from latest */
	CELL w = get(x, LATEST);
	while (w != 0) {
		if (!has_flag(x, w, HIDDEN) && compare_without_case(x, w, a, l)) break;
		w = get_link(x, w);
	}
	if (w == 0) {
		push(x, cstring);
		push(x, 0);
	} else if (has_flag(x, w, IMMEDIATE)) {
		push(x, get_xt(x, w));
		push(x, 1);
	} else {
		push(x, get_xt(x, w));
		push(x, -1);
	}
}

void _here(X* x) { 
	push(x, to_abs(x, here(x))); 
}

void _immediate(X* x) { 
	set_flag(x, get(x, LATEST), IMMEDIATE); 
}

void _to_in(X* x) { 
	push(x, to_abs(x, IPOS)); 
}

void _literal(X* x) { 
	literal(x, pop(x)); 
}

void _postpone(X* x) { 
	CELL i, xt, tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pick(x, 0));
	if (tlen == 0) { pop(x); return; }
	_find(x); 
	i = pop(x);
	xt = pop(x);
	if (i == 0) { 
		return;
	} else if (i == -1) {
		/* Compile the compilation of the normal word */
		literal(x, xt);
		compile(x, get_xt(x, find_word(x, "(COMPILE)")));
	} else if (i == 1) {
		/* Compile the immediate word */
		compile(x, xt);
	}
}

void _refill(X* x) {
	char linebuf[1024];
	switch (get(x, SOURCE_ID)) {
	case -1: 
		push(x, 0);
		break;
	case 0:
		push(x, get(x, IBUF)); push(x, 80);
		_accept(x);
		set(x, ILEN, pop(x));
		set(x, IPOS, 0);
		push(x, -1); 
		break;
	default: 
		if (fgets(linebuf, 1024, (FILE *)get(x, SOURCE_ID))) {
			set(x, IBUF, (CELL)linebuf);
			set(x, IPOS, 0);
			set(x, ILEN, strlen(linebuf));
			push(x, -1);
		} else {
			push(x, 0);
		}
		break;
	}	
}

void _source(X* x) { 
	push(x, get(x, IBUF)); 
	push(x, get(x, ILEN)); 
}

void _state(X* x) { 
	push(x, to_abs(x, STATE)); 
}

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

void _bracket_tick(X* x) { 
	_tick(x); 
	literal(x, pop(x)); 
}

void _word(X* x) {
	/* The region to store WORD counted strings starts */
	/* at here + CBUF. */
	CHAR c = (CHAR)pop(x);
	CELL ibuf = get(x, IBUF);
	CELL ilen = get(x, ILEN);
	CELL ipos = get(x, IPOS);
	CELL start, end, i;
	/* First, ignore c until not c is found */
	/* The Forth Standard says that if the control character is */
	/* the space (hex 20) then control characters may be treated */
	/* as delimiters. */
	if (c == 32) {
		while (ipos < ilen && cfetch(x, ibuf + ipos) <= c) ipos++;
	} else {
		while (ipos < ilen && cfetch(x, ibuf + ipos) == c) ipos++;
	}
	start = ibuf + ipos;
	/* Next, continue parsing until c is found again */
	if (c == 32) {
		while (ipos < ilen && cfetch(x, ibuf + ipos) > c) ipos++;
	} else {
		while (ipos < ilen && cfetch(x, ibuf + ipos) != c) ipos++;
	}
	end = ibuf + ipos;	
	/* Now, copy it to the counted string buffer */
	cstore(x, to_abs(x, here(x) + CBUF), end - start);
	for (i = 0; i < (end - start); i++) {
		cstore(x, to_abs(x, here(x) + CBUF + sCHAR + i*sCHAR), cfetch(x, start + i*sCHAR));
	}
	push(x, to_abs(x, here(x) + CBUF));
	/* If we are not at the end of the input buffer, */
	/* skip c after the word, but its not part of the counted */
	/* string */
	if (ipos < ilen) ipos++;
	set(x, IPOS, ipos);
}

/* Commands to define data structures */

void _constant(X* x) { 
	CELL tok, tlen, v = pop(x);
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	literal(x, v); 
	compile(x, get_xt(x, find_word(x, "EXIT")));
}

void _variable(X* x) { 
	_create(x); 
	push(x, 0); 
	_comma(x); 
}

/* System constants & facilities for generating ASCII values */

void _bl(X* x) { 
	push(x, 32); 
}

void _char(X* x) {
	push(x, 32); _word(x);
	push(x, cfetch(x, pop(x) + 1));
}

void _bracket_char(X* x) { 
	push(x, 32); _word(x);
	literal(x, cfetch(x, pop(x) + 1));
}

/* More facilities for defining routines (compiling-mode only) */

void _abort(X* x) { 
	/* TODO */ 
}

void _abort_quote(X* x) { 
	/* TODO */ 
}

void _colon(X* x) {
	CELL tok, tlen;
	push(x, 32); _word(x);
	tok = pick(x, 0) + sCHAR;
	tlen = cfetch(x, pop(x));
	header(x, tok, tlen);
	set(x, LATESTXT, get_xt(x, get(x, LATEST)));
	set_flag(x, get(x, LATEST), HIDDEN);
	set(x, STATE, 1);
}

void _semicolon(X* x) {
	compile(x, get_xt(x, find_word(x, "EXIT")));
	set(x, STATE, 0);
	/* Don't change flags for nonames */
	if (get_xt(x, get(x, LATEST)) == get(x, LATESTXT))
		unset_flag(x, get(x, LATEST), HIDDEN);
}

void _if(X* x) { 
	compile(x, get_xt(x, find_word(x, "(ZBRANCH)")));
	_here(x); 
	comma(x, 0); 
}

void _then(X* x) { 
	_here(x); 
	_over(x); 
	_minus(x); 
	_swap(x); 
	_store(x); 
}

void _else(X* x) { 
	/* AHEAD (Not defined for Core) */
	compile(x, get_xt(x, find_word(x, "(BRANCH")));
	_here(x); 
	comma(x, 0);
	/* \AHEAD */
	_swap(x); 
	_then(x); 
}

void _left_bracket(X* x) { 
	set(x, STATE, 0); 
}

void _quit(X* x) { 
	/* TODO */ 
}

void _recurse(X* x) { 
	compile(x, get(x, LATESTXT)); 
}

void _right_bracket(X* x) { 
	set(x, STATE, 1); 
}

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

/* Loop helpers */

void ipush(X* x) { 
	rpush(x, get(x, KX));
	set(x, KX, get(x, JX));
	set(x, JX, get(x, IX));
	set(x, LX, 0);
}

void ipop(X* x) { 
	set(x, LX, 0);
	set(x, IX, get(x, JX));
	set(x, JX, get(x, KX));
	set(x, KX, rpop(x));
}

/* Algorithm for doloop taken from pForth */
/* (pf_inner.c case ID_PLUS_LOOP) */
void _doloop(X* x) {
	CELL q, do_first_loop, l, o, d;
	ipush(x);
	q = pop(x);
	do_first_loop = pop(x);
	set(x, IX, pop(x));
	l = pop(x);

	o = get(x, IX) - l;
	d = 0;

	/* First iteration is executed always on a DO */
	if (do_first_loop == 1) {
		eval(x, q);
		if (get(x, LX) == 0) {
			d = pop(x);
			o = get(x, IX) - l;
			set(x, IX, get(x, IX) + d);
		}
	}

	if (!(do_first_loop == 0 && o == 0)) {
		while (((o ^ (o + d)) & (o ^ d)) >= 0 && get(x, LX) == 0) {
			eval(x, q);
			if (get(x, LX) == 0) { /* Avoid pop if we're leaving */
				d = pop(x);
				o = get(x, IX) - l;
				set(x, IX, get(x, IX) + d);
			}
		}
	}

	if (get(x, LX) == 0 || get(x, LX) == 1) { 
		/* Leave case */
		ipop(x);
	} else if (get(x, LX) < 0) {
		/* Unloop case */
		set(x, LX, get(x, LX) + 1);
		rpop(x);
		_exit_(x);
	}
}

/* Forming definite loops */

void _do(X* x) { 
	literal(x, 1); 
	_start_quotation(x); 
}

void _i(X* x) { 
	push(x, get(x, IX)); 
}

void _j(X* x) { 
	push(x, get(x, JX)); 
}

void _leave(X* x) { 
	set(x, LX, 1); 
	_exit_(x); 
}

void _unloop(X* x) { 
	set(x, LX, get(x, LX) - 1);
	if (get(x, LX) == -1) {
		set(x, IX, get(x, JX));
		set(x, JX, get(x, KX));
		set(x, KX, rpick(x, 1));
	} else if (get(x, LX) == -2) {
		set(x, IX, get(x, JX));
		set(x, JX, get(x, KX));
		set(x, KX, rpick(x, 3));
	}
}

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

void _begin(X* x) { 
	_here(x); 
}

void _until(X* x) { 
	compile(x, get_xt(x, find_word(x, "(ZBRANCH)")));
	_here(x); 
	_minus(x); 
	_comma(x); 
}

void _while(X* x) { 
	_if(x); 
	_swap(x); 
}

void _repeat(X* x) { 
	/* AGAIN (not defined in CORE) */
	compile(x, get_xt(x, find_word(x, "(BRANCH)")));
	_here(x); 
	_minus(x); 
	_comma(x);
	/* \AGAIN */
	_then(x); 
}


