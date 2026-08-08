#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  ifndef MAX_PATH
#    define MAX_PATH 260
#  endif
#endif

#include "sloth.h"
#include "unity.h"

/* Custom EMIT for testing — replaces the default printf one */
char emitted_char = 0;
void custom_emit_(X* x) { emitted_char = (char)sloth_pop(x); }

/* Custom KEY for testing */
int mock_key_char = 0;
void custom_key_(X* x) { sloth_push(x, mock_key_char); }

X* x;
char ibuf[1024];

void setUp(void) {
	/* Create a VM with 64 primitive slots and 32767 bytes */
	/* of dictionary. */
	x = sloth_create(256, 32767, 1024);
	sloth_set_emit(custom_emit_);
	sloth_set_key(custom_key_);
}

void tearDown(void) {
	sloth_free(x);
}

void test_context_init(void) {
	TEST_ASSERT_NOT_NULL(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(0, x->rp);
	TEST_ASSERT_EQUAL(-1, x->ip);
	TEST_ASSERT_NOT_NULL((void*)x->d);
	TEST_ASSERT_NOT_NULL((void*)x->u);
	TEST_ASSERT_EQUAL(32767, x->dz);
	TEST_ASSERT_EQUAL(1024, x->uz);
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 0*sCELL)), x->d + 3*sCELL);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 1*sCELL)), 0);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 2*sCELL)), 0);
	TEST_ASSERT_EQUAL(*((CELL*)(x->u + 0*sCELL)), x->d + 2*sCELL);
}

void test_new(void) {
	X* x2 = sloth_new();
	TEST_ASSERT_NOT_NULL(x2);
	TEST_ASSERT_EQUAL(512, x2->p->pz);
	TEST_ASSERT_EQUAL(524288, x2->dz);
	TEST_ASSERT_EQUAL(1024, x2->uz);
	sloth_free(x2);
}

/* -- Data and return stack ---------------------------- */

void test_data_stack(void) {
	sloth_push(x, 42);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(42, x->s[0]);
	
	sloth_push(x, 100);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(100, x->s[1]);
	TEST_ASSERT_EQUAL(42, x->s[0]);
	
	sloth_dup_(x);
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(100, x->s[2]);
	TEST_ASSERT_EQUAL(100, x->s[1]);
	TEST_ASSERT_EQUAL(42, x->s[0]);
	sloth_drop_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	sloth_swap_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(100, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->sp);
}

void test_over_(void) {
	sloth_push(x, 42);
	sloth_push(x, 100);
	sloth_over_(x);
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(42, x->s[2]);
	TEST_ASSERT_EQUAL(100, x->s[1]);
	TEST_ASSERT_EQUAL(42, x->s[0]);
}

void test_return_stack(void) {
	sloth_rpush(x, 42);
	TEST_ASSERT_EQUAL(1, x->rp);
	TEST_ASSERT_EQUAL(42, x->r[0]);
	
	sloth_rpush(x, 100);
	TEST_ASSERT_EQUAL(2, x->rp);
	TEST_ASSERT_EQUAL(100, x->r[1]);
	TEST_ASSERT_EQUAL(42, x->r[0]);
	
	sloth_r_from_(x);
	TEST_ASSERT_EQUAL(1, x->rp);
	TEST_ASSERT_EQUAL(42, x->r[0]);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(100, x->s[0]);
	
	sloth_to_r_(x);
	TEST_ASSERT_EQUAL(2, x->rp);
	TEST_ASSERT_EQUAL(100, x->r[1]);
	TEST_ASSERT_EQUAL(42, x->r[0]);
	
	TEST_ASSERT_EQUAL(100, sloth_rpop(x));
	TEST_ASSERT_EQUAL(1, x->rp);
	TEST_ASSERT_EQUAL(42, sloth_rpop(x));
	TEST_ASSERT_EQUAL(0, x->rp);
}

/* -- Memory management -------------------------------- */

void test_memory(void) {
	TEST_ASSERT_EQUAL(x->d, sloth_to_abs(x, 0));
	TEST_ASSERT_EQUAL(0, sloth_to_rel(x, x->d));
	
	sloth_b_store(x, sloth_to_abs(x, 1000), -33);
	TEST_ASSERT_EQUAL(-33, sloth_b_fetch(x, sloth_to_abs(x, 1000)));

	sloth_c_store(x, sloth_to_abs(x, 1000), 'a');
	TEST_ASSERT_EQUAL('a', sloth_c_fetch(x, sloth_to_abs(x, 1000)));
	
	sloth_store(x, sloth_to_abs(x, 1000), 123);
	TEST_ASSERT_EQUAL(123, sloth_fetch(x, sloth_to_abs(x, 1000)));
	
	sloth_set(x, 1000, 13);
	TEST_ASSERT_EQUAL(13, sloth_get(x, 1000));
	
	sloth_user_set(x, 1000, 111);
	TEST_ASSERT_EQUAL(111, sloth_user_get(x, 1000));
	
	TEST_ASSERT_EQUAL(x->d + 3*sCELL, sloth_here(x));
	sloth_allot(x, sCELL);
	TEST_ASSERT_EQUAL(x->d + 4*sCELL, sloth_here(x));
	
	sloth_allot(x, suCHAR);
	TEST_ASSERT_EQUAL(x->d + 5*sCELL, sloth_aligned(sloth_here(x)));
	sloth_align_(x);
	TEST_ASSERT_EQUAL(x->d + 5*sCELL, sloth_here(x));
	
	sloth_push(x, 99);
	sloth_push(x, sloth_to_abs(x, 1000));
	sloth_store_(x);
	sloth_push(x, sloth_to_abs(x, 1000));
	sloth_fetch_(x);
	TEST_ASSERT_EQUAL(99, sloth_pop(x));
	
	sloth_push(x, 127);
	sloth_push(x, sloth_to_abs(x, 1000));
	sloth_c_store_(x);
	sloth_push(x, sloth_to_abs(x, 1000));
	sloth_c_fetch_(x);
	TEST_ASSERT_EQUAL(127, sloth_pop(x));
	
	sloth_push(x, 1);
	sloth_cells_(x);
	TEST_ASSERT_EQUAL(sCELL, sloth_pop(x));
	
	sloth_push(x, 1);
	sloth_chars_(x);
	TEST_ASSERT_EQUAL(suCHAR, sloth_pop(x));

	sloth_unused_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL((x->d + x->dz) - sloth_here(x), sloth_pop(x));
}

void test_allot_(void) {
	CELL here = sloth_here(x);
	sloth_push(x, 10);
	sloth_allot_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(here + 10, sloth_here(x));
}

void test_here_(void) {
	sloth_here_(x);
	TEST_ASSERT_EQUAL(sloth_here(x), sloth_pop(x));
	sloth_here_(x);
	TEST_ASSERT_EQUAL(sloth_fetch(x, x->d), sloth_pop(x));
}

/* -- Compilation -------------------------------------- */

void test_compilation(void) {
	TEST_ASSERT_EQUAL(x->d + 3*sCELL, sloth_here(x));
	sloth_comma(x, 13);
	TEST_ASSERT_EQUAL(x->d + 4*sCELL, sloth_here(x));
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, x->d + 3*sCELL));
	
	sloth_c_comma(x, 17);
	TEST_ASSERT_EQUAL(x->d + 4*sCELL + suCHAR, sloth_here(x));
	TEST_ASSERT_EQUAL(17, sloth_c_fetch(x, x->d + 4*sCELL));
}

/* -- Headers ------------------------------------------ */

void test_get_set_latest(void) {
	TEST_ASSERT_EQUAL(0, sloth_get_latest(x));
	sloth_set_latest(x, 13);
	TEST_ASSERT_EQUAL(13, sloth_get_latest(x));
}

void test_get_link(void) {
	sloth_set(x, 100, 13);
	TEST_ASSERT_EQUAL(13, sloth_get_link(x, sloth_to_abs(x, 100)));
}

void test_get_set_xt(void) {
	sloth_set_xt(x, sloth_to_abs(x, 100), 17);
	TEST_ASSERT_EQUAL(17, sloth_get_xt(x, sloth_to_abs(x, 100)));
}

void test_get_set_flags(void) {
	sloth_set_flags(x, sloth_to_abs(x, 100), 33);
	TEST_ASSERT_EQUAL(33, sloth_get_flags(x, sloth_to_abs(x, 100)));
}

