/* All this code is extracted from sloth.h but needs */
/* to be tested again here. */

#include<sloth.h>

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

/* Code adapted from pForth */
void sloth_d_plus_(X* x) { 
	uCELL ah, al, bl, bh, sh, sl;
	bh = (uCELL)sloth_pop(x);
	bl = (uCELL)sloth_pop(x);
	ah = (uCELL)sloth_pop(x);
	al = (uCELL)sloth_pop(x);
	sh = 0;
	sl = al + bl;
	if (sl < bl) sh = 1;	/* carry */
	sh += ah + bh;
	sloth_push(x, sl);
	sloth_push(x, sh);
}

/* Code for _m_star has been created by claude.ai */
void sloth_m_star_(X* x) {
	CELL b = sloth_pop(x), a = sloth_pop(x), high, low;

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

	sloth_push(x, low);
	sloth_push(x, high);
}

void bootstrap_core_wordset(X* x) {
	SLOTH_CODE("D+", d_plus);
	SLOTH_CODE("M*", m_star);
}
