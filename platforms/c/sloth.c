#include "sloth.h"

/* C89 NaN/Inf detection (IEEE 754 assumed) */
#define SLOTH_F_NAN()    (0.0 / 0.0)
#define SLOTH_F_ISNAN(x) ((x) != (x))
#define SLOTH_F_ISINF(x) (!SLOTH_F_ISNAN(x) && (x) - (x) != 0.0)

/* -- getch simple multiplatform definition and implementation  */

#ifndef WINDOWS
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

/* -- Data and return stack ---------------------------- */

#define TOS(x) x->s[x->sp - 1]

void sloth_push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL sloth_pop(X* x) { x->sp--; return x->s[x->sp]; }
void sloth_rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL sloth_rpop(X* x) { x->rp--; return x->r[x->rp]; }

void sloth_dup_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_push(x, a); 
	sloth_push(x, a); 
}
void sloth_drop_(X* x) { sloth_pop(x); }
void sloth_over_(X* x) { 
	CELL b = sloth_pop(x); 
	CELL a = sloth_pop(x); 
	sloth_push(x, a); 
	sloth_push(x, b); 
	sloth_push(x, a);
}
void sloth_swap_(X* x) { 
	if (x->sp < 2) {
		sloth_throw(x, SLOTH_STACK_UNDERFLOW);
	} else {
		CELL a = sloth_pop(x); 
		CELL b = sloth_pop(x);
		sloth_push(x, a);
		sloth_push(x, b);
	}
}
/*
void sloth_swap_(X* x) { 
	CELL b = sloth_pop(x); 
	CELL a = sloth_pop(x); 
	sloth_push(x, b); 
	sloth_push(x, a); 
}
*/
void sloth_to_r_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_rpush(x, a); 
}
void sloth_r_from_(X* x) { 
	CELL a = sloth_rpop(x); 
	sloth_push(x, a);
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

/* -- Floating point stack ----------------------------- */

void sloth_f_push(X* x, FCELL v) { x->f[x->fp] = v; x->fp++; }
FCELL sloth_f_pop(X* x) { x->fp--; return x->f[x->fp]; }
FCELL sloth_f_pick(X* x, CELL a) { return x->f[x->fp - a - 1]; }

#endif

/* -- Memory management -------------------------------- */

/* Transform from relative to absolute addresses. */
CELL sloth_to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL sloth_to_rel(X* x, CELL a) { return a - x->d; }

/* STORE/FETCH/CSTORE/cfetch work on absolute address units, */
/* not just inside SLOTH dictionary (memory block). */
void sloth_b_store(X* x, CELL a, BYTE_ v) { 
	(void)x;
	*((BYTE_*)a) = v; 
}
BYTE_ sloth_b_fetch(X* x, CELL a) { 
	(void)x;
	return *((BYTE_*)a); 
}
void sloth_c_store(X* x, CELL a, uCHAR v) { 
	(void)x;
	*((uCHAR*)a) = v; 
}
uCHAR sloth_c_fetch(X* x, CELL a) { 
	(void)x;
	return *((uCHAR*)a); 
}
void sloth_store(X* x, CELL a, CELL v) { 
	(void)x;
	*((CELL*)a) = v; 
}
CELL sloth_fetch(X* x, CELL a) { 
	(void)x;
	return *((CELL*)a); 
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

void 
sloth_f_store(X* x, CELL a, FCELL v) { 
	(void)x;
	*((FCELL*)a) = v; 
}
FCELL 
sloth_f_fetch(X* x, CELL a) { 
	(void)x;
	return *((FCELL*)a); 
}
void 
sloth_s_f_store(X* x, CELL a, SFCELL v) { 
	(void)x;
	*((SFCELL*)a) = v; 
}
SFCELL 
sloth_s_f_fetch(X* x, CELL a) { 
	(void)x;
	return *((SFCELL*)a); 
}
void 
sloth_d_f_store(X* x, CELL a, DFCELL v) { 
	(void)x;
	*((DFCELL*)a) = v; 
}
DFCELL 
sloth_d_f_fetch(X* x, CELL a) { 
	(void)x;
	return *((DFCELL*)a); 
}

#endif

/* Setting and getting cells in memory */

void sloth_set(X* x, CELL a, CELL v) {
	sloth_store(x, sloth_to_abs(x, a), v); 
}
CELL sloth_get(X* x, CELL a) { 
	return sloth_fetch(x, sloth_to_abs(x, a)); 
}
void sloth_user_set(X* x, CELL a, CELL v) { 
	sloth_store(x, x->u + a, v); 
}
CELL sloth_user_get(X* x, CELL a) { 
	return sloth_fetch(x, x->u + a);
}

/* Working with the HERE pointer */

CELL sloth_here(X* x) { return sloth_get(x, SLOTH_HERE); }
void sloth_allot(X* x, CELL v) { 
	sloth_set(x, SLOTH_HERE, sloth_here(x) + v); 
}
CELL sloth_aligned(CELL a) { return ALIGNED(a, sCELL); }

void sloth_here_(X* x) { sloth_push(x, sloth_here(x)); }
void sloth_allot_(X* x) { sloth_allot(x, sloth_pop(x)); }
void sloth_align_(X* x) { 
	sloth_set(x, SLOTH_HERE, ALIGNED(sloth_here(x), sCELL)); 
}

/* Moving data from stack to dictionary and viceversa */

void sloth_c_fetch_(X* x) { 
	sloth_push(x, sloth_c_fetch(x, sloth_pop(x))); 
}
void sloth_c_store_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_c_store(x, a, sloth_pop(x)); 
}
void sloth_fetch_(X* x) { 
	sloth_push(x, sloth_fetch(x, sloth_pop(x))); 
}
void sloth_store_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_store(x, a, sloth_pop(x)); 
}

void sloth_cells_(X* x) { sloth_push(x, sloth_pop(x)*sCELL); }
void sloth_chars_(X* x) { /* Does nothing */ (void)x; }

/* Inspecting memory */

void sloth_unused_(X* x) {
	sloth_push(x, x->d + x->dz - sloth_get(x, SLOTH_HERE)); 
}

/* -- Basic compilation -------------------------------- */

void sloth_comma(X* x, CELL v) { 
	sloth_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + sCELL);
}
void sloth_c_comma(X* x, uCHAR v) { 
	sloth_c_store(x, sloth_here(x), v);
	sloth_store(x, x->d, sloth_here(x) + suCHAR);
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

void sloth_f_comma(X* x, FCELL v) { 
	sloth_f_store(x, sloth_here(x), v);
	sloth_allot(x, sFCELL); 
}

#endif

/* -- Headers ------------------------------------------ */

CELL sloth_get_latest(X* x) { 
	return sloth_fetch(x, sloth_user_get(x, SLOTH_CURRENT));
}
void sloth_set_latest(X* x, CELL w) { 
	sloth_store(x, sloth_user_get(x, SLOTH_CURRENT), w);
}

CELL sloth_get_link(X* x, CELL w) {
	return sloth_fetch(x, w);
}

CELL sloth_get_xt(X* x, CELL w) {
	return sloth_fetch(x, w + sCELL);
}
void sloth_set_xt(X* x, CELL w, CELL xt) { 
	sloth_store(x, w + sCELL, xt); 
}

uCHAR sloth_get_flags(X* x, CELL w) {
	return sloth_c_fetch(x, w + 2*sCELL);
}

void sloth_set_flags(X* x, CELL w, uCHAR v) {
	sloth_c_store(x, w + 2*sCELL, v);
}

CELL sloth_has_flag(X* x, CELL w, CELL v) { 
	return sloth_get_flags(x, w) & v; 
}

void sloth_set_flag(X* x, CELL w, uCHAR v) {
	sloth_set_flags(x, w, sloth_get_flags(x, w) | v);
}

void sloth_unset_flag(X* x, CELL w, uCHAR v) {
	sloth_set_flags(x, w, sloth_get_flags(x, w) & ~v);
}

uCHAR sloth_get_namelen(X* x, CELL w) {
	return sloth_c_fetch(x, w + 2*sCELL + suCHAR);
}

CELL sloth_get_name_addr(X* x, CELL w) {
	(void)x;
	return w + 2*sCELL + 2*suCHAR;
}

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags uCHAR					@ NT + 3*sCELL */
/* Namelen uCHAR				@ NT + 3*sCELL + suCHAR */
/* Name uCHAR*namelen	@ NT + 3*sCELL + 2*suCHAR */

CELL sloth_header(X* x, CELL n, CELL l) {
	CELL w, i;
	sloth_align_(x);
	w = sloth_here(x); /* NT address */
	sloth_comma(x, sloth_get_latest(x));
	sloth_set_latest(x, w);
	sloth_comma(x, 0); /* Reserve space for XT */
	sloth_c_comma(x, 0); /* Flags (default flags: 0) */
	sloth_c_comma(x, l); /* Name length */
	for (i = 0; i < l; i++) sloth_c_comma(x, sloth_c_fetch(x, n + i)); /* Name */
	sloth_align_(x); /* Align XT address */
	sloth_set_xt(x, w, sloth_here(x));
	return w;
}

/* -- Inner interpreter -------------------------------- */

CELL sloth_op(X* x) {	
	CELL o = sloth_fetch(x, x->ip);	
	x->ip += sCELL;	
	return o; 
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

FCELL sloth_f_op(X* x) {
	FCELL n = sloth_f_fetch(x, x->ip);
	x->ip += sFCELL;
	return n;
}

#endif

void sloth__do_prim(X* x, CELL p) { (x->p->p[-1 - p])(x); }

void sloth__call(X* x, CELL q) { 
	if (x->ip >= 0 || x->rp > 0) sloth_rpush(x, x->ip); 
	x->ip = q; 
}

void sloth__execute(X* x, CELL q) { 
	if (q < 0) sloth__do_prim(x, q); 
	else sloth__call(x, q); 
}

void sloth__inner(X* x) { 
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) {
		sloth__execute(x, sloth_op(x));
	}
}

void sloth_eval(X* x, CELL q) { 
	sloth__execute(x, q); 
	if (q > 0) sloth__inner(x); 
}

/* -- Tracing inner interpreter ------------------------ */

void sloth__debug(X* x, CELL debug_xt) {
	sloth_push(x, x->ip);
	sloth_eval(x, debug_xt);
}

void sloth__debug_inner(X* x, CELL debug_xt) {
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) {
		sloth__debug(x, debug_xt);
		sloth__execute(x, sloth_op(x));
	}
}