void test_has_set_unset_flag(void) {
	sloth_set_flags(x, sloth_to_abs(x, 100), 0);
	TEST_ASSERT_FALSE(sloth_has_flag(x, sloth_to_abs(x, 100), 2));
	sloth_set_flag(x, sloth_to_abs(x, 100), 2);
	TEST_ASSERT_TRUE(sloth_has_flag(x, sloth_to_abs(x, 100), 2));
	sloth_unset_flag(x, sloth_to_abs(x, 100), 2);
	TEST_ASSERT_FALSE(sloth_has_flag(x, sloth_to_abs(x, 100), 2));
}

void test_headers(void) {
	char name[] = "NEW-WORD";
	CELL w = sloth_here(x);
	sloth_header(x, (CELL)name, (CELL)strlen(name));
	TEST_ASSERT_EQUAL(sloth_aligned(w + 2*sCELL + 2*suCHAR + strlen(name)*suCHAR), sloth_here(x));
	TEST_ASSERT_EQUAL(sloth_aligned(sloth_here(x)), sloth_here(x));
	TEST_ASSERT_EQUAL(x->d + 3*sCELL, sloth_get_latest(x));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, x->d + 3*sCELL));
	TEST_ASSERT_EQUAL(sloth_here(x), sloth_fetch(x, x->d + 4*sCELL));
	TEST_ASSERT_EQUAL(0, sloth_c_fetch(x, x->d + 5*sCELL));
	TEST_ASSERT_EQUAL((uCHAR)strlen(name), sloth_c_fetch(x, x->d + 5*sCELL + 1*suCHAR));
	TEST_ASSERT_EQUAL_MEMORY(name, x->d + 5*sCELL + 2*suCHAR, strlen(name));
}

void test_name_and_len(void) {
	char name[] = "TEST-WORD";
	CELL w = sloth_header(x, (CELL)name, (CELL)strlen(name));
	TEST_ASSERT_EQUAL(strlen(name), sloth_get_namelen(x, w));
	TEST_ASSERT_EQUAL_MEMORY(name, (char*)sloth_get_name_addr(x, w), sloth_get_namelen(x, w));
}

/* -- Primitive and word creation ---------------------- */

int my_var = 11;
void my_primitive(X* x) { (void)x; my_var = 17; }

void test_primitive_and_word_creation(void) {
	TEST_ASSERT_EQUAL(0, x->p->last);
	TEST_ASSERT_EQUAL(-1, sloth_primitive(x, &my_primitive));
	TEST_ASSERT_EQUAL(1, x->p->last);
	TEST_ASSERT_EQUAL(&my_primitive, *x->p->p);
	
	sloth_code(x, "MY-PRIMITIVE", -1);
	TEST_ASSERT_EQUAL(-1, sloth_fetch(x, x->d + 4*sCELL));
	TEST_ASSERT_EQUAL_MEMORY("MY-PRIMITIVE", x->d + 5*sCELL + 2*suCHAR, strlen("MY-PRIMITIVE"));
}

/* -- Inner interpreter -------------------------------- */

void test_op(void) {
	sloth_set(x, 3*sCELL, 11);
	x->ip = sloth_to_abs(x, 3*sCELL);
	TEST_ASSERT_EQUAL(11, sloth_op(x));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 4*sCELL), x->ip);
}

void test_do_prim(void) {
	TEST_ASSERT_EQUAL(11, my_var);
	sloth_primitive(x, &my_primitive);
	sloth__do_prim(x, -1);
	TEST_ASSERT_EQUAL(17, my_var);
}

void test_call(void) {
	x->ip = -1;
	sloth__call(x, 17);
	TEST_ASSERT_EQUAL(17, x->ip);
	TEST_ASSERT_EQUAL(0, x->rp);

	/* This test ensures that if there's something in the */
	/* return stack (like storing input source information) */
	/* a call pushes the previous IP (in this case -1) to the */
	/* return stack to not EXIT to a non IP value at the end */
	/* of the called word. */
	sloth_rpush(x, 13);
	x->ip = -1;
	sloth__call(x, 17);
	TEST_ASSERT_EQUAL(17, x->ip);
	TEST_ASSERT_EQUAL(2, x->rp);
	TEST_ASSERT_EQUAL(-1, x->r[1]);
	TEST_ASSERT_EQUAL(13, x->r[0]);
	sloth_rpop(x);
	sloth_rpop(x);

	x->ip = 11;
	sloth__call(x, 19);
	TEST_ASSERT_EQUAL(19, x->ip);
	TEST_ASSERT_EQUAL(1, x->rp);
	TEST_ASSERT_EQUAL(11, x->r[0]);
	sloth_rpop(x);
	
	sloth__call(x, -1);
	TEST_ASSERT_EQUAL(-1, x->ip);
}

void test_execute(void) {
	sloth_primitive(x, &my_primitive);
	
	my_var = 11;
	sloth__execute(x, -1);
	TEST_ASSERT_EQUAL(17, my_var);
	
	x->ip = -1;
	sloth__execute(x, sloth_to_abs(x, 3*sCELL));
	TEST_ASSERT_EQUAL(0, x->rp);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 3*sCELL), x->ip);
}

void my_exit(X* x) { x->ip = -1; }

void test_inner(void) {
	sloth_set(x, 3*sCELL, sloth_primitive(x, my_primitive));
	sloth_set(x, 4*sCELL, sloth_primitive(x, my_exit));
	x->ip = sloth_to_abs(x, 3*sCELL);
	my_var = 11;
	sloth__inner(x);
	TEST_ASSERT_EQUAL(17, my_var);
	TEST_ASSERT_EQUAL(-1, x->ip);
}

void test_eval(void) {
	sloth_set(x, 3*sCELL, sloth_primitive(x, my_primitive));
	sloth_set(x, 4*sCELL, sloth_primitive(x, my_exit));
	
	x->ip = sloth_to_abs(x, 3*sCELL);
	my_var = 11;
	sloth_eval(x, x->ip);
	TEST_ASSERT_EQUAL(17, my_var);
	TEST_ASSERT_EQUAL(-1, x->ip);
	
	my_var = 11;
	x->ip = -1;
	sloth_eval(x, x->ip);
	TEST_ASSERT_EQUAL(17, my_var);
}

void my_debug(X* x) { (void)x; my_var = 23; }

void test_debug_inner(void) {
	sloth_set(x, 3*sCELL, sloth_primitive(x, my_primitive));
	sloth_set(x, 4*sCELL, sloth_primitive(x, my_exit));
	sloth_primitive(x, &my_debug);
	my_var = 11;
	x->ip = sloth_to_abs(x, 4*sCELL);
	sloth__debug_inner(x, -3);
	TEST_ASSERT_EQUAL(23, my_var);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 4*sCELL), x->s[0]);
}

int pre_var = 11;
void pre_xt(X* x) { (void)x; pre_var = 13; }
int inner_var = 17;
void inner_xt(X* x) { (void)x; inner_var = 19; }
int post_var = 23;
void post_xt(X* x) { (void)x; post_var = 27; }

void test_debug_(void) {
	sloth_primitive(x, my_exit);
	sloth_primitive(x, pre_xt);
	sloth_primitive(x, inner_xt);
	sloth_primitive(x, post_xt);
	
	sloth_push(x, -1);
	sloth_push(x, -2);
	sloth_push(x, -3);
	sloth_push(x, -4);
	
	sloth_debug_(x);
	
	TEST_ASSERT_EQUAL(13, pre_var);
	TEST_ASSERT_EQUAL(17, inner_var);
	TEST_ASSERT_EQUAL(27, post_var);
	
	sloth_set(x, 3*sCELL, -1);
	
	pre_var = 11;
	inner_var = 17;
	post_var = 23;
	
	sloth_push(x, sloth_to_abs(x, 3*sCELL));
	sloth_push(x, -2);
	sloth_push(x, -3);
	sloth_push(x, -4);
	
	sloth_debug_(x);
	
	TEST_ASSERT_EQUAL(13, pre_var);
	TEST_ASSERT_EQUAL(19, inner_var);
	TEST_ASSERT_EQUAL(27, post_var);
}

/* -- Exceptions --------------------------------------- */

void throw_42_prim(X* x) { sloth_throw(x, 42); }

void test_catch_no_throw(void) {
	CELL p = sloth_primitive(x, my_primitive);
	CELL e = sloth_catch(x, p);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(0, e);
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_catch_throw(void) {
	CELL p = sloth_primitive(x, throw_42_prim);
	CELL e = sloth_catch(x, p);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(42, e);
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_catch_restores_stack(void) {
	CELL p, e;
	sloth_push(x, 1);
	sloth_push(x, 2);
	p = sloth_primitive(x, throw_42_prim);
	e = sloth_catch(x, p);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(42, e);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, sloth_pop(x));
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void nested_throw_test_prim(X* x) {
	CELL p = sloth_primitive(x, throw_42_prim);
	sloth_push(x, sloth_catch(x, p));
}

void test_nested_catch(void) {
	CELL p = sloth_primitive(x, nested_throw_test_prim);
	CELL e =sloth_catch(x, p);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, e);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_catch_throw_prim(void) {
	CELL p = sloth_primitive(x, throw_42_prim);
	sloth_push(x, p);
	sloth_catch_(x);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	
	sloth_push(x, 0); /* No error */
	sloth_throw_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
}

/* -- Inner interpreter primitives -------------------- */

void test_exit_(void) {
	/* Test rp > 0 */
	sloth_rpush(x, 100);
	sloth_exit_(x);
	TEST_ASSERT_EQUAL(100, x->ip);
	TEST_ASSERT_EQUAL(0, x->rp);
	
	/* Test rp == 0 */
	sloth_exit_(x);
	TEST_ASSERT_EQUAL(-1, x->ip);
}

void test_lit_(void) {
	sloth_set(x, 3*sCELL, 42);
	x->ip = sloth_to_abs(x, 3*sCELL);
	sloth_lit_(x);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 4*sCELL), x->ip);
}

void test_rip_(void) {
	CELL base_ip = sloth_to_abs(x, 3*sCELL);
	sloth_set(x, 3*sCELL, 10 * sCELL); /* offset */
	x->ip = base_ip;
	sloth_rip_(x);
	TEST_ASSERT_EQUAL(base_ip + 9*sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(base_ip + sCELL, x->ip);
}

void test_branch_(void) {
	CELL base_ip = sloth_to_abs(x, 3*sCELL);
	sloth_set(x, 3*sCELL, 10 * sCELL); /* offset */
	x->ip = base_ip;
	sloth_branch_(x);
	TEST_ASSERT_EQUAL(base_ip + 10*sCELL, x->ip);
}

void test_zbranch_(void) {
	CELL base_ip = sloth_to_abs(x, 3*sCELL);
	sloth_set(x, 3*sCELL, 10 * sCELL);
	
	/* Case 0: TOS is 0 (should branch) */
	x->ip = base_ip;
	sloth_push(x, 0);
	sloth_zbranch_(x);
	TEST_ASSERT_EQUAL(base_ip + 10*sCELL, x->ip);
	
	/* Case 1: TOS is NOT 0 (should NOT branch) */
	x->ip = base_ip;
	sloth_push(x, 1);
	sloth_zbranch_(x);
	TEST_ASSERT_EQUAL(base_ip + sCELL, x->ip);
}

/* -- Arithmetic and logical operations ---------------- */

/* These words implement ANS Forth tests adapted to C */

#define SLOTH_MAXUINT				((uCELL)~0)
#define SLOTH_MAXINT				(((uCELL)~0)>>1)
#define SLOTH_MININT				(~(((uCELL)~0)>>1))
#define SLOTH_MIDUINT				((((uCELL)~0))>>1)
#define SLOTH_MIDUINTplus1  (~(((uCELL)~0)>>1))

#define S0						0
#define S1						((uCELL)~0)

#define MSB						((CELL)~(((uCELL)~0) >> 1))

void test_invert_(void) {
	sloth_push(x, 0); sloth_invert_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, -1); sloth_invert_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_and_(void) {
	sloth_push(x, 0); sloth_push(x, 0); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, 1); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 1); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 0); sloth_invert_(x); sloth_push(x, 1); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 1); sloth_invert_(x); sloth_push(x, 1); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, S0); sloth_push(x, S0); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S0, sloth_pop(x));

	sloth_push(x, S0); sloth_push(x, S1); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S0, sloth_pop(x));

	sloth_push(x, S1); sloth_push(x, S0); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S0, sloth_pop(x));

	sloth_push(x, S1); sloth_push(x, S1); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S1, sloth_pop(x));
}

void test_l_shift_(void) {
	sloth_push(x, 1); sloth_push(x, 0); sloth_l_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 1); sloth_l_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 2); sloth_l_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(4, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0xF); sloth_l_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0x8000, sloth_pop(x));

	sloth_push(x, S1); sloth_push(x, 1); sloth_l_shift_(x);
	sloth_push(x, sloth_pop(x) ^ 1);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S1, sloth_pop(x));

	sloth_push(x, MSB); sloth_push(x, 1); sloth_l_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_minus_(void) {
	sloth_push(x, 0); sloth_push(x, 5); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-5, sloth_pop(x));

	sloth_push(x, 5); sloth_push(x, 0); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(5, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, -5); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(5, sloth_pop(x));

	sloth_push(x, -5); sloth_push(x, 0); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-5, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 2); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, -2); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(3, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, 2); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-3, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, -2); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, 1); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1); sloth_push(x, 1); sloth_minus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MIDUINT, sloth_pop(x));
}

void test_plus_(void) {
	sloth_push(x, 0); sloth_push(x, 5); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(5, sloth_pop(x));

	sloth_push(x, 5); sloth_push(x, 0); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(5, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, -5); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-5, sloth_pop(x));

	sloth_push(x, -5); sloth_push(x, 0); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-5, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 2); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(3, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, -2); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, 2); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, -2); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-3, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, 1); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINT); sloth_push(x, 1); sloth_plus_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MIDUINTplus1, sloth_pop(x));
}

void test_r_shift_(void) {
	sloth_push(x, 1); sloth_push(x, 0); sloth_r_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 1); sloth_r_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 2); sloth_push(x, 1); sloth_r_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 4); sloth_push(x, 2); sloth_r_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 0x8000); sloth_push(x, 0xF); sloth_r_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, MSB); sloth_push(x, 1); sloth_r_shift_(x);
	sloth_push(x, MSB); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, MSB); sloth_push(x, 1); sloth_r_shift_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(MSB, sloth_pop(x)*2);
}

void test_star_(void) {
	sloth_push(x, 0); sloth_push(x, 0); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	
	sloth_push(x, 0); sloth_push(x, 1); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 2); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));

	sloth_push(x, 2); sloth_push(x, 1); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));

	sloth_push(x, 3); sloth_push(x, 3); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(9, sloth_pop(x));

	sloth_push(x, -3); sloth_push(x, 3); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-9, sloth_pop(x));

	sloth_push(x, 3); sloth_push(x, -3); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-9, sloth_pop(x));

	sloth_push(x, -3); sloth_push(x, -3); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(9, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1); sloth_push(x, 1);
	sloth_r_shift_(x); sloth_push(x, 2); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MIDUINTplus1, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1); sloth_push(x, 2);
	sloth_r_shift_(x); sloth_push(x, 4); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MIDUINTplus1, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1); sloth_push(x, 1);
	sloth_r_shift_(x); sloth_push(x, SLOTH_MIDUINTplus1 | sloth_pop(x)); 
	sloth_push(x, 2); sloth_star_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MIDUINTplus1, sloth_pop(x));
}

void test_two_slash_(void) {
	sloth_push(x, S0); sloth_two_slash_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S0, sloth_pop(x));

	sloth_push(x, 1); sloth_two_slash_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0x4000); sloth_two_slash_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0x2000, sloth_pop(x));

	sloth_push(x, S1); sloth_two_slash_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S1, sloth_pop(x));

	sloth_push(x, S1^1); sloth_two_slash_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(S1, sloth_pop(x));

	sloth_push(x, MSB); sloth_two_slash_(x);
	sloth_push(x, MSB); sloth_and_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(MSB, sloth_pop(x));
}

void test_u_m_star_(void) {
	sloth_push(x, 0); sloth_push(x, 0); sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, 1); sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0); sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 2); sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(2, sloth_pop(x));

	sloth_push(x, 2); sloth_push(x, 1); sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(2, sloth_pop(x));

	sloth_push(x, 3); sloth_push(x, 3); sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(9, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1>>1); sloth_push(x, 2);
	sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(SLOTH_MIDUINTplus1, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1); sloth_push(x, 2);
	sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, SLOTH_MIDUINTplus1); sloth_push(x, 4);
	sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, S1); sloth_push(x, 2);
	sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));
	TEST_ASSERT_EQUAL(S1<<1, sloth_pop(x));

	sloth_push(x, SLOTH_MAXUINT); sloth_push(x, SLOTH_MAXUINT);
	sloth_u_m_star_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(~1, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, sloth_pop(x));
}