void sloth_debug_(X* x) {
	CELL post_xt = sloth_pop(x); 
	CELL inner_xt = sloth_pop(x);
	CELL pre_xt = sloth_pop(x);
	CELL q = sloth_pop(x);
	sloth__debug(x, pre_xt);
	sloth__execute(x, q);
	if (q > 0) sloth__debug_inner(x, inner_xt);
	sloth__debug(x, post_xt);
}

/* Inner interpreter primitives */

void sloth_exit_(X* x) { x->ip = (x->rp > 0) ? sloth_rpop(x) : -1; }

void sloth_lit_(X* x) { sloth_push(x, sloth_op(x)); }
void sloth_rip_(X* x) {
	CELL ip = x->ip;
	CELL o = sloth_op(x);
	sloth_push(x, ip + o - sCELL);
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

void sloth_f_lit_(X* x) { sloth_f_push(x, sloth_f_op(x)); }

#endif

void sloth_branch_(X* x) { x->ip += sloth_op(x) - sCELL; }
void sloth_zbranch_(X* x) { 
	x->ip += sloth_pop(x) == 0 ? 
		(sloth_op(x) - sCELL) 
		: sCELL; 
}

/* -- Exceptions --------------------------------------- */

CELL sloth_catch(X* x, CELL q) {
	volatile int tsp = x->sp;
	volatile int trp = x->rp;
	volatile CELL tip = x->ip;
	volatile int e;
	volatile CELL err;

	if (!(e = setjmp(x->jmpbuf[++x->jmpbuf_idx]))) {
		sloth_eval(x, q);
		err = 0;
	} else {
		x->sp = tsp;
		x->rp = trp;
		x->ip = tip;
		err = e;
	}

	x->jmpbuf_idx--;

	return err;
}

void sloth_throw(X* x, CELL e) {
	if (x->jmpbuf_idx >= 0) {
		longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
	} else {
		CELL ibuf = *((CELL*)(x->u+20*sCELL));
		CELL ipos = *((CELL*)(x->u+21*sCELL));
		CELL ilen = *((CELL*)(x->u+22*sCELL));
		if (ibuf && ipos <= ilen) {
		    printf("BUFFER: <%.*s>\n", (int)ilen, (char*)ibuf);
		    printf("TOKEN: <%.*s>\n", (int)(ilen - ipos), (char*)(ibuf + ipos));
		}
#if defined(WINDOWS)
		printf("Exception: %Id\n", e);
#else
		printf("Exception: %ld\n", e);
#endif
		exit(e);
	}
}

void sloth_catch_(X* x) { sloth_push(x, sloth_catch(x, sloth_pop(x))); }
void sloth_throw_(X* x){ 
	CELL e = sloth_pop(x); 
	if (e) sloth_throw(x, e); 
}

/* -- Arithmetic and logical operations ---------------- */

void sloth_invert_(X* x) { sloth_push(x, ~sloth_pop(x)); }
void sloth_and_(X* x) { 
	CELL v = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) & v); 
}
void sloth_l_shift_(X* x) { 
	CELL n = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) << n); 
}
void sloth_minus_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) - a); 
}
void sloth_plus_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) + a); 
}
void sloth_r_shift_(X* x) { 
	CELL n = sloth_pop(x); 
	sloth_push(x, ((uCELL)sloth_pop(x)) >> n); 
}
void sloth_star_(X* x) { 
	CELL b = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) * b); 
}
void sloth_two_slash_(X* x) { 
	sloth_push(x, sloth_pop(x) >> 1); 
}
void sloth_u_m_star_(X* x) {
	uCELL b = (uCELL)sloth_pop(x), a = (uCELL)sloth_pop(x), high, low;

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

	sloth_push(x, low);
	sloth_push(x, high);
}
/* UM/MOD code taken from pForth */
#define DULT(du1l,du1h,du2l,du2h) ( (du2h<du1h) ? 0 : ( (du2h==du1h) ? (du1l<du2l) : 1) )
void sloth_u_m_slash_mod_(X* x) {
	uCELL ah, al, q, di, bl, bh, sl, sh;
	bh = (uCELL)sloth_pop(x);
	bl = 0;
	ah = (uCELL)sloth_pop(x);
	al = (uCELL)sloth_pop(x);
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
	sloth_push(x, al); /* rem */
	sloth_push(x, q);
}

/* -- Comparison operations ---------------------------- */

void sloth_equals_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) == a ? -1 : 0); 
}
void sloth_less_than_(X* x) { 
	CELL a = sloth_pop(x); 
	sloth_push(x, sloth_pop(x) < a ? -1 : 0); 
}

/* -- Strings ------------------------------------------ */

void sloth_string_(X* x) {
	CELL l = sloth_op(x);
	sloth_push(x, x->ip);
	sloth_push(x, l);
	x->ip = sloth_aligned(x->ip + l + 1);
}

/* TODO (CSTRING) is used only by CLITERAL that is used only */
/* by C" that is never used. It could be taken from here by */
/* making a Forth version that stores a normal string literal */
/* and copies it to a transient region or to the CBuffer, */
/* or even to the normal string buffer. */
void sloth_c_string_(X* x) {
	uCHAR l = sloth_c_fetch(x, x->ip);
	sloth_push(x, x->ip);
	x->ip = sloth_aligned(x->ip + l + 2);
}

/* MOVE moves address units, not characters */
void sloth_move_(X* x) {
	CELL u = sloth_pop(x);
	CELL addr2 = sloth_pop(x);
	CELL addr1 = sloth_pop(x);
	CELL i;
	if (addr1 >= addr2) {
		for (i = 0; i < u; i++) {
			sloth_b_store(x, addr2 + i, sloth_b_fetch(x, addr1 + i));
		}
	} else {
		for (i = u - 1; i >= 0; i--) {
			sloth_b_store(x, addr2 + i, sloth_b_fetch(x, addr1 + i));
		}
	}
}

/* -- Searching for words ------------------------------ */

int sloth__compare(X* x, CELL a1, uCELL u1, CELL a2, uCELL u2) {
	uCELL i;
	if (u1 != u2) return 0;
	for (i = 0; i < u2; i++) {
		uCHAR a = sloth_c_fetch(x, a1 + i);
		uCHAR b = sloth_c_fetch(x, a2 + i);
		if (a >= 97 && a <= 122) a -= 32;
		if (b >= 97 && b <= 122) b -= 32;
		if (a != b) return 0;
	}
	return 1;
}

CELL sloth__search_word(X* x, CELL n, int l) {
	CELL wl, w, i;
	/* The wordlist iteration starts at -1 to always search */
	/* on (LOCALS-WORDLIST) first, even if the search order */
	/* is completely empty. */
	for (i = -1; i < sloth_user_get(x, SLOTH_ORDER); i++) {
		wl = sloth_user_get(x, SLOTH_CONTEXT + i*sCELL);
		if (wl != 0) {
			w = sloth_fetch(x, wl);
			while (w > 0) {
				if (!sloth_has_flag(x, w, SLOTH_HIDDEN) 
				 && sloth__compare(
							x, 
							sloth_get_name_addr(x, w), sloth_get_namelen(x, w),
							n, l)) 
					return w;
				w = sloth_get_link(x, w);
			}
		}
	}
	return 0;
}

CELL sloth_find_word(X* x, char* name) {
	return sloth__search_word(x, (CELL)name, strlen(name));
}

void sloth_find_(X* x) {
	CELL cstring = sloth_pop(x);
	CELL w = sloth__search_word(
		x, 
		cstring + suCHAR, 
		sloth_c_fetch(x, cstring)
	);
	if (w == 0) {
		sloth_push(x, cstring);
		sloth_push(x, 0);
	} else if (sloth_has_flag(x, w, SLOTH_IMMEDIATE)) {
		sloth_push(x, sloth_get_xt(x, w));
		sloth_push(x, 1);
	} else {
		sloth_push(x, sloth_get_xt(x, w));
		sloth_push(x, -1);
	}
}

/* -- More compilation --------------------------------- */

void sloth_compile(X* x, CELL xt) { sloth_comma(x, xt); }