void test_u_m_slash_mod_(void) {
	sloth_push(x, 0); sloth_push(x, 0); sloth_push(x, 1);
	sloth_u_m_slash_mod_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0); sloth_push(x, 2);
	sloth_u_m_slash_mod_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, 3); sloth_push(x, 0); sloth_push(x, 2);
	sloth_u_m_slash_mod_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	sloth_push(x, SLOTH_MAXUINT); sloth_push(x, 2); sloth_u_m_star_(x);
	sloth_push(x, 2);	sloth_u_m_slash_mod_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MAXUINT, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, SLOTH_MAXUINT); sloth_push(x, 2); sloth_u_m_star_(x);
	sloth_push(x, SLOTH_MAXUINT);	sloth_u_m_slash_mod_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(2, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, SLOTH_MAXUINT); sloth_push(x, SLOTH_MAXUINT); 
	sloth_u_m_star_(x);	
	sloth_push(x, SLOTH_MAXUINT);	sloth_u_m_slash_mod_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(SLOTH_MAXUINT, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

}

/* -- Comparison operators ----------------------------- */

void test_equals_(void) {
	sloth_push(x, 0); sloth_push(x, 0); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 1); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, -1); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, 0); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, 1); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, -1); sloth_equals_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_less_than_(void) {
	sloth_push(x, 0); sloth_push(x, 1); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 2); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, 0); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, -1); sloth_push(x, 1); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, SLOTH_MININT); sloth_push(x, 0); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, SLOTH_MININT); sloth_push(x, SLOTH_MAXINT); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, SLOTH_MAXINT); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, 0); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 1); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, 0); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 2); sloth_push(x, 1); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, -1); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 1); sloth_push(x, -1); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, 0); sloth_push(x, SLOTH_MININT); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, SLOTH_MAXINT); sloth_push(x, SLOTH_MININT); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	sloth_push(x, SLOTH_MAXINT); sloth_push(x, 0); sloth_less_than_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

/* -- Strings ------------------------------------------ */

void test_string_(void) {
	sloth_set(x, 100, 5);
	x->ip = sloth_to_abs(x, 100);
	sloth_string_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(5, sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100 + sCELL), sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_aligned(sloth_to_abs(x, 100 + sCELL + 5*suCHAR)), x->ip);
}

void test_c_string_(void) {
	sloth_c_store(x, sloth_to_abs(x, 100), 5);
	x->ip = sloth_to_abs(x, 100);
	sloth_c_string_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100), sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_aligned(sloth_to_abs(x, 100 + 6*suCHAR)), x->ip);
}

void do_test_move(X* x, CELL b, CELL a1, CELL a2, CELL u, CELL v1, CELL v2, CELL v3) {
	sloth_push(x, sloth_to_abs(x, a1));
	sloth_push(x, sloth_to_abs(x, a2));
	sloth_push(x, u);
	sloth_move_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(v1, *((BYTE_*)sloth_to_abs(x, b + 0)));
	TEST_ASSERT_EQUAL(v2, *((BYTE_*)sloth_to_abs(x, b + 1)));
	TEST_ASSERT_EQUAL(v3, *((BYTE_*)sloth_to_abs(x, b + 2)));
}

void test_move_(void) {
	/* Reuses the ANS Forth tests for MOVE */
	int fbuf = 100;
	int sbuf = 103;

	*((BYTE_*)x->d + fbuf + 0) = 20;
	*((BYTE_*)x->d + fbuf + 1) = 20;
	*((BYTE_*)x->d + fbuf + 2) = 20;

	*((BYTE_*)x->d + sbuf + 0) = 12;
	*((BYTE_*)x->d + sbuf + 1) = 34;
	*((BYTE_*)x->d + sbuf + 2) = 56;

	do_test_move(x, fbuf, fbuf, fbuf, 3, 20, 20, 20);
	do_test_move(x, fbuf, sbuf, fbuf, 0, 20, 20, 20);
	do_test_move(x, fbuf, sbuf, fbuf, 1, 12, 20, 20);
	do_test_move(x, fbuf, sbuf, fbuf, 3, 12, 34, 56);
	do_test_move(x, fbuf, fbuf, fbuf + 1, 2, 12, 12, 34);
	do_test_move(x, fbuf, fbuf + 1, fbuf, 2, 12, 34, 34);
}

/* -- Searching ---------------------------------------- */

void test_searching(void) {
	char name[] = "FIND-ME";
	CELL w, cstring;
	size_t i;

	w = sloth_header(x, (CELL)name, (CELL)strlen(name));
	sloth_set_xt(x, w, 123);
	
	TEST_ASSERT_TRUE(sloth__compare(x, (CELL)name, strlen(name), (CELL)"find-me", strlen("find-me")));
	TEST_ASSERT_FALSE(sloth__compare(x, (CELL)name, strlen(name), (CELL)"other", 5));
	
	TEST_ASSERT_EQUAL(w, sloth__search_word(x, (CELL)name, strlen(name)));
	TEST_ASSERT_EQUAL(w, sloth_find_word(x, name));
	
	/* Test sloth_find_ (primitive) */
	cstring = sloth_here(x);
	sloth_c_comma(x, strlen(name));
	for(i=0; i<strlen(name); i++) sloth_c_comma(x, name[i]);
	
	sloth_push(x, cstring);
	sloth_find_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x)); /* Not immediate */
	TEST_ASSERT_EQUAL(123, sloth_pop(x));
	
	sloth_immediate_(x);
	sloth_push(x, cstring);
	sloth_find_(x);
	TEST_ASSERT_EQUAL(1, sloth_pop(x)); /* Immediate */
	TEST_ASSERT_EQUAL(123, sloth_pop(x));
}

/* -- Compile and literal ------------------------------ */

void test_compile_and_literal(void) {
	CELL h;

	/* Create (LIT) to ensure it's correctly compiled */
	sloth_code(x, "(LIT)", 999);

	sloth_literal(x, 42);
	h = sloth_here(x);
	TEST_ASSERT_EQUAL(999, sloth_fetch(x, h - 2*sCELL));
	TEST_ASSERT_EQUAL(42, sloth_fetch(x, h - sCELL));
	
	sloth_compile(x, 777);
	TEST_ASSERT_EQUAL(777, sloth_fetch(x, sloth_here(x) - sCELL));
}

/* -- Quotations ---------------------------------------- */

void test_quotation_primitive(void) {
	sloth_set(x, 100, 3*sCELL);
	x->ip = sloth_to_abs(x, 100);
	sloth_quotation_(x);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100 + 4*sCELL), x->ip);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100 + sCELL), sloth_pop(x));
}

void test_start_quotation_interpret_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 0);
	sloth_start_quotation_(x);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(-1, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(here + sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + 2*sCELL, sloth_pop(x));
}

void test_start_nested_quotation_interpret_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 0);
	sloth_start_quotation_(x);
	sloth_start_quotation_(x);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(-2, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(5, x->sp);
	TEST_ASSERT_EQUAL(here + 3*sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + 2*sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + 2*sCELL, sloth_pop(x));
}

void test_start_quotation_compile_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_start_quotation_(x);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(2, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(here + sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_start_nested_quotation_compile_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_start_quotation_(x);
	sloth_start_quotation_(x);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(3, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(4, x->sp);
	TEST_ASSERT_EQUAL(here + 3*sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + 2*sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_end_quotation_interpret_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 0);
	sloth_start_quotation_(x);
	sloth_end_quotation_(x);

	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(sCELL, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(here + 2*sCELL, sloth_pop(x));
}

void test_end_nested_quotation_interpret_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 0);
	sloth_start_quotation_(x);
	sloth_start_quotation_(x);
	sloth_end_quotation_(x);

	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(sCELL, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, here + 4*sCELL));
	TEST_ASSERT_EQUAL(-1, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(here + sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(here + 2*sCELL, sloth_pop(x));
}

void test_end_quotation_compile_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_start_quotation_(x);
	sloth_end_quotation_(x);

	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(sCELL, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(1, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(0, x->sp);
}

void test_end_nested_quotation_compile_mode(void) {
	CELL here;

	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_start_quotation_(x);
	sloth_start_quotation_(x);
	sloth_end_quotation_(x);
	sloth_end_quotation_(x);

	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(sCELL, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, here + 4*sCELL));
	TEST_ASSERT_EQUAL(1, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(0, x->sp);
}

/* -- User variable creation --------------------------- */

void test_user_variable_creation(void) {
	sloth_code(x, "EXIT", 111);
	sloth_code(x, "(LIT)", 222);
	sloth_user_variable(x, "TEST-VAR", 8, 13);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, sloth_here(x) - 1*sCELL));
	TEST_ASSERT_EQUAL(x->u + 8, sloth_fetch(x, sloth_here(x) - 2*sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, sloth_here(x) - 3*sCELL));
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, x->u + 8));
}

/* -- Source code preprocessing, interpreting & auditing commands */

void test_file_position(void) {
	/* TODO A non-valid file pointer can't be detected right now */
	FILE *f = tmpfile();
	assert(f);

	fwrite("abc", 1, 3, f);
	fflush(f);

	sloth_push(x, (CELL)f);
	sloth_file_position_(x);
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(3, sloth_pop(x));

	fclose(f);
}

void test_read_line(void) {
	char buf[16];
	FILE *f = tmpfile();
	assert(f);

	fwrite("abc", 1, 3, f);
	fflush(f);

	/* Test read at end of file */
	sloth_push(x, (CELL)buf);
	sloth_push(x, 16);
	sloth_push(x, (CELL)f);

	sloth_read_line_(x);

	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	/* Test correct read */
	fseek(f, 0, SEEK_SET);

	sloth_push(x, (CELL)buf);
	sloth_push(x, 16);
	sloth_push(x, (CELL)f);

	sloth_read_line_(x);

	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_NOT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(3, sloth_pop(x));
	TEST_ASSERT_EQUAL('a', buf[0*suCHAR]);
	TEST_ASSERT_EQUAL('b', buf[1*suCHAR]);
	TEST_ASSERT_EQUAL('c', buf[2*suCHAR]);

	fclose(f);
}

void my_accept(X* x) { sloth_push(x, 15); }

void test_refill(void) {
	CELL ibuf;

	sloth_user_set(x, SLOTH_SOURCE_ID, -1);
	sloth_refill_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	ibuf = sloth_user_get(x, SLOTH_IBUF);
	sloth_code(x, "ACCEPT", sloth_primitive(x, &my_accept));
	sloth_user_set(x, SLOTH_SOURCE_ID, 0);
	sloth_refill_(x);	
	TEST_ASSERT_EQUAL(ibuf, sloth_user_get(x, SLOTH_IBUF));
	TEST_ASSERT_EQUAL(15, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_IPOS));
}

void test_refill_file(void) {
	char ibuf[1024];
	FILE *f = tmpfile();
	assert(f);

	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 1024);

	fwrite("abc\ndefg", 1, 8, f);
	fflush(f);

	fseek(f, 0, SEEK_SET);

	sloth_user_set(x, SLOTH_SOURCE_ID, (CELL)f);

	sloth_refill_(x);

	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_IPOS));
	TEST_ASSERT_EQUAL(3, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL_STRING_LEN("abc", sloth_user_get(x, SLOTH_IBUF), 3);

	sloth_push(x, (CELL)f);
	sloth_file_position_(x);
	sloth_pop(x);
	sloth_pop(x);
	TEST_ASSERT_EQUAL(4, sloth_pop(x));

	sloth_refill_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x));

	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_IPOS));
	TEST_ASSERT_EQUAL(4, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL_STRING_LEN("defg", sloth_user_get(x, SLOTH_IBUF), 4);

	sloth_push(x, (CELL)f);
	sloth_file_position_(x);
	sloth_pop(x);
	sloth_pop(x);
	TEST_ASSERT_EQUAL(8, sloth_pop(x));

	fclose(f);
}

/* Portable temp file creation code */

static int write_temp_file(char *pathbuf, const char *content, size_t len) {
	FILE *f;
	/* tmpnam is C89 but not thread-safe — acceptable */
	/* for single-threaded tests */
	if (!tmpnam(pathbuf)) return -1;
	f = fopen(pathbuf, "wb");
	if (!f) return -1;
	fwrite(content, 1, len, f);
	fclose(f);
	return 0;
}

int interpret_calls;

void noop_interpret(X* x) {
	(void)x;
	interpret_calls++;
}