void sloth_literal(X* x, CELL n) { 
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(LIT)")));
	sloth_comma(x, n); 
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

void sloth_fliteral(X* x, FCELL n) {
	sloth_comma(x, sloth_get_xt(x, sloth_find_word(x, "(FLIT)")));
	sloth_f_comma(x, n);
}

#endif

/* -- Quotations --------------------------------------- */

void sloth_quotation_(X* x) { 
	CELL d = sloth_op(x); 
	sloth_push(x, x->ip); 
	x->ip += d; 
}

void sloth_start_quotation_(X* x) {
	CELL s = sloth_user_get(x, SLOTH_STATE);
	/* If in interpret mode (state<=0), substract 1 from state. */
	/* If in compile mode (state>0), add 1 to state. */
	sloth_user_set(x, SLOTH_STATE, s <= 0 ? s - 1 : s + 1);
	/* If in interpret mode and this is a first level quotation, */
	/* push its starting address to be able to execute it later. */
	if (sloth_user_get(x, SLOTH_STATE) == -1) 
		sloth_push(x, sloth_here(x) + 2*sCELL);
	/* Push latestXT to be save it while quotation is compiled. */
	/* LatestXT is needed by recurse. */
	sloth_push(x, sloth_user_get(x, SLOTH_LATESTXT));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(QUOTATION)")));
	/* Push HERE to be able to patch it later with the correct */
	/* jump distance, and reserve 1 cell for it. */
	sloth_push(x, sloth_here(x));
	sloth_comma(x, 0);
	/* Set latestXT to start of quotation, after (QUOTATION) and */
	/* the cell with the jump distance. */
	sloth_user_set(x, SLOTH_LATESTXT, sloth_here(x));
}

void sloth_end_quotation_(X* x) {
	CELL s = sloth_user_get(x, SLOTH_STATE), a = sloth_pop(x);
	/* Compile an EXIT at the end of the quotation. */
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	/* Store the correct jump distance in the reserved cell after */
	/* the (QUOTATION) word. */
	sloth_store(x, a, sloth_here(x) - a - sCELL);
	/* Restore previous latestXT */
	sloth_user_set(x, SLOTH_LATESTXT, sloth_pop(x));
	/* Restore previous state */
	sloth_user_set(x, SLOTH_STATE, s < 0 ? s + 1 : s - 1);
}

/* -- End work session --------------------------------- */

void sloth_bye_(X* x) { 
	(void)x;
	printf("\n"); 
	exit(0); 
}

/* -- Input/output and parsing operations -------------- */

void 
sloth_default_emit_(X* x) {
	printf("%c", (uCHAR)sloth_pop(x));
}
void
sloth_default_key_(X* x) {
	sloth_push(x, getch());
}

/* TODO: Should this be part of the context instead of globals? */
static F sloth_emit_ = sloth_default_emit_;
static F sloth_key_ = sloth_default_key_;

void 
sloth_set_emit(F fn) { 
	sloth_emit_ = fn ? fn : sloth_default_emit_;
}

void
sloth_set_key(F fn) {
	sloth_key_ = fn ? fn : sloth_default_key_;
}

/* -- */

void sloth_source_(X* x) { 
	sloth_push(x, sloth_user_get(x, SLOTH_IBUF)); 
	sloth_push(x, sloth_user_get(x, SLOTH_ILEN)); 
}

void sloth_word_(X* x) {
	/* The region to store WORD counted strings starts */
	/* at here + CBUF. */
	uCHAR c = (uCHAR)sloth_pop(x);
	CELL ibuf = sloth_user_get(x, SLOTH_IBUF);
	CELL ilen = sloth_user_get(x, SLOTH_ILEN);
	CELL ipos = sloth_user_get(x, SLOTH_IPOS);
	CELL start, end, i;
	/* First, ignore c until not c is found */
	/* The Forth Standard says that if the control character is */
	/* the space (hex 20) then control characters may be treated */
	/* as delimiters. */
	if (c == 32) {
		while (ipos < ilen && sloth_c_fetch(x, ibuf + ipos) <= c) 
			ipos++;
	} else {
		while (ipos < ilen && sloth_c_fetch(x, ibuf + ipos) == c) 
			ipos++;
	}
	start = ibuf + ipos;
	/* Next, continue parsing until c is found again */
	if (c == 32) {
		while (ipos < ilen && sloth_c_fetch(x, ibuf + ipos) > c) 
			ipos++;
	} else {
		while (ipos < ilen && sloth_c_fetch(x, ibuf + ipos) != c) 
			ipos++;
	}
	end = ibuf + ipos;	
	/* Now, copy it to the counted string buffer */
	/* TODO Here, end-start must be divided by sCHAR to ensure */
	/* implementations with char != 1 work well */
	sloth_c_store(x, sloth_here(x) + SLOTH_CBUF, end - start);

	for (i = 0; i < (end - start); i++) {
		sloth_c_store(
			x, 
			sloth_here(x) + SLOTH_CBUF + suCHAR + i*suCHAR, 
			sloth_c_fetch(x, start + i*suCHAR));
	}
	sloth_push(x, sloth_here(x) + SLOTH_CBUF);

	/* If we are not at the end of the input buffer, */
	/* skip c after the word, but its not part of the counted */
	/* string */
	if (ipos < ilen) ipos++;
	sloth_user_set(x, SLOTH_IPOS, ipos);
}

#ifndef SLOTH_NO_FILES
void sloth_file_position_(X* x) {
	FILE* fptr = (FILE*)sloth_pop(x);
	CELL pos = 0, ior = -37;
	/* TODO To detect incorrect fptr I must store a list of */
	/* currently opened file pointers. */
	pos = ftell(fptr);
	if (!ferror(fptr)) ior = 0;
	sloth_push(x, pos);
	/* FIXME This will simulate a double number for now */
	sloth_push(x, 0);
	sloth_push(x, ior);
}

void sloth_read_line_(X* x) {
	char buf[1024];
	FILE *fptr = (FILE*)sloth_pop(x);
	int u1 = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	int u2;
	/* Read at most u1 characters */
	if (u1 == 0) {
		sloth_push(x, 0);
		sloth_push(x, -1);
		sloth_push(x, 0);
	} else {
		/* We need to read u1 + 1 because fgets counts the */
		/* zero at the end. */
		if (fgets(buf, u1 + 1, fptr)) {
			u2 = strlen(buf);
			/* Detect CR/LF to substract it from counted chars */
			if (buf[u2 - 1] == 10 || buf[u2 - 1] == 13) u2--;
			/* In case of CRLF, substract one again */
			if (buf[u2 - 1] == 10) u2--;
			memcpy(caddr, buf, u2);
			/* Ensure that we add the 0 at the end */
			caddr[u2] = 0;
			sloth_push(x, u2);
			sloth_push(x, -1);
			sloth_push(x, 0);
		} else {
			sloth_push(x, 0);
			sloth_push(x, 0);
			if (feof(fptr)) {
				sloth_push(x, 0);
			} else {
				sloth_push(x, -37);
			}
		}
	}
}
#endif

/* TODO Refill could be implemented in ans.4th. */
/* As I have created the api functions READ-LINE and */
/* FILE-POSITION. */
/* Except for the comments on the few first lines (that can */
/* be removed without problems as \ is the fourth definition */
/* found), REFILL is not used again until the definition of */
/* ( in line 196 */
void sloth_refill_(X* x) {
	CELL flag, ior;
	CELL source_id = sloth_user_get(x, SLOTH_SOURCE_ID);
	switch (source_id) {
	case -1: 
		sloth_push(x, 0);
		break;
	case 0:
		sloth_push(x, sloth_user_get(x, SLOTH_IBUF)); 
		sloth_push(x, 80);
		sloth_eval(x, sloth_get_xt(x, sloth_find_word(x, "ACCEPT")));
		sloth_user_set(x, SLOTH_ILEN, sloth_pop(x));
		sloth_user_set(x, SLOTH_IPOS, 0);
		sloth_push(x, -1); 
		break;
#ifndef SLOTH_NO_FILES
	default:
		/* File position is stored to go back to it when */
		/* exiting of nesting includes */
		sloth_push(x, source_id);
		sloth_file_position_(x);
		sloth_pop(x);
		sloth_pop(x); /* TODO This is most significant part of the double number, it will normally be 0? */
		sloth_user_set(x, SLOTH_SOURCE_POS, sloth_pop(x));

		/* REFILL can only be called after INCLUDE/INCLUDED */
		/* that means that the line buffer of _included_ will */
		/* be used and its size is known and fixed. */
		sloth_push(x, sloth_user_get(x, SLOTH_IBUF));
		sloth_push(x, 1024);
		sloth_push(x, source_id);
		sloth_read_line_(x);

		ior = sloth_pop(x);
		flag = sloth_pop(x);

		if (flag && !ior) {
			sloth_user_set(x, SLOTH_ILEN, sloth_pop(x));
			sloth_user_set(x, SLOTH_IPOS, 0);
			sloth_push(x, -1);
		} else {
			sloth_pop(x);
			sloth_push(x, 0);
		}
		break;
#endif
	}	
}

/* TODO Could SAVE-INPUT and RESTORE-INPUT be implemented */
/* in ANS Forth? SAVE-INPUT surely... */
void sloth_save_input_(X* x) {
	sloth_push(x, sloth_user_get(x, SLOTH_SOURCE_POS));
	sloth_push(x, sloth_user_get(x, SLOTH_SOURCE_ID));
	sloth_push(x, sloth_user_get(x, SLOTH_IBUF));
	sloth_push(x, sloth_user_get(x, SLOTH_IPOS));
	sloth_push(x, sloth_user_get(x, SLOTH_ILEN));
	sloth_push(x, 5);
}

void sloth_restore_input_(X* x) {
	char* res;
	sloth_pop(x); /* Just drop count */
	sloth_user_set(x, SLOTH_ILEN, sloth_pop(x));
	sloth_user_set(x, SLOTH_IPOS, sloth_pop(x));
	sloth_user_set(x, SLOTH_IBUF, sloth_pop(x));
	sloth_user_set(x, SLOTH_SOURCE_ID, sloth_pop(x));
	sloth_user_set(x, SLOTH_SOURCE_POS, sloth_pop(x));
#ifndef SLOTH_NO_FILES
	if (sloth_user_get(x, SLOTH_SOURCE_ID) > 0) {
		fseek(
			(FILE*)sloth_user_get(x, SLOTH_SOURCE_ID),
			sloth_user_get(x, SLOTH_SOURCE_POS),
			SEEK_SET);
		res = fgets((char*)sloth_user_get(x, SLOTH_IBUF), 1024, (FILE*)sloth_user_get(x, SLOTH_SOURCE_ID));
		if (!res) {
			/* TODO Error management */
		}
	}
#endif
	sloth_push(x, 0);
}

#ifndef SLOTH_NO_FILES
void sloth__save_input_and_path(X* x) {
	int i;
	sloth_save_input_(x);
	sloth_push(x, sloth_user_get(x, SLOTH_PATH_START));
	sloth_push(x, sloth_user_get(x, SLOTH_PATH_END));
	for (i = 0; i < 8; i++) sloth_to_r_(x);
}

void sloth__restore_input_and_path(X* x) {
	int i;
	for (i = 0; i < 8; i++) sloth_r_from_(x);
	sloth_user_set(x, SLOTH_PATH_END, sloth_pop(x));
	sloth_user_set(x, SLOTH_PATH_START, sloth_pop(x));
	sloth_restore_input_(x);
	sloth_pop(x);
}

FILE* sloth__open_included_file(X* x, char* a, int l) {
	FILE* f;

	/* Variables for working with path, initialized to */
	/* reuse current path if possible. */
	char* pathstart = (char*)sloth_user_get(x, SLOTH_PATH_START);
	char* pathend = (char*)sloth_user_get(x, SLOTH_PATH_END);

	/* Copy pathname/filename to end of current path */
	strncpy(pathend, a, l);
	*(pathend + l) = 0;
	/* Try to use it as absolute path filename or relative to */
	/* current directory (as has been copied to the end of */
	/* previous path). */
	/* TODO explain that rb+ is needed for fopen to be */
	/* compatible both in Linux and on Windows */
	f = fopen(pathend, "rb+");
	if (f) {
		/* Storing path as absolute or relative to cwd */
		pathstart = pathend;
		pathend = pathend + l;
	} else {
		/* Trying as relative to previous path. */
		f = fopen(pathstart, "rb+");
		if (f) {
			pathend = pathend + l;
		}	else {
			/* Trying as relative to root path. */
			strncpy(pathend, (char*)(x->u + SLOTH_PATHS), sloth_user_get(x, SLOTH_ROOT_PATH_LENGTH));
			strncpy(pathend + sloth_user_get(x, SLOTH_ROOT_PATH_LENGTH), a, l);
			*(pathend + sloth_user_get(x, SLOTH_ROOT_PATH_LENGTH) + l) = 0;
			f = fopen(pathend, "rb+");
			/* Opening a file from the root path must not change */
			/* pathstart or pathend as everytime a file is opened */
			/* it can be checked against root, no need to remember it */
			/* and it's better to just remember previous dirs. */
		}
	}

	if (f) {
		/* Remove filename from path... */
		while (pathend > pathstart) {
			if (*pathend == '/' || *pathend == '\\') {
				pathend++;
				break;
			}
			pathend--;
		}
		/* ...and store for nested includes. */
		sloth_user_set(x, SLOTH_PATH_START, (CELL)pathstart);
		sloth_user_set(x, SLOTH_PATH_END, (CELL)pathend);
	}

	return f;
}

void sloth__add_to_included_files_list(X* x, char* a, int l) {
		/* TODO Check if this file has been included before, */
		/* and in that case don't add it to the linked list. */
		CELL here = sloth_here(x);
		sloth_comma(x, sloth_user_get(x, SLOTH_INCLUDED_FILES));
		sloth_user_set(x, SLOTH_INCLUDED_FILES, here);
		sloth_comma(x, l);
		memcpy((char*)sloth_here(x), a, l);
		sloth_allot(x, l);
		sloth_align_(x);
}

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
void sloth_included_(X* x) {
	FILE *f;
	char linebuf[1024];
	CELL e;
	size_t l = (size_t)sloth_pop(x);
	char* a = (char*)sloth_pop(x);
	sloth__save_input_and_path(x);

	if ((f = sloth__open_included_file(x, a, l))) {
		CELL linenumber = 0;
		sloth__add_to_included_files_list(x, a, l);

		sloth_user_set(x, SLOTH_SOURCE_ID, (CELL)f);

		sloth_user_set(x, SLOTH_IBUF, (CELL)linebuf);
		sloth_user_set(x, SLOTH_IPOS, 0);
		sloth_user_set(x, SLOTH_ILEN, 1024);

		do {
			sloth_refill_(x);
			if (!sloth_pop(x)) break;
			e = sloth_catch(x, sloth_user_get(x, SLOTH_INTERPRET));
			if (e != 0) {
				printf("File: %s\n", (char*)sloth_user_get(x, SLOTH_PATH_START));
				#ifdef WINDOWS
					printf("Line (%lld): %s\n", linenumber, linebuf);	
				#else
					printf("Line (%ld): %s\n", linenumber, linebuf);	
				#endif
				sloth_throw(x, e);
			}
			linenumber++;
		} while(1);

		fclose(f);
	}

	sloth__restore_input_and_path(x);

	if (!f) {
		sloth_throw(x, -38);
	}
}
#endif


/* -- Defining words ----------------------------------- */

void sloth_colon_(X* x) {
	CELL addr;
	sloth_push(x, 32); sloth_word_(x);
	addr = sloth_pop(x);
	sloth_header(x, addr + suCHAR, sloth_c_fetch(x, addr));
	sloth_user_set(x, SLOTH_LATESTXT, sloth_get_xt(x, sloth_get_latest(x)));
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_HIDDEN);
	sloth_user_set(x, SLOTH_STATE, 1);
}
void sloth_colon_no_name_(X* x) { 
	sloth_push(x, sloth_here(x));
	sloth_user_set(x, SLOTH_LATESTXT, sloth_here(x));
	sloth_user_set(x, SLOTH_STATE, 1);
}
void sloth_semicolon_(X* x) {
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_user_set(x, SLOTH_STATE, 0);
	/* Don't change flags for nonames */
	if (sloth_get_xt(x, sloth_get_latest(x)) == sloth_user_get(x, SLOTH_LATESTXT))
		sloth_unset_flag(x, sloth_get_latest(x), SLOTH_HIDDEN);
}
void sloth_recurse_(X* x) { 
	sloth_compile(x, sloth_user_get(x, SLOTH_LATESTXT)); 
}
void sloth_immediate_(X* x) { 
	sloth_set_flag(x, sloth_get_latest(x), SLOTH_IMMEDIATE); 
}
void sloth_postpone_(X* x) { 
	CELL i, xt, tlen;
	sloth_push(x, 32); sloth_word_(x);
	tlen = sloth_c_fetch(x, TOS(x));
	if (tlen == 0) { sloth_pop(x); return; }
	sloth_find_(x); 
	i = sloth_pop(x);
	xt = sloth_pop(x);
	if (i == 0) { 
		return;
	} else if (i == -1) {
		/* Compile the compilation of the normal word */
		sloth_literal(x, xt);
		sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "COMPILE,")));
	} else if (i == 1) {
		/* Compile the immediate word */

		sloth_compile(x, xt);
	}
}