void test_included_absolute_path(void) {
	char tmppath[MAX_PATH];
	char path[256];
	CELL saved_incl, new_head;
	CELL ibuf, ipos, ilen, source, source_pos;

	/* Set PATH_START and PATH_END variables */
	sloth_user_set(x, SLOTH_PATH_START, (CELL)path);
	sloth_user_set(x, SLOTH_PATH_END, (CELL)path);
	
	TEST_ASSERT_EQUAL(0, write_temp_file(tmppath, "line one\nline two", 17));

	sloth_user_set(x, SLOTH_INTERPRET, sloth_primitive(x, &noop_interpret));
	interpret_calls = 0;

	saved_incl = sloth_user_get(x, SLOTH_INCLUDED_FILES);

	ibuf = sloth_user_get(x, SLOTH_IBUF);
	ipos = sloth_user_get(x, SLOTH_IPOS);
	ilen = sloth_user_get(x, SLOTH_ILEN);
	source = sloth_user_get(x, SLOTH_SOURCE_ID);
	source_pos = sloth_user_get(x, SLOTH_SOURCE_POS);

	sloth_push(x, (CELL)tmppath);
	sloth_push(x, strlen(tmppath));
	sloth_included_(x);

	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(2, interpret_calls);

	TEST_ASSERT_EQUAL(source_pos, sloth_user_get(x, SLOTH_SOURCE_POS));
	TEST_ASSERT_EQUAL(source, sloth_user_get(x, SLOTH_SOURCE_ID));
	TEST_ASSERT_EQUAL(ilen, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL(ipos, sloth_user_get(x, SLOTH_IPOS));
	TEST_ASSERT_EQUAL(ibuf, sloth_user_get(x, SLOTH_IBUF));

	/* A new entry must have been prepended to INCLUDED_FILES */
	new_head = sloth_user_get(x, SLOTH_INCLUDED_FILES);
	TEST_ASSERT_NOT_EQUAL(saved_incl, new_head);
	TEST_ASSERT_EQUAL(saved_incl, sloth_fetch(x, new_head));            /* link to prev */
	TEST_ASSERT_EQUAL((CELL)strlen(tmppath), sloth_fetch(x, new_head + sCELL)); /* name len */
	TEST_ASSERT_EQUAL_MEMORY(tmppath, (char*)(new_head + 2*sCELL), strlen(tmppath)); /* name */

	remove(tmppath);
}

void test_included_file_not_found(void) {
	char path[256];
	char missing[] = "sloth_test_no_such_file.4th";
	CELL throw_prim;

	sloth_user_set(x, SLOTH_PATH_START, (CELL)path);
	sloth_user_set(x, SLOTH_PATH_END, (CELL)path);
	
	throw_prim = sloth_primitive(x, &sloth_included_);
	sloth_push(x, (CELL)missing);
	sloth_push(x, (CELL)strlen(missing));
	sloth_push(x, throw_prim);
	sloth_catch_(x);
	TEST_ASSERT_EQUAL(-38, sloth_pop(x));
	/* After throwing, the stack is restored to */
	/* the depth previous to the catch. */
	TEST_ASSERT_EQUAL(2, x->sp);
}

void test_included_relative_path(void) {
	char tmppath[MAX_PATH];
	char path[256];
	char *filename, *sep;
	size_t dirlen;
	
	sloth_user_set(x, SLOTH_INTERPRET, sloth_primitive(x, &noop_interpret));
	sloth_user_set(x, SLOTH_ROOT_PATH_LENGTH, 0);
	interpret_calls = 0;
	
	TEST_ASSERT_EQUAL(0, write_temp_file(tmppath, "line one\nline two", 17));
	
	/* Split tmppath into directory and filename */
	sep = strrchr(tmppath, '/');
#ifdef _WIN32
	char *sep2 = strrchr(tmppath, '\\');
	if (sep2 > sep) sep = sep2;
#endif
	TEST_ASSERT_NOT_NULL(sep);
	filename = sep + 1;
	dirlen = filename - tmppath;  /* includes the trailing separator */

	/* Simulate a previous include having set PATH_START/PATH_END
	   to the directory containing our temp file */
	memcpy(path, tmppath, dirlen);
	sloth_user_set(x, SLOTH_PATH_START, (CELL)path);
	sloth_user_set(x, SLOTH_PATH_END,   (CELL)(path + dirlen));
	
	/* Push only the filename (no directory) */
	sloth_push(x, (CELL)filename);
	sloth_push(x, (CELL)strlen(filename));
	sloth_included_(x);
	
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(2, interpret_calls);
	
	remove(tmppath);
} 

void test_included_root_path(void) {
	char tmppath[MAX_PATH];
	char path[MAX_PATH];
	char rootpath[MAX_PATH];
	char *filename, *sep;
	size_t dirlen;
	
	sloth_user_set(x, SLOTH_INTERPRET, sloth_primitive(x, &noop_interpret));
	interpret_calls = 0;
	
	TEST_ASSERT_EQUAL(0, write_temp_file(tmppath, "line one\nline two", 17));
	
	/* Split tmppath into directory and filename */
	sep = strrchr(tmppath, '/');
#ifdef _WIN32
	char *sep2 = strrchr(tmppath, '\\');
	if (sep2 > sep) sep = sep2;
#endif
	TEST_ASSERT_NOT_NULL(sep);
	filename = sep + 1;
	dirlen = filename - tmppath;  /* includes the trailing separator */
	
	/* PATH_START/PATH_END point to an empty path so the first two
	   strategies (absolute and relative-to-previous) both fail */
	sloth_user_set(x, SLOTH_PATH_START, (CELL)path);
	sloth_user_set(x, SLOTH_PATH_END,   (CELL)path);
	
	/* Put the temp file's directory into SLOTH_PATHS as the root */
	memcpy(rootpath, tmppath, dirlen);
	memcpy((char*)(x->u + SLOTH_PATHS), rootpath, dirlen);
	sloth_user_set(x, SLOTH_ROOT_PATH_LENGTH, (CELL)dirlen);
	
	/* Push only the filename (no directory) */
	sloth_push(x, (CELL)filename);
	sloth_push(x, (CELL)strlen(filename));
	sloth_included_(x);
	
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(2, interpret_calls);
	
	remove(tmppath);
}

void refill_interpret(X* x) {
	sloth_refill_(x);
	sloth_pop(x);
	interpret_calls++;
}

void test_included_and_refill(void) {
	char tmppath[MAX_PATH];
	char path[256];
	CELL saved_incl, new_head;
	CELL ibuf, ipos, ilen, source, source_pos;

	/* Set PATH_START and PATH_END variables */
	sloth_user_set(x, SLOTH_PATH_START, (CELL)path);
	sloth_user_set(x, SLOTH_PATH_END, (CELL)path);
	
	TEST_ASSERT_EQUAL(0, write_temp_file(tmppath, "line one\nline two\nline three\nline four", 39));

	sloth_user_set(x, SLOTH_INTERPRET, sloth_primitive(x, &refill_interpret));
	interpret_calls = 0;

	saved_incl = sloth_user_get(x, SLOTH_INCLUDED_FILES);

	ibuf = sloth_user_get(x, SLOTH_IBUF);
	ipos = sloth_user_get(x, SLOTH_IPOS);
	ilen = sloth_user_get(x, SLOTH_ILEN);
	source = sloth_user_get(x, SLOTH_SOURCE_ID);
	source_pos = sloth_user_get(x, SLOTH_SOURCE_POS);

	sloth_push(x, (CELL)tmppath);
	sloth_push(x, strlen(tmppath));
	sloth_included_(x);

	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(2, interpret_calls);

	TEST_ASSERT_EQUAL(source_pos, sloth_user_get(x, SLOTH_SOURCE_POS));
	TEST_ASSERT_EQUAL(source, sloth_user_get(x, SLOTH_SOURCE_ID));
	TEST_ASSERT_EQUAL(ilen, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL(ipos, sloth_user_get(x, SLOTH_IPOS));
	TEST_ASSERT_EQUAL(ibuf, sloth_user_get(x, SLOTH_IBUF));

	/* A new entry must have been prepended to INCLUDED_FILES */
	new_head = sloth_user_get(x, SLOTH_INCLUDED_FILES);
	TEST_ASSERT_NOT_EQUAL(saved_incl, new_head);
	TEST_ASSERT_EQUAL(saved_incl, sloth_fetch(x, new_head));            /* link to prev */
	TEST_ASSERT_EQUAL((CELL)strlen(tmppath), sloth_fetch(x, new_head + sCELL)); /* name len */
	TEST_ASSERT_EQUAL_MEMORY(tmppath, (char*)(new_head + 2*sCELL), strlen(tmppath)); /* name */

	remove(tmppath);
}

/* -- Input/Output and parsing ------------------------- */

void test_emit(void) {
	emitted_char = 0;
	sloth_push(x, 'Z');
	custom_emit_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL('Z', emitted_char);

	/* Ensure that our custom emit is being used */
	sloth_primitive(x, custom_emit_);
	sloth_push(x, 'Z');
	sloth__do_prim(x, -1);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL('Z', emitted_char);
}

void test_key_pushes_char_onto_stack(void) {
	mock_key_char = 'Q';
	custom_key_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL('Q', sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->sp);

	/* Ensure that our custom key is being used */
	mock_key_char = 'Q';
	sloth_primitive(x, custom_key_);
	sloth__do_prim(x, -1);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL('Q', sloth_pop(x));
	TEST_ASSERT_EQUAL(0, x->sp);
}

void test_key_handles_zero(void) {
	mock_key_char = 0;
	custom_key_(x);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_key_handles_max_uchar(void) {
	mock_key_char = 255;
	custom_key_(x);
	TEST_ASSERT_EQUAL(255, sloth_pop(x));
}

void test_source_(void) {
	char *ibuf = "TEST";
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_ILEN, 4);
	sloth_source_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(4, sloth_pop(x));
	TEST_ASSERT_EQUAL((CELL)ibuf, sloth_pop(x));
}

void test_word_(void) {
	char *ibuf = "Hello world";
	CELL addr;
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 11);
	sloth_push(x, ' ');
	sloth_word_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	addr = sloth_pop(x);
	TEST_ASSERT_EQUAL(5, sloth_c_fetch(x, addr));
	TEST_ASSERT_EQUAL('H', sloth_c_fetch(x, addr + 1*suCHAR));
	TEST_ASSERT_EQUAL('e', sloth_c_fetch(x, addr + 2*suCHAR));
	TEST_ASSERT_EQUAL('l', sloth_c_fetch(x, addr + 3*suCHAR));
	TEST_ASSERT_EQUAL('l', sloth_c_fetch(x, addr + 4*suCHAR));
	TEST_ASSERT_EQUAL('o', sloth_c_fetch(x, addr + 5*suCHAR));

	sloth_push(x, ' ');
	sloth_word_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	addr = sloth_pop(x);
	TEST_ASSERT_EQUAL(5, sloth_c_fetch(x, addr));
	TEST_ASSERT_EQUAL('w', sloth_c_fetch(x, addr + 1*suCHAR));
	TEST_ASSERT_EQUAL('o', sloth_c_fetch(x, addr + 2*suCHAR));
	TEST_ASSERT_EQUAL('r', sloth_c_fetch(x, addr + 3*suCHAR));
	TEST_ASSERT_EQUAL('l', sloth_c_fetch(x, addr + 4*suCHAR));
	TEST_ASSERT_EQUAL('d', sloth_c_fetch(x, addr + 5*suCHAR));

	sloth_push(x, ' ');
	sloth_word_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_c_fetch(x, sloth_pop(x)));
}

/* -- Outer interpreter -------------------------------- */

void test_interpret_empty(void) {
	sloth_user_set(x, SLOTH_IBUF, 0);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 0);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
}

void test_interpret_character_literals_interpret_mode(void) {
	char *ibuf = "'a' 'b'";
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 7);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL('b', sloth_pop(x));
	TEST_ASSERT_EQUAL('a', sloth_pop(x));
}

void test_interpret_character_literals_compile_mode(void) {
	char *ibuf = "'a' 'b'";
	CELL here;
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 7);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_code(x, "(LIT)", -2);
	here = sloth_here(x);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL('a', sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL('b', sloth_fetch(x, here + 3*sCELL));
}