void sloth_compile_comma_(X* x) { sloth_compile(x, sloth_pop(x)); }

void sloth_create_name_(X* x) {
	CELL tlen = sloth_pop(x);
	CELL tok = sloth_pop(x);
	sloth_header(x, tok, tlen);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(RIP)")));
	sloth_compile(x, 4*sCELL);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
}

/* CREATE parses the next word in the input buffer, creates */
/* a new header for it and then compiles some code. */
/* The compiled code is 5 CELLS long and has a RIP instruction */
/* with a displacement of 5 CELLS and three EXIT instructions. */
/* The RIP instruction will load onto the stack the address */
/* after the last EXIT instruction. That's the address used */
/* by created words. */
/* The first EXIT instruction exists to be replaced with a */
/* call to the DOES> part if CREATE DOES> is used and also */
/* returns after pushing the vlaue address if no DOES> part */
/* has been called. */
/* The next EXIT is there to return after calling the DOES> part. */
/* The last EXIT represents the function to call in case of using */
/* TO. */
void sloth_create_(X* x) {
	CELL addr;
	sloth_push(x, 32); sloth_word_(x);
	addr = sloth_pop(x);
	sloth_push(x, addr + suCHAR);
	sloth_push(x, sloth_c_fetch(x, addr));
	sloth_create_name_(x);
}
/* Helper compiled by DOES> that replaces the first EXIT */
/* compiled by CREATE on the new created word with a call */
/* to the code after the DOES> in the CREATE DOES> word */
void sloth_do_does_(X* x) {
	sloth_store(x, 
		sloth_get_xt(x, sloth_get_latest(x)) + 2*sCELL, 
		sloth_pop(x));
}
/* DOES> stores the XT of the code that will be executed */
/* by the created word. That's the address just after the EXIT */
/* compiled by DOES>. */
/* When a word is created, that address is pushed to the stack */
/* and the helper (DOES) is executed, compiling a call to the */
/* code just after the DOES> in the defining word. */
void sloth_does_(X* x) {
	sloth_literal(x, sloth_here(x) + 4*sCELL);
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "(DOES)")));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
}

void sloth_evaluate_(X* x) {
	CELL e;
	CELL l = sloth_pop(x), a = sloth_pop(x);

	CELL previbuf = sloth_user_get(x, SLOTH_IBUF);
	CELL previpos = sloth_user_get(x, SLOTH_IPOS);
	CELL previlen = sloth_user_get(x, SLOTH_ILEN);

	CELL prevsourceid = sloth_user_get(x, SLOTH_SOURCE_ID);

	sloth_user_set(x, SLOTH_SOURCE_ID, -1);

	sloth_user_set(x, SLOTH_IBUF, a);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, l);

	/* To ensure that the input buffer is restored correctly */
	/* even in case of a throw, I catch any possible throw */
	/* here and rethrow it after restoring the input buffer. */
	e = sloth_catch(x, sloth_user_get(x, SLOTH_INTERPRET));
	
	sloth_user_set(x, SLOTH_SOURCE_ID, prevsourceid);

	sloth_user_set(x, SLOTH_IBUF, previbuf);
	sloth_user_set(x, SLOTH_IPOS, previpos);
	sloth_user_set(x, SLOTH_ILEN, previlen);

	if (e != 0) {
		sloth_throw(x, e);
	}
}

void sloth_execute_(X* x) { sloth_eval(x, sloth_pop(x)); }

/* -- Outer interpreter -------------------------------- */

/* INTERPRET is not an ANS word ??!! */
void sloth_interpret_(X* x) {
	CELL flag, n;
	char* tok;
	int tlen;
	char buf[128]; char *endptr;
	int is_double;
	while (sloth_user_get(x, SLOTH_IPOS) < sloth_user_get(x, SLOTH_ILEN)) {
		sloth_push(x, 32); sloth_word_(x);
		tok = (char*)(TOS(x) + suCHAR);
		tlen = sloth_c_fetch(x, TOS(x));
		if (tlen == 0) { sloth_pop(x); return; }
		sloth_find_(x);
		if ((flag = sloth_pop(x)) != 0) {
			if (sloth_user_get(x, SLOTH_STATE) == 0
			|| (sloth_user_get(x, SLOTH_STATE) != 0 && flag == 1)) {
				sloth_eval(x, sloth_pop(x));	
			} else {
				sloth_compile(x, sloth_pop(x));
			}
		} else {
			CELL temp_base;
			temp_base = sloth_user_get(x, SLOTH_BASE);
			sloth_pop(x);
			if (tlen == 3 && *tok == '\'' && (*(tok + 2*suCHAR)) == '\'') {
				/* Character literal */
				if (sloth_user_get(x, SLOTH_STATE) == 0)	
					sloth_push(x, *(tok + suCHAR));
				else 
					sloth_literal(x, *(tok + suCHAR));
			} else {
				is_double = 0;
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
				} else if (*(tok + tlen - 1) == '.') {
					tlen--;
					is_double = 1;
				}
				strncpy(buf, tok, tlen);
				buf[tlen] = 0;
				n = strtol(buf, &endptr, temp_base);
				if (*endptr == '\0') {
					if (sloth_user_get(x, SLOTH_STATE) == 0) {
						sloth_push(x, n);
						if (is_double) sloth_push(x, n < 0 ? -1 : 0);
					} else { 
						sloth_literal(x, n);
						if (is_double) sloth_literal(x, n < 0 ? -1 : 0);
					}
				} else {
				#ifndef SLOTH_WITHOUT_FLOATING_POINT

					FCELL r;
					if (sloth_user_get(x, SLOTH_BASE) == 10) {
						r = strtod(buf, &endptr);	
						if (r == 0 && buf == endptr) {
							if (sloth_user_get(x, SLOTH_SOURCE_ID) != -1) {
								printf("%.*s ?\n", tlen, tok);
							}
							sloth_throw(x, -13);
						} else {
							if (sloth_user_get(x, SLOTH_STATE) == 0) {
								sloth_f_push(x, r);
							} else {
								sloth_fliteral(x, r);
							}
						}
					} else {
						if (sloth_user_get(x, SLOTH_SOURCE_ID) != -1) {
							printf("%.*s ?\n", tlen, tok);
						}
						sloth_throw(x, -13);
					}

				#else

					if (sloth_user_get(x, SLOTH_SOURCE_ID) != -1) {
						printf("%.*s ?\n", tlen, tok);
					}
					sloth_throw(x, -13);

				#endif
				}
			}
		}
	}
}