void test_interpret_number_literals_interpret_mode(void) {
	char *ibuf = "1 0 37 -560";
	sloth_user_set(x, SLOTH_BASE, 10);
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 11);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(4, x->sp);
	TEST_ASSERT_EQUAL(-560, sloth_pop(x));
	TEST_ASSERT_EQUAL(37, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, sloth_pop(x));

	ibuf = "%1111 #15 $f";
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 12);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(15, sloth_pop(x));
	TEST_ASSERT_EQUAL(15, sloth_pop(x));
	TEST_ASSERT_EQUAL(15, sloth_pop(x));
}

void test_interpret_number_literals_compile_mode(void) {
	char *ibuf = "1 0 37 -560";
	CELL here;
	sloth_user_set(x, SLOTH_BASE, 10);
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 11);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_code(x, "(LIT)", -2);
	here = sloth_here(x);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(1, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(37, sloth_fetch(x, here + 5*sCELL));
	TEST_ASSERT_EQUAL(-560, sloth_fetch(x, here + 7*sCELL));

	ibuf = "%1111 #15 $f";
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 12);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_code(x, "(LIT)", -2);
	here = sloth_here(x);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(15, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(15, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(15, sloth_fetch(x, here + 5*sCELL));
}

/* TODO Tests for floating point literals */

CELL p1 = 0, p2 = 0;

void primitive1(X* x) { (void)x; p1 = 1; }
void primitive2(X* x) { (void)x; p2 = 1; }

void test_interpret_(void) {
	char *ibuf = "11 PRIM1 13 PRIM2";
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 17);
	sloth_code(x, "PRIM1", sloth_primitive(x, &primitive1));
	sloth_code(x, "PRIM2", sloth_primitive(x, &primitive2));
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(13, sloth_pop(x));
	TEST_ASSERT_EQUAL(11, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, p1);
	TEST_ASSERT_EQUAL(1, p2);
}

void test_interpret_compile_mode(void) {
	char *ibuf = "11 PRIM1 13 PRIM2";
	CELL here;
	sloth_user_set(x, SLOTH_BASE, 10);
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 17);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_code(x, "EXIT", -1);
	sloth_code(x, "(LIT)", -2);
	sloth_code(x, "PRIM1", -3);
	sloth_code(x, "PRIM2", -4);
	here = sloth_here(x);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(-2, sloth_fetch(x, here + 0*sCELL));
	TEST_ASSERT_EQUAL(11, sloth_fetch(x, here + 1*sCELL));
	TEST_ASSERT_EQUAL(-3, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(-2, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, here + 4*sCELL));
	TEST_ASSERT_EQUAL(-4, sloth_fetch(x, here + 5*sCELL));
}

void test_interpret_immediate_words_compile_mode(void) {
	char *ibuf = "11 PRIM1 13 PRIM2";
	CELL here;
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 17);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_code(x, "PRIM1", sloth_primitive(x, &primitive1)); sloth_immediate_(x);
	sloth_code(x, "(LIT)", -2);
	sloth_code(x, "PRIM2", -3);
	here = sloth_here(x);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(-2, sloth_fetch(x, here + 0*sCELL));
	TEST_ASSERT_EQUAL(11, sloth_fetch(x, here + 1*sCELL));
	TEST_ASSERT_EQUAL(-2, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, here + 3*sCELL));
	TEST_ASSERT_EQUAL(-3, sloth_fetch(x, here + 4*sCELL));

	TEST_ASSERT_EQUAL(1, p1);
}

/* -- Defining words ----------------------------------- */

void test_colon_(void) {
	char *name = "TEST";
	CELL here = sloth_here(x);
	sloth_user_set(x, SLOTH_IBUF, (CELL)name);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 11);
	sloth_colon_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(here, sloth_get_latest(x));
	TEST_ASSERT_EQUAL(sloth_here(x), sloth_user_get(x, SLOTH_LATESTXT));
	TEST_ASSERT_EQUAL(SLOTH_HIDDEN, sloth_get_flags(x, sloth_get_latest(x)));
	TEST_ASSERT_EQUAL(1, sloth_user_get(x, SLOTH_STATE));
}

void test_colon_no_name_(void) {
	sloth_colon_no_name_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(sloth_here(x), sloth_user_get(x, SLOTH_LATESTXT));
	TEST_ASSERT_EQUAL(1, sloth_user_get(x, SLOTH_STATE));
}

void test_semicolon_(void) {
	char *name = "TEST";
	sloth_user_set(x, SLOTH_IBUF, (CELL)name);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 11);
	sloth_colon_(x);
	sloth_code(x, "EXIT", -1);
	sloth_semicolon_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_STATE));
}

void test_recurse_(void) {
	CELL here = sloth_here(x);
	sloth_user_set(x, SLOTH_LATESTXT, 1111);
	sloth_recurse_(x);
	TEST_ASSERT_EQUAL(1111, sloth_fetch(x, here));
}

void test_immediate_(void) {
	char name[] = "IMM-WORD";
	CELL w = sloth_header(x, (CELL)name, (CELL)strlen(name));
	TEST_ASSERT_FALSE(sloth_has_flag(x, w, SLOTH_IMMEDIATE));
	sloth_immediate_(x);
	TEST_ASSERT_TRUE(sloth_has_flag(x, w, SLOTH_IMMEDIATE));
}

CELL postpone_p1;
void postpone_primitive1(X* x) {
	(void)x;
	postpone_p1 = 1;
}

void test_postpone_(void) {
	char *ibuf = ": TEST IMM-WORD ;";
	char *ibuf2 = ": TEST POSTPONE IMM-WORD ;";
	char *ibuf3 = "TEST";
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 17);
	postpone_p1 = 0;
	sloth_code(x, "EXIT", sloth_primitive(x, &sloth_exit_));
	sloth_code(x, "(LIT)", sloth_primitive(x, &sloth_lit_));
	sloth_code(x, "(RIP)", sloth_primitive(x, &sloth_rip_));
	sloth_code(x, ":", sloth_primitive(x, &sloth_colon_));
	sloth_code(x, ";", sloth_primitive(x, &sloth_semicolon_)); sloth_immediate_(x);
	sloth_code(x, "COMPILE,", sloth_primitive(x, &sloth_compile_comma_));
	sloth_code(x, "POSTPONE", sloth_primitive(x, &sloth_postpone_)); sloth_immediate_(x);
	sloth_code(x, "IMM-WORD", sloth_primitive(x, &postpone_primitive1)); sloth_immediate_(x);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(1, postpone_p1);

	/* Test that IMM-WORD is not executed when POSTPONEd */
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf2);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 26);
	postpone_p1 = 0;
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(0, postpone_p1);

	/* Test that IMM-WORD was compiled when POSTPONEd */
	sloth_user_set(x, SLOTH_IBUF, (CELL)ibuf3);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 4);
	sloth_interpret_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(1, postpone_p1);
}

void test_compile_comma_(void) {
	CELL here = sloth_here(x);
	sloth_push(x, 123);
	sloth_compile_comma_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(123, sloth_fetch(x, here));
}

void test_create_(void) {
	char *name = "TEST";
	CELL here, xt;
	sloth_user_set(x, SLOTH_IBUF, (CELL)name);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 4);
	sloth_code(x, "(RIP)", -2);
	sloth_code(x, "EXIT", -1);
	here = sloth_here(x);
	sloth_create_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	xt = sloth_fetch(x, here + sCELL);
	TEST_ASSERT_EQUAL(-2, sloth_fetch(x, xt));
	TEST_ASSERT_EQUAL(4*sCELL, sloth_fetch(x, xt+sCELL));
	TEST_ASSERT_EQUAL(-1, sloth_fetch(x, xt+2*sCELL));
	TEST_ASSERT_EQUAL(-1, sloth_fetch(x, xt+3*sCELL));
}

void test_do_does_(void) {
	char *name = "TEST";
	CELL here, xt;
	sloth_user_set(x, SLOTH_IBUF, (CELL)name);
	sloth_user_set(x, SLOTH_IPOS, 0);
	sloth_user_set(x, SLOTH_ILEN, 4);
	sloth_code(x, "(RIP)", -2);
	sloth_code(x, "EXIT", -1);
	here = sloth_here(x);
	sloth_create_(x);
	xt = sloth_fetch(x, here + sCELL);
	TEST_ASSERT_EQUAL(-1, sloth_fetch(x, xt+2*sCELL));
	sloth_push(x, 13);
	sloth_do_does_(x);
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, xt+2*sCELL));
}