/* -- Environment queries ------------------------------ */

void sloth_environment_(X* x) {
	switch (sloth_pop(x)) {
	case 0: /* /COUNTED-STRING */ sloth_push(x, 64); break;
	case 1: /* /HOLD */	/* TODO */ break;
	case 2: /* /PAD */ /* TODO */ break;
	case 3: /* ADDRESS-UNIT-BITS */	sloth_push(x, CHAR_BIT); break;
	case 4: /* FLOORED */
		/* Good explanation about floored/symmetric division: */
		/* https://www.nimblemachines.com/symmetric-division-considered-harmful/ */
		sloth_push(x, (-3 / 2 == -2) ? -1 : 0);
		break;
	case 5: /* MAX-CHAR */ sloth_push(x, UCHAR_MAX); break;
	case 6: /* MAX-D */ /* TODO */ break;
	case 7: /* MAX-N */ /* TODO */ break;
	case 8: /* MAX-U */	/* TODO */ break;
	case 9: /* MAX-UD */ /* TODO */ break;
	case 10: /* RETURN-STACK-CELLS */
		sloth_push(x, SLOTH_RETURN_STACK_SIZE);
		break;
	case 11: /* STACK-CELLS */
		sloth_push(x, SLOTH_STACK_SIZE);
		break;
	case 12: /* FLOATING-STACK */
		#ifndef SLOTH_WITHOUT_FLOATING_POINT
			sloth_push(x, SLOTH_FLOAT_STACK_SIZE);
		#else
			sloth_push(x, -1);
		#endif
		break;
	/* Obsolescent queries (but required for tests) */
	case 100:
		#ifndef SLOTH_WITHOUT_FLOATING_POINT
			sloth_push(x, -1);
		#else
			sloth_push(x, 0);
		#endif
		break;
	/* Non standard queries */
	case -1: /* PLATFORM */
		/* This code adapted from: */
		/* https://stackoverflow.com/questions/142508/how-do-i-check-os-with-a-preprocessor-directive */
		#if defined(_WIN64)
			sloth_push(x, 0);
		#elif defined(WIN32) || defined(_WIN32)
			sloth_push(x, 1);
		#elif defined(__CYGWIN__) && !defined(_WIN32)
			sloth_push(x, 2);
		#elif defined(__ANDROID__)
			sloth_push(x, 3);
		#elif defined(__linux__)
			sloth_push(x, 4);
		#else
			sloth_push(x, -1);
		#endif
		break;
	case -2: /* RETURN KEY */
		#if defined(_WIN64) || defined(WIN32) || defined(_WIN32)
			sloth_push(x, 13);
		#else
			sloth_push(x, 10);
		#endif
		break;
	case -3: /* BACKSPACE KEY */
		#if defined(_WIN64) || defined(WIN32) || defined(_WIN32)
			sloth_push(x, 8);
		#else
			sloth_push(x, 127);
		#endif
	}
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

/* == Floating point word set ========================== */

/* Constructing compiler and interpreter system extensions */

void sloth_f_align_(X* x) { 
	sloth_set(
		x, 
		SLOTH_HERE, 
		ALIGNED(sloth_get(x, SLOTH_HERE), sFCELL));
}
void sloth_f_aligned_(X* x) { 
	sloth_push(x, ALIGNED(sloth_pop(x), sFCELL)); 
}
void sloth_f_literal_(X* x) { /* TODO */ (void)x; }
void sloth_floats_(X* x) { sloth_push(x, sloth_pop(x) * sFCELL); }
void sloth_float_plus_(X* x) { /* TODO */ (void)x; }

void sloth_s_f_aligned_(X* x) { 
	sloth_push(x, ALIGNED(sloth_pop(x), sSFCELL)); 
}
void sloth_d_f_aligned_(X* x) {
	sloth_push(x, ALIGNED(sloth_pop(x), sDFCELL)); 
}

void sloth_s_floats_(X* x) { sloth_push(x, sloth_pop(x) * sSFCELL); }
void sloth_d_floats_(X* x) { sloth_push(x, sloth_pop(x) * sDFCELL); }

/* Manipulating stack items */

void sloth_f_depth_(X* x) { sloth_push(x, x->fp); }
void sloth_f_drop_(X* x) { sloth_f_pop(x); }
void sloth_f_dup_(X* x) { sloth_f_push(x, sloth_f_pick(x, 0)); }
void sloth_f_over_(X* x) { sloth_f_push(x, sloth_f_pick(x, 1)); }
void sloth_f_rot_(X* x) { 
	FCELL c = sloth_f_pop(x);
	FCELL b = sloth_f_pop(x);
	FCELL a = sloth_f_pop(x);
	sloth_f_push(x, b);
	sloth_f_push(x, c);
	sloth_f_push(x, a);
}
void sloth_f_swap_(X* x) { 
	FCELL b = sloth_f_pop(x);
	FCELL a = sloth_f_pop(x);
	sloth_f_push(x, b);
	sloth_f_push(x, a);
}

/* Comparison operations */

void sloth_f_less_than_(X* x) { 
	FCELL b = sloth_f_pop(x);
	FCELL a = sloth_f_pop(x);
	sloth_push(x, a < b ? -1 : 0);
}
void sloth_f_zero_less_than_(X* x) { 
	sloth_push(x, sloth_f_pop(x) < 0.0 ? -1 : 0); 
}
void sloth_f_zero_equals_(X* x) {
	sloth_push(x, sloth_f_pop(x) == 0.0 ? -1 : 0);
}

/* Memory-stack transfer operations */

void sloth_f_fetch_(X* x) { 
	sloth_f_push(x, sloth_f_fetch(x, sloth_pop(x))); 
}
void sloth_f_store_(X* x) { 
	sloth_f_store(x, sloth_pop(x), sloth_f_pop(x)); 
}

void sloth_s_f_fetch_(X* x) { 
	sloth_f_push(x, sloth_s_f_fetch(x, sloth_pop(x))); 
}
void sloth_s_f_store_(X* x) { 
	sloth_s_f_store(x, sloth_pop(x), (SFCELL)sloth_f_pop(x));
}

void sloth_d_f_fetch_(X* x) { 
	sloth_f_push(x, sloth_d_f_fetch(x, sloth_pop(x))); 
}
void sloth_d_f_store_(X* x) { 
	sloth_d_f_store(x, sloth_pop(x), sloth_f_pop(x)); 
}

/* Commands to define data structures */

void sloth_f_constant_(X* x) { /* TODO */ (void)x; }
void sloth_f_variable_(X* x) { /* TODO */ (void)x; }

/* Number-type conversion operators */

void sloth_d_to_f_(X* x) {
	CELL hi = sloth_pop(x);
	CELL lo = sloth_pop(x);
	double r;
	if (hi >= 0) {
		r = ldexp((double)hi, sFCELL_BITS) + (double)lo;
	} else {
		/* negative number: -(2^128 - unsigned_value) */
		/* compute unsigned_value = ( (uint64_t)hi << 64 ) | lo */
		/* but better to subtract from 0.0 */
		r = - ( ldexp((double)(~(uCELL)hi), sFCELL_BITS) + (double)(~lo) + 1.0 );
	}
	sloth_f_push(x, r);
}
void sloth_f_to_d_(X* x) {
	FCELL i;
	modf(sloth_f_pop(x), &i);
	sloth_push(x, (CELL)i);
	sloth_push(x, i < 0 ? -1 : 0);
}

/* Arithmetic and logical operations */

void sloth_f_abs_(X* x) {
	sloth_f_push(x, fabs(sloth_f_pop(x)));
}
void sloth_f_plus_(X* x) { 
	FCELL b = sloth_f_pop(x);
	sloth_f_push(x, sloth_f_pop(x) + b);
}
void sloth_f_minus_(X* x) { 
	FCELL b = sloth_f_pop(x);
	sloth_f_push(x, sloth_f_pop(x) - b);
}
void sloth_f_star_(X* x) { 
	FCELL b = sloth_f_pop(x);
	sloth_f_push(x, sloth_f_pop(x) * b);
}
void sloth_f_star_star_(X* x) {
	FCELL b = sloth_f_pop(x);
	sloth_f_push(x, pow(sloth_f_pop(x), b));
}
void sloth_f_slash_(X* x) { 
	FCELL b = sloth_f_pop(x);
	sloth_f_push(x, sloth_f_pop(x) / b);
}
void sloth_floor_(X* x) { 
	sloth_f_push(x, floor(sloth_f_pop(x)));
}
void sloth_f_max_(X* x) { 
	FCELL b = sloth_f_pop(x);
	FCELL a = sloth_f_pop(x);
	sloth_f_push(x, a > b ? a : b);
}
void sloth_f_min_(X* x) { 
	FCELL b = sloth_f_pop(x);
	FCELL a = sloth_f_pop(x);
	sloth_f_push(x, a < b ? a : b);
}
void sloth_f_negate_(X* x) { 
	sloth_f_push(x, -sloth_f_pop(x));
}
void sloth_f_round_(X* x) {
    FCELL r = sloth_f_pop(x);
    FCELL fl = floor(r);
    FCELL diff = r - fl;

    if (diff < 0.5) {
        sloth_f_push(x, fl);
    } else if (diff > 0.5) {
        sloth_f_push(x, fl + 1.0);
    } else {
        /* Exact tie: pick the even one (least significant bit = 0) */
        FCELL even = (floor(fl / 2.0) == fl / 2.0) ? fl : fl + 1.0;
        sloth_f_push(x, even);
    }
}
void sloth_f_proximate_(X* x) {
	FCELL r3 = sloth_f_pop(x);
	FCELL r2 = sloth_f_pop(x);
	FCELL r1 = sloth_f_pop(x);
	if (SLOTH_F_ISNAN(r3)) {
		sloth_push(x, 0);
	} else if (r3 > 0.0) {
		sloth_push(x, fabs(r1 - r2) < r3 ? -1 : 0);
	} else if (r3 < 0.0) {
		sloth_push(x, fabs(r1 - r2) < (fabs(r3)*(fabs(r1)+fabs(r2))) ? -1 : 0);
	} else {
		uint64_t a, b;
		memcpy(&a, &r1, sizeof(double));
		memcpy(&b, &r2, sizeof(double));
		sloth_push(x, a == b ? -1 : 0);
	}
}
void sloth_f_atan2_(X* x) {
	FCELL b = sloth_f_pop(x);
	sloth_f_push(x, atan2(sloth_f_pop(x), b));
}
void sloth_f_sqrt_(X* x) {
	sloth_f_push(x, sqrt(sloth_f_pop(x)));
}
void sloth_f_l_n_(X* x) {
	sloth_f_push(x, log(sloth_f_pop(x)));
}
void sloth_f_sine_(X* x) {
	sloth_f_push(x, sin(sloth_f_pop(x)));
}
void sloth_f_cos_(X* x) {
	sloth_f_push(x, cos(sloth_f_pop(x)));
}
void sloth_f_sine_cos_(X* x) {
	FCELL r = sloth_f_pop(x);
	sloth_f_push(x, sin(r));
	sloth_f_push(x, cos(r));
}
void sloth_f_tan_(X* x) {
	sloth_f_push(x, tan(sloth_f_pop(x)));
}
void sloth_f_a_sine_(X* x) {
	sloth_f_push(x, asin(sloth_f_pop(x)));
}
void sloth_f_a_cos_(X* x) {
	sloth_f_push(x, acos(sloth_f_pop(x)));
}
void sloth_f_a_tan_(X* x) {
	sloth_f_push(x, atan(sloth_f_pop(x)));
}
void sloth_f_exp_(X* x) {
	sloth_f_push(x, exp(sloth_f_pop(x)));
}
void sloth_f_exp_m_one_(X* x) {
	sloth_f_push(x, exp(sloth_f_pop(x)) - 1.0);
}
void sloth_f_log_ten_(X* x) {
	sloth_f_push(x, log10(sloth_f_pop(x)));
}
void sloth_f_l_n_p_one_(X* x) {
	sloth_f_push(x, log(sloth_f_pop(x) + 1.0));
}
void sloth_f_a_log_(X* x) {
	sloth_f_push(x, pow(10.0, sloth_f_pop(x)));
}
void sloth_f_sin_h_(X* x) {
	sloth_f_push(x, sinh(sloth_f_pop(x)));
}
void sloth_f_cos_h_(X* x) {
	sloth_f_push(x, cosh(sloth_f_pop(x)));
}
void sloth_f_tan_h_(X* x) {
	sloth_f_push(x, tanh(sloth_f_pop(x)));
}
/* There's no asinh function in math.h in C89. It */
/* appeared on C99. */
void sloth_f_a_sine_h_(X* x) {
	FCELL r = sloth_f_pop(x);
	if (r == 0) {
		sloth_f_push(x, 0.0);
	} else if (r > 0) {
		sloth_f_push(x, log(r + sqrt(r * r + 1.0)));
	} else {
		sloth_f_push(x, -log(-r + sqrt(r * r + 1.0)));
	}
}
void sloth_f_a_cos_h_(X* x) {
	FCELL r = sloth_f_pop(x);
	if (r < 1.0) {
		/* undefined, push NaN */
		sloth_f_push(x, SLOTH_F_NAN());
	} else {
		sloth_f_push(x, log(r + sqrt(r * r - 1.0)));
	}
}

/* String/numeric conversion */

/* This function gets complicated to correctly represent */
/* the ANS Forth standard. It would be easier if just was */
/* a mirror to the strtod C function. */
void sloth_to_float_(X* x) {
	char buf[64]; 
	char *endptr;
	int tlen = (int)sloth_pop(x);
	char* tok = (char*)sloth_pop(x);
	FCELL n;
	int i, j, nlen, marker;;
	/* >FLOAT does not allow trailing spaces (although */
	/* strtod does). But, at the same time, a string of */
	/* blanks must be considered as a special case */
	/* representing zero. */
	if (*tok == ' ' || tlen == 0) {
		for (i = 0; i < tlen; i++) {
			/* If a non space character is found, we have a */
			/* trailing space string, and that means we */
			/* cannot convert it (by the standard). */
			if (*(tok + i) != ' ') {
				sloth_push(x, 0);
				return;
			}
		}
		/* If the string was made only of blanks, its a 0E */
		sloth_push(x, -1);
		sloth_f_push(x, 0.0);
		return;
	}
	if (*(tok + tlen - 1) == ' ') {
		sloth_push(x, 0);
		return;
	}
	/* Let's copy the string but taking into account the */
	/* possibility of not having the E in the string. */
	nlen = tlen;
	marker = 0;
	for (i = 0, j = 0; i < tlen; i++) {
		/* Forth's >FLOAT is a lot more restrictive than strtod */
		/* so we check if any non specified character appears in */
		/* the string to just do not allow the conversion. */
		if (*(tok + i) != '0' && *(tok + i) != '1' && *(tok + i) != '2'
		 && *(tok + i) != '3' && *(tok + i) != '4' && *(tok + i) != '5'
		 && *(tok + i) != '6' && *(tok + i) != '7' && *(tok + i) != '8'
		 && *(tok + i) != '9' && *(tok + i) != '+' && *(tok + i) != '-'
		 && *(tok + i) != 'D' && *(tok + i) != 'd' && *(tok + i) != 'E'
		 && *(tok + i) != 'e' && *(tok + i) != '.') {
			sloth_push(x, 0);
			return;
		}
		/* Being correct characters, D/d E/e must not appear */
		/* more than once. */
		if (*(tok + i) == 'D' || *(tok + i) == 'd' || *(tok + i) == 'E'
		 || *(tok + i) == 'e') {
			if (marker == 0) marker = 1;
			else {
				sloth_push(x, 0);
				return;
			}
		}
		if (i != 0
		 && (*(tok + i) == '+' || *(tok + i) == '-')
		 && (*(tok + i - 1) != 'E' && *(tok + i - 1) != 'e')) {
			buf[j++] = 'E';
			nlen++;
		}
		buf[j++] = *(tok + i);
	}
	buf[nlen] = 0;
	n = strtod(buf, &endptr);
	if (n == 0 && endptr == buf) {
		sloth_push(x, 0);
	} else {
		sloth_f_push(x, n);
		sloth_f_dot_s_(x); printf("\n");
		sloth_push(x, -1);
	}
}

void sloth_represent_(X* x) {
	FCELL r = sloth_f_pop(x);
	CELL u = sloth_pop(x);
	CELL addr = sloth_pop(x);
	/* This implementation uses the algorithm found in */
	/* represent_in_c.zip in Taygeta FTP. */
	/* I'm ignoring the REPRESENT-CHARS part */
	int i;
	size_t j, k;
	char buf[64], *endptr;
	/* 1. Fill buffer at caddr with n blanks (space chars) */
	/* where n is the greater of n1 or REPRESENT-CHARS. */
	for (i = 0; i < u; i++) sloth_c_store(x, addr + i, ' ');
	/* 2. Apply sprintf to r using %#.*E where * is */
	/* MAX-FLOAT-DIGITS less 1. */
	sprintf(buf, "%#.*E", (int)(u - 1), r);
	/* 3. Check if its a non-number representation. */
	for (j = 0; j < strlen(buf); j++) {
		if (buf[j] == 'n' || buf[j] == 'N') {
			for (k = 0; k < strlen(buf); k++) {
				sloth_c_store(x, addr + k, buf[k]);
				sloth_push(x, 0);
				sloth_push(x, 0);
				return;
			}
		}
	}
	/* 4. r was a finite number. */
	for (i = 0; i < u; i++) sloth_c_store(x, addr + i, '0');
	for (j = 0, k = 0; j < strlen(buf); j++) {
		if (buf[j] == 'E') {
			sloth_push(x, (strtol(buf + j + 1, &endptr, 10)) + 1);
			break;
		} else if (buf[j] != '-' && buf[j] != '.') {
			sloth_c_store(x, addr + k, buf[j]);
			k++;
		}
	}
	sloth_push(x, r < 0.0 ? -1 : 0);
	/* When should this return 0 as invalid result? */
	sloth_push(x, -1);
}

/* Output operations */

void sloth_f_dot_(X* x) {
	FCELL r = sloth_f_pop(x);
	int int_digits = (r == 0.0) ? 1 : (int)log10(fabs(r)) + 1;
	int decimals;
	if (r == floor(r)) {
		printf("%.0f. ", r);
	} else if (floor(r) == 0.0 || floor(r) == -1.0) {
		printf("%.*f ", (int)sloth_user_get(x, SLOTH_PRECISION), r);
	} else {
		decimals = sloth_user_get(x, SLOTH_PRECISION) - int_digits;
		printf("%.*f ", decimals, r);
	}
}

void sloth_f_s_dot_(X* x) {
	printf("%.*E ", (int)sloth_user_get(x, SLOTH_PRECISION) - 1, sloth_f_pop(x));
}

/* Engineering notation with special case handling */
/* by ChatGPT */
void sloth_f_e_dot_(X* x) {
	FCELL r = sloth_f_pop(x);
	int exp;
	double scaled;

	if (SLOTH_F_ISNAN(r)) {
		printf("NaN ");
		return;
	}
	
	if (SLOTH_F_ISINF(r)) {
		if (r > 0) {
			printf("Inf ");
		} else {
			printf("-Inf ");
		}
		return;
	}
	
	if (r == 0.0) {
		/* Will not differentiate between positive and */
		/* negative zero. */
		printf("0.0E+00 ");
		return;
	}
	
	exp = (int)floor(log10(fabs(r)) / 3.0) * 3;
	scaled = r / pow(10, exp);
	
	/* Format with 3 digits after decimal — tweak to taste */
	printf("%.3fE%+03d ", scaled, exp);
}

/* Non ANS floating point helpers */

void sloth_f_dot_s_(X* x) {
	int i;
#if defined(WINDOWS)
	printf("F:<%Id> ", x->fp);
#else
	printf("F:<%ld> ", x->fp);
#endif
	for (i = 0; i < x->fp; i++) printf("%f ", x->f[i]);
}

#endif

/* Non ANS helpers */

void sloth_self_(X* x) { sloth_push(x, (CELL)x); }
void sloth_dict_(X* x) { sloth_push(x, (CELL)x->d); }
void sloth_empty_return_stack_(X* x) { x->rp = 0; }
/* -- Primitive, word and user variable creation ------- */

CELL sloth_primitive(X* x, F f) { 
	assert(x->p->last < x->p->pz);
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL sloth_code(X* x, char* name, CELL xt) {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, xt);
	return xt; 
}

void sloth_user_variable(X* x, char* name, CELL d, CELL v) {
	CELL w = sloth_header(x, (CELL)name, strlen(name));
	sloth_set_xt(x, w, sloth_here(x));
	sloth_literal(x, (CELL)(x->u + d));
	sloth_compile(x, sloth_get_xt(x, sloth_find_word(x, "EXIT")));
	sloth_store(x, x->u + d, v);
}

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_kernel(X* x) {
	/* Basic primitives */

	sloth_code(x, "EXIT", sloth_primitive(x, &sloth_exit_));
	sloth_code(x, "(LIT)", sloth_primitive(x, &sloth_lit_));
	sloth_code(x, "(RIP)", sloth_primitive(x, &sloth_rip_));
	sloth_code(x, "(BRANCH)", sloth_primitive(x, &sloth_branch_));
	sloth_code(x, "(?BRANCH)", sloth_primitive(x, &sloth_zbranch_));

	/* Define user variables */

	sloth_user_variable(x, "(CURRENT)", SLOTH_CURRENT, sloth_to_abs(x, SLOTH_FORTH_WL));
	sloth_user_variable(x, "#ORDER", SLOTH_ORDER, 2);
	sloth_user_variable(x, "(LOCALS-WORDLIST)", SLOTH_LOCALS_WORDLIST, 0);
	sloth_user_variable(x, "CONTEXT", SLOTH_CONTEXT, sloth_to_abs(x, SLOTH_FORTH_WL));
	sloth_user_variable(x, "BASE", SLOTH_BASE, 10);
	sloth_user_variable(x, "STATE", SLOTH_STATE, 0);
	sloth_user_variable(x, "(IBUF)", SLOTH_IBUF, 0);
	sloth_user_variable(x, ">IN", SLOTH_IPOS, 0);
	sloth_user_variable(x, "(ILEN)", SLOTH_ILEN, 0);
	sloth_user_variable(x, "(SOURCE-ID)", SLOTH_SOURCE_ID, 0);
	sloth_user_variable(x, "(SOURCE-POS)", SLOTH_SOURCE_POS, 0);
	sloth_user_variable(x, "(LATESTXT)", SLOTH_LATESTXT, 0);
	sloth_user_variable(x, "(INTERPRET)", SLOTH_INTERPRET, sloth_primitive(x, &sloth_interpret_));

	sloth_user_variable(x, "(SLOTH_ROOT_PATH_LENGTH)", SLOTH_ROOT_PATH_LENGTH, 0);
	sloth_user_variable(x, "(SLOTH_PATH_START)", SLOTH_PATH_START, x->u + SLOTH_PATHS);
	sloth_user_variable(x, "(SLOTH_PATH_END)", SLOTH_PATH_END, x->u + SLOTH_PATHS);
	sloth_user_variable(x, "(SLOTH_PATHS)", SLOTH_PATHS, 0);

	sloth_user_variable(x, "(INCLUDED-FILES)", SLOTH_INCLUDED_FILES, 0);

	/* Data and return stack */

	sloth_code(x, "DROP", sloth_primitive(x, &sloth_drop_));
	sloth_code(x, "DUP", sloth_primitive(x, &sloth_dup_));
	sloth_code(x, "OVER", sloth_primitive(x, &sloth_over_));
	sloth_code(x, ">R", sloth_primitive(x, &sloth_to_r_));
	sloth_code(x, "R>", sloth_primitive(x, &sloth_r_from_));
	sloth_code(x, "SWAP", sloth_primitive(x, &sloth_swap_));

	/* Memory */

	sloth_code(x, "C@", sloth_primitive(x, &sloth_c_fetch_));
	sloth_code(x, "C!", sloth_primitive(x, &sloth_c_store_));
	sloth_code(x, "@", sloth_primitive(x, &sloth_fetch_));
	sloth_code(x, "!", sloth_primitive(x, &sloth_store_));

	sloth_code(x, "CELLS", sloth_primitive(x, &sloth_cells_));
	sloth_code(x, "CHARS", sloth_primitive(x, &sloth_chars_));

	sloth_code(x, "HERE", sloth_primitive(x, &sloth_here_));
  sloth_code(x, "ALIGN", sloth_primitive(x, &sloth_align_));
	sloth_code(x, "ALLOT", sloth_primitive(x, &sloth_allot_));
	sloth_code(x, "UNUSED", sloth_primitive(x, &sloth_unused_));

	/* Exceptions */

	sloth_code(x, "CATCH", sloth_primitive(x, &sloth_catch_));
	sloth_code(x, "THROW", sloth_primitive(x, &sloth_throw_));

	/* Arithmetic and logical operations */

	sloth_code(x, "INVERT", sloth_primitive(x, &sloth_invert_));
	sloth_code(x, "AND", sloth_primitive(x, &sloth_and_));
	sloth_code(x, "LSHIFT", sloth_primitive(x, &sloth_l_shift_));
	sloth_code(x, "-", sloth_primitive(x, &sloth_minus_));
	sloth_code(x, "+", sloth_primitive(x, &sloth_plus_));
	sloth_code(x, "RSHIFT", sloth_primitive(x, &sloth_r_shift_));
	sloth_code(x, "*", sloth_primitive(x, &sloth_star_));
	sloth_code(x, "2/", sloth_primitive(x, &sloth_two_slash_));
	sloth_code(x, "UM*", sloth_primitive(x, &sloth_u_m_star_));
	sloth_code(x, "UM/MOD", sloth_primitive(x, &sloth_u_m_slash_mod_));

	/* Comparison operations */

	sloth_code(x, "=", sloth_primitive(x, &sloth_equals_));
	sloth_code(x, "<", sloth_primitive(x, &sloth_less_than_));

	/* Strings */

	sloth_code(x, "(STRING)", sloth_primitive(x, &sloth_string_));
	sloth_code(x, "(CSTRING)", sloth_primitive(x, &sloth_c_string_));
	sloth_code(x, "MOVE", sloth_primitive(x, &sloth_move_));

	/* Input/output and parsing operations */

	sloth_code(x, "EMIT", sloth_primitive(x, sloth_emit_));
	sloth_code(x, "KEY", sloth_primitive(x, sloth_key_));
	sloth_code(x, "SOURCE", sloth_primitive(x, &sloth_source_));
	sloth_code(x, "WORD", sloth_primitive(x, &sloth_word_));
	sloth_code(x, "REFILL", sloth_primitive(x, &sloth_refill_));
	sloth_code(x, "SAVE-INPUT", sloth_primitive(x, &sloth_save_input_));
	sloth_code(x, "RESTORE-INPUT", sloth_primitive(x, &sloth_restore_input_));
#ifndef SLOTH_NO_FILES
	sloth_code(x, "FILE-POSITION", sloth_primitive(x, &sloth_file_position_));
	sloth_code(x, "READ-LINE", sloth_primitive(x, &sloth_read_line_));
	sloth_code(x, "INCLUDED", sloth_primitive(x, &sloth_included_));
#endif

	/* Finding words */

	sloth_code(x, "FIND", sloth_primitive(x, &sloth_find_));

	/* Quotations */

	sloth_code(x, "(QUOTATION)", sloth_primitive(x, &sloth_quotation_));
	sloth_code(x, "[:", sloth_primitive(x, &sloth_start_quotation_)); sloth_immediate_(x);
	sloth_code(x, ";]", sloth_primitive(x, &sloth_end_quotation_)); sloth_immediate_(x);

	/* End work session */

	sloth_code(x, "BYE", sloth_primitive(x, &sloth_bye_));

	/* Defining words */

	sloth_code(x, ":", sloth_primitive(x, &sloth_colon_));
	sloth_code(x, ":NONAME", sloth_primitive(x, &sloth_colon_no_name_));
	sloth_code(x, ";", sloth_primitive(x, &sloth_semicolon_)); sloth_immediate_(x);
	sloth_code(x, "RECURSE", sloth_primitive(x, &sloth_recurse_)); sloth_immediate_(x);
	sloth_code(x, "IMMEDIATE", sloth_primitive(x, &sloth_immediate_));
	sloth_code(x, "POSTPONE", sloth_primitive(x, &sloth_postpone_)); sloth_immediate_(x);

	sloth_code(x, "COMPILE,", sloth_primitive(x, &sloth_compile_comma_));
	sloth_code(x, "CREATE-NAME", sloth_primitive(x, &sloth_create_name_));
	sloth_code(x, "CREATE", sloth_primitive(x, &sloth_create_));
	sloth_code(x, "(DOES)", sloth_primitive(x, &sloth_do_does_));
	sloth_code(x, "DOES>", sloth_primitive(x, &sloth_does_)); sloth_immediate_(x);

	/* Executing */

	sloth_code(x, "EVALUATE", sloth_primitive(x, &sloth_evaluate_));
	sloth_code(x, "EXECUTE", sloth_primitive(x, &sloth_execute_));
	sloth_code(x, "DEBUG", sloth_primitive(x, &sloth_debug_));

	/* Environment queries */

	sloth_code(x, "(ENVIRONMENT)", sloth_primitive(x, &sloth_environment_));

	/* Non ANS helpers */

	sloth_code(x, "(SELF)", sloth_primitive(x, &sloth_self_));
	sloth_code(x, "(DICT)", sloth_primitive(x, &sloth_dict_));
	sloth_code(x, "(EMPTY-RETURN-STACK)", sloth_primitive(x, &sloth_empty_return_stack_));
}

#ifndef SLOTH_WITHOUT_FLOATING_POINT

void sloth_bootstrap_floating_point_word_set(X* x) {

	sloth_user_variable(x, "(PRECISION)", SLOTH_PRECISION, 15);

	/* == Primitives ===================================== */

	sloth_code(x, "(FLIT)", sloth_primitive(x, &sloth_f_lit_));

	/* == Floating point word set ======================== */

	/* Constructing compiler and interpreter system extensions */

	sloth_code(x, "FALIGN", sloth_primitive(x, &sloth_f_align_));
	sloth_code(x, "FALIGNED", sloth_primitive(x, &sloth_f_aligned_));
	sloth_code(x, "FLITERAL", sloth_primitive(x, &sloth_f_literal_));
	sloth_code(x, "FLOATS", sloth_primitive(x, &sloth_floats_));
	sloth_code(x, "FLOAT+", sloth_primitive(x, &sloth_float_plus_));

	sloth_code(x, "SFALIGNED", sloth_primitive(x, &sloth_s_f_aligned_));
	sloth_code(x, "DFALIGNED", sloth_primitive(x, &sloth_d_f_aligned_));

	sloth_code(x, "SFLOATS", sloth_primitive(x, &sloth_s_floats_));
	sloth_code(x, "DFLOATS", sloth_primitive(x, &sloth_d_floats_));

	/* Manipulating stack items */

	sloth_code(x, "FDEPTH", sloth_primitive(x, &sloth_f_depth_));
	sloth_code(x, "FDROP", sloth_primitive(x, &sloth_f_drop_));
	sloth_code(x, "FDUP", sloth_primitive(x, &sloth_f_dup_));
	sloth_code(x, "FOVER", sloth_primitive(x, &sloth_f_over_));
	sloth_code(x, "FROT", sloth_primitive(x, &sloth_f_rot_));
	sloth_code(x, "FSWAP", sloth_primitive(x, &sloth_f_swap_));

	/* Comparison operations */

	sloth_code(x, "F<", sloth_primitive(x, &sloth_f_less_than_));
	sloth_code(x, "F0<", sloth_primitive(x, &sloth_f_zero_less_than_));
	sloth_code(x, "F0=", sloth_primitive(x, &sloth_f_zero_equals_));

	/* Memory-stack transfer operations */

	sloth_code(x, "F@", sloth_primitive(x, &sloth_f_fetch_));
	sloth_code(x, "F!", sloth_primitive(x, &sloth_f_store_));

	sloth_code(x, "SF@", sloth_primitive(x, &sloth_s_f_fetch_));
	sloth_code(x, "SF!", sloth_primitive(x, &sloth_s_f_store_));

	sloth_code(x, "DF@", sloth_primitive(x, &sloth_d_f_fetch_));
	sloth_code(x, "DF!", sloth_primitive(x, &sloth_d_f_store_));

	/* Number-type conversion operators */

	sloth_code(x, "D>F", sloth_primitive(x, &sloth_d_to_f_));
	sloth_code(x, "F>D", sloth_primitive(x, &sloth_f_to_d_));
	/* Not needed: sloth_code(x, "S>F", sloth_primitive(x, &sloth_s_to_f_)); */

	/* Arithmetic and logical operations */

	sloth_code(x, "FABS", sloth_primitive(x, &sloth_f_abs_));
	sloth_code(x, "F+", sloth_primitive(x, &sloth_f_plus_));
	sloth_code(x, "F-", sloth_primitive(x, &sloth_f_minus_));
	sloth_code(x, "F*", sloth_primitive(x, &sloth_f_star_));
	sloth_code(x, "F**", sloth_primitive(x, &sloth_f_star_star_));
	sloth_code(x, "F/", sloth_primitive(x, &sloth_f_slash_));
	sloth_code(x, "FLOOR", sloth_primitive(x, &sloth_floor_));
	sloth_code(x, "FMAX", sloth_primitive(x, &sloth_f_max_));
	sloth_code(x, "FMIN", sloth_primitive(x, &sloth_f_min_));
	sloth_code(x, "FNEGATE", sloth_primitive(x, &sloth_f_negate_));
	sloth_code(x, "FROUND", sloth_primitive(x, &sloth_f_round_));
	sloth_code(x, "F~", sloth_primitive(x, &sloth_f_proximate_));
	sloth_code(x, "FATAN2", sloth_primitive(x, &sloth_f_atan2_));
	sloth_code(x, "FSQRT", sloth_primitive(x, &sloth_f_sqrt_));
	sloth_code(x, "FLN", sloth_primitive(x, &sloth_f_l_n_));
	sloth_code(x, "FSIN", sloth_primitive(x, &sloth_f_sine_));
	sloth_code(x, "FCOS", sloth_primitive(x, &sloth_f_cos_));
	sloth_code(x, "FSINCOS", sloth_primitive(x, &sloth_f_sine_cos_));
	sloth_code(x, "FTAN", sloth_primitive(x, &sloth_f_tan_));
	sloth_code(x, "FASIN", sloth_primitive(x, &sloth_f_a_sine_));
	sloth_code(x, "FACOS", sloth_primitive(x, &sloth_f_a_cos_));
	sloth_code(x, "FATAN", sloth_primitive(x, &sloth_f_a_tan_));
	sloth_code(x, "FEXP", sloth_primitive(x, &sloth_f_exp_));
	sloth_code(x, "FEXPM1", sloth_primitive(x, &sloth_f_exp_m_one_));
	sloth_code(x, "FLOG", sloth_primitive(x, &sloth_f_log_ten_));
	sloth_code(x, "FLNP1", sloth_primitive(x, &sloth_f_l_n_p_one_));
	sloth_code(x, "FALOG", sloth_primitive(x, &sloth_f_a_log_));
	sloth_code(x, "FSINH", sloth_primitive(x, &sloth_f_sin_h_));
	sloth_code(x, "FCOSH", sloth_primitive(x, &sloth_f_cos_h_));
	sloth_code(x, "FTANH", sloth_primitive(x, &sloth_f_tan_h_));
	sloth_code(x, "FASINH", sloth_primitive(x, &sloth_f_a_sine_h_));
	sloth_code(x, "FACOSH", sloth_primitive(x, &sloth_f_a_cos_h_));

	/* String/numeric conversion */

	sloth_code(x, ">FLOAT", sloth_primitive(x, &sloth_to_float_));
	sloth_code(x, "REPRESENT", sloth_primitive(x, &sloth_represent_));

	/* Output operations */

	sloth_code(x, "F.", sloth_primitive(x, &sloth_f_dot_));
	sloth_code(x, "FS.", sloth_primitive(x, &sloth_f_s_dot_));
	sloth_code(x, "FE.", sloth_primitive(x, &sloth_f_e_dot_));

	/* Non ANS helpers */

	sloth_code(x, "F.S", sloth_primitive(x, &sloth_f_dot_s_));
}

void sloth_bootstrap(X* x) {
	sloth_bootstrap_kernel(x);
	sloth_bootstrap_floating_point_word_set(x);
}

#else

void sloth_bootstrap(X* x) {
	sloth_bootstrap_kernel(x);
}

#endif

/* -- Context initialization and destruction ----------- */

void sloth__init(X* x, CELL d, CELL dz, CELL u, CELL uz) { 
	x->sp = 0; 
	x->rp = 0; 
#ifndef SLOTH_WITHOUT_FLOATING_POINT
	x->fp = 0;
#endif
	x->ip = -1; 
	x->d = d;
	x->dz = dz;
	x->u = u;
	x->uz = uz;

	x->jmpbuf_idx = -1;

	/* Initialize HERE */
	*((CELL*)(x->d + 0*sCELL)) = x->d + 3*sCELL;
	/* Initialize INTERNAL-WORDLIST */
	*((CELL*)(x->d + 1*sCELL)) = 0;
	/* Initialize FORTH-WORDLIST (the default wordlist) */
	*((CELL*)(x->d + 2*sCELL)) = 0;

	/* Initialize CURRENT to point to FORTH-WORDLIST */
	*((CELL*)x->u) = sloth_to_abs(x, SLOTH_FORTH_WL); /* CURRENT */
	*((CELL*)(x->u + 1*sCELL)) = 2; /* #ORDER */
	*((CELL*)(x->u + 2*sCELL)) = 0; /* LOCALS-WORDLIST */
	/* CONTEXT 0 */
	*((CELL*)(x->u + 3*sCELL)) = sloth_to_abs(x, SLOTH_FORTH_WL);
	/* CONTEXT 1 */
	*((CELL*)(x->u + 4*sCELL)) = sloth_to_abs(x, SLOTH_INTERNAL_WL);
}

X* sloth_create(int psize, int dsize, int usize) {
	X* x;

	x = malloc(sizeof(X));
	x->p = malloc(sizeof(sloth_P));
	x->p->p = malloc(sizeof(F) * psize);
	x->p->last = 0;
	x->p->pz = psize;
	x->d = (CELL)calloc(1, dsize);
	x->u = (CELL)calloc(1, usize);

	sloth__init(x, x->d, dsize, x->u, usize);

	return x;
}

X* sloth_new() { return sloth_create(512, 524288, 1024); }

void sloth_free(X* x) {
	free((void*)x->d);
	free(x->p->p);
	free(x->p);
	free(x);
}

/* Helpers to work with files from C */

void sloth_set_root_path(X* x, char* s) {
	memcpy((char*)(x->u + SLOTH_PATHS), s, strlen(s));
	sloth_user_set(x, SLOTH_ROOT_PATH_LENGTH, strlen(s));
	sloth_user_set(x, SLOTH_PATH_START, x->u + SLOTH_PATHS + strlen(s));
	sloth_user_set(x, SLOTH_PATH_END, x->u + SLOTH_PATHS + strlen(s));
}

int sloth_include(X* x, char* f) {
	CELL e;
	sloth_push(x, (CELL)f);
	sloth_push(x, strlen(f));
	e = sloth_catch(x, sloth_get_xt(x, sloth_find_word(x, "INCLUDED")));
	if (e) {
		/* If an exception has been thrown the stack will */
		/* be at its previous position and the address and */
		/* length of the filename has to be removed. */
		sloth_pop(x);
		sloth_pop(x);
	}
	return e;
}

void sloth_evaluate(X* x, char* s) {
	sloth_push(x, (CELL)s);
	sloth_push(x, (CELL)strlen(s));
	sloth_evaluate_(x);
}

/* Helper REPL */

void sloth_repl(X* x) {
	char buf[125];
	sloth_user_set(x, SLOTH_IBUF, (CELL)buf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 80);
	sloth_eval(x, sloth_get_xt(x, sloth_find_word(x, "QUIT")));
}