void test_does_(void) {
	CELL here;
	sloth_code(x, "(LIT)", -3);
	sloth_code(x, "(DOES)", -2);
	sloth_code(x, "EXIT", -1);
	here = sloth_here(x);
	sloth_does_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(-3, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(here + 4*sCELL, sloth_fetch(x, here+sCELL));
	TEST_ASSERT_EQUAL(-2, sloth_fetch(x, here+2*sCELL));
	TEST_ASSERT_EQUAL(-1, sloth_fetch(x, here+3*sCELL));
}

void test_evaluate_(void) {
	char *buf = "11 12 +";
	sloth_push(x, (CELL)buf);
	sloth_push(x, 7);
	sloth_code(x, "+", sloth_primitive(x, &sloth_plus_));
	sloth_user_set(x, SLOTH_INTERPRET, sloth_primitive(x, &sloth_interpret_));
	sloth_evaluate_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(23, sloth_pop(x));
}

CELL p0;

void primitive0(X* x) { (void)x; p0 = 1; }

void test_execute_(void) {
	p0 = 0;
	sloth_push(x, sloth_primitive(x, &primitive0));
	sloth_execute_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(1, p0);
}

/* -- Bootstrapping ------------------------------------ */

void test_bootstrap(void) {
	sloth_bootstrap(x);

	TEST_ASSERT_NOT_EQUAL(0, sloth_find_word(x, "EXIT"));
	TEST_ASSERT_NOT_EQUAL(0, sloth_find_word(x, "DUP"));
	TEST_ASSERT_NOT_EQUAL(0, sloth_find_word(x, "@"));
}

void test_bootstrap_user_area(void) {
	sloth_bootstrap(x);

	TEST_ASSERT_EQUAL(sloth_to_abs(x, SLOTH_FORTH_WL), sloth_user_get(x, SLOTH_CURRENT));
	TEST_ASSERT_EQUAL(2, sloth_user_get(x, SLOTH_ORDER));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_LOCALS_WORDLIST));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, SLOTH_FORTH_WL), sloth_user_get(x, SLOTH_CONTEXT));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, SLOTH_INTERNAL_WL), sloth_user_get(x, SLOTH_CONTEXT + sCELL));
	TEST_ASSERT_EQUAL(10, sloth_user_get(x, SLOTH_BASE));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_IBUF));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_IPOS));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_SOURCE_ID));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_SOURCE_POS));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_LATESTXT));
	TEST_ASSERT_NOT_EQUAL(0, sloth_user_get(x, SLOTH_INTERPRET));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_ROOT_PATH_LENGTH));
	TEST_ASSERT_EQUAL(x->u + SLOTH_PATHS, sloth_user_get(x, SLOTH_PATH_START));
	TEST_ASSERT_EQUAL(x->u + SLOTH_PATHS, sloth_user_get(x, SLOTH_PATH_END));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_PATHS));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_INCLUDED_FILES));
}

int main(void) {
	UNITY_BEGIN();
	/* Sloth VM tests */
	RUN_TEST(test_context_init);
	RUN_TEST(test_new);
	RUN_TEST(test_data_stack);
	RUN_TEST(test_over_);
	RUN_TEST(test_return_stack);
	RUN_TEST(test_memory);
	RUN_TEST(test_allot_);
	RUN_TEST(test_here_);
	RUN_TEST(test_compilation);
	/* Headers */
	RUN_TEST(test_get_set_latest);
	RUN_TEST(test_get_link);
	RUN_TEST(test_get_set_xt);
	RUN_TEST(test_get_set_flags);
	RUN_TEST(test_has_set_unset_flag);
	RUN_TEST(test_headers);
	RUN_TEST(test_name_and_len);
	/* Primitive and word creation */
	RUN_TEST(test_primitive_and_word_creation);
	/* Inner interpreter */
	RUN_TEST(test_op);
	RUN_TEST(test_do_prim);
	RUN_TEST(test_call);
	RUN_TEST(test_execute);
	RUN_TEST(test_inner);
	RUN_TEST(test_eval);
	RUN_TEST(test_debug_inner);
	RUN_TEST(test_debug_);
	/* Exceptions */
	RUN_TEST(test_catch_no_throw);
	RUN_TEST(test_catch_throw);
	RUN_TEST(test_catch_restores_stack);
	RUN_TEST(test_nested_catch);
	RUN_TEST(test_catch_throw_prim);
	/* Inner interpreter primitives */
	RUN_TEST(test_exit_);
	RUN_TEST(test_lit_);
	RUN_TEST(test_rip_);
	RUN_TEST(test_branch_);
	RUN_TEST(test_zbranch_);
	/* Arithmetic and logical operations */
	RUN_TEST(test_invert_);
	RUN_TEST(test_and_);
	RUN_TEST(test_l_shift_);
	RUN_TEST(test_minus_);
	RUN_TEST(test_plus_);
	RUN_TEST(test_r_shift_);
	RUN_TEST(test_star_);
	RUN_TEST(test_two_slash_);
	RUN_TEST(test_u_m_star_);
	RUN_TEST(test_u_m_slash_mod_);
	/* Comparison operators */
	RUN_TEST(test_equals_);
	RUN_TEST(test_less_than_);
	/* Strings */
	RUN_TEST(test_string_);
	RUN_TEST(test_c_string_);
	RUN_TEST(test_move_);
	/* Searching */
	RUN_TEST(test_searching);
	/* Compile and literal */
	RUN_TEST(test_compile_and_literal);
	/* Quotations */
	RUN_TEST(test_quotation_primitive);
	RUN_TEST(test_start_quotation_interpret_mode);
	RUN_TEST(test_start_nested_quotation_interpret_mode);
	RUN_TEST(test_start_quotation_compile_mode);
	RUN_TEST(test_start_nested_quotation_compile_mode);
	RUN_TEST(test_end_quotation_interpret_mode);
	RUN_TEST(test_end_nested_quotation_interpret_mode);
	RUN_TEST(test_end_quotation_compile_mode);
	RUN_TEST(test_end_nested_quotation_compile_mode);
	/* User variable creation */
	RUN_TEST(test_user_variable_creation);
	/* Source code preprocessing, interpreting & auditing commands */
	RUN_TEST(test_file_position);
	RUN_TEST(test_read_line);
	RUN_TEST(test_refill);
	RUN_TEST(test_refill_file);
	/* TODO Test save_input_and_path and restore_input_and_path */
	RUN_TEST(test_included_absolute_path);
	RUN_TEST(test_included_file_not_found);
	RUN_TEST(test_included_relative_path);
	RUN_TEST(test_included_root_path);
	RUN_TEST(test_included_and_refill);
	/* Input/output and parsing */
	RUN_TEST(test_emit);
	RUN_TEST(test_key_pushes_char_onto_stack);
	RUN_TEST(test_key_handles_zero);
	RUN_TEST(test_key_handles_max_uchar);
	RUN_TEST(test_source_);
	RUN_TEST(test_word_);
	/* Defining words */
	RUN_TEST(test_colon_);
	RUN_TEST(test_colon_no_name_);
	RUN_TEST(test_semicolon_);
	RUN_TEST(test_recurse_);
	RUN_TEST(test_immediate_);
	RUN_TEST(test_postpone_);
	RUN_TEST(test_compile_comma_);
	RUN_TEST(test_create_);
	RUN_TEST(test_do_does_);
	RUN_TEST(test_does_);
	RUN_TEST(test_evaluate_);
	RUN_TEST(test_execute_);
	/* Outer interpreter */
	RUN_TEST(test_interpret_empty);
	RUN_TEST(test_interpret_character_literals_interpret_mode);
	RUN_TEST(test_interpret_character_literals_compile_mode);
	RUN_TEST(test_interpret_number_literals_interpret_mode);
	RUN_TEST(test_interpret_number_literals_compile_mode);
	RUN_TEST(test_interpret_);
	RUN_TEST(test_interpret_compile_mode);
	RUN_TEST(test_interpret_immediate_words_compile_mode);
	/* Bootstrap */
	RUN_TEST(test_bootstrap);
	RUN_TEST(test_bootstrap_user_area);
	return UNITY_END();
}
