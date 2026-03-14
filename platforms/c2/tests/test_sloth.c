#define SLOTH_IMPLEMENTATION
#include "sloth.h"
#include "unity.h"

X* x;

void setUp() {
	/* Create a VM with 64 primitive slots and 1024 bytes */
	/* of dictionary. */
	x = sloth_create(64, 1024, 256);
}

void tearDown() {
	sloth_free(x);
}

void test_context_init() {
	TEST_ASSERT_NOT_NULL(x);
	TEST_ASSERT_EQUAL(0, x->sp);
	TEST_ASSERT_EQUAL(0, x->rp);
	TEST_ASSERT_EQUAL(-1, x->ip);
	TEST_ASSERT_NOT_NULL((void*)x->d);
	TEST_ASSERT_NOT_NULL((void*)x->u);
	TEST_ASSERT_EQUAL(1024, x->dz);
	TEST_ASSERT_EQUAL(256, x->uz);
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 0*sCELL)), x->d + 3*sCELL);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 1*sCELL)), 0);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 2*sCELL)), 0);
	TEST_ASSERT_EQUAL(*((CELL*)(x->u + 0*sCELL)), x->d + 1*sCELL);
}

/* -- Data and return stack ---------------------------- */

void test_data_stack() {
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

void test_return_stack() {
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

void test_memory() {
	TEST_ASSERT_EQUAL(x->d, sloth_to_abs(x, 0));
	TEST_ASSERT_EQUAL(0, sloth_to_rel(x, x->d));

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
}

/* -- Compilation -------------------------------------- */

void test_compilation() {
	TEST_ASSERT_EQUAL(x->d + 3*sCELL, sloth_here(x));
	sloth_comma(x, 13);
	TEST_ASSERT_EQUAL(x->d + 4*sCELL, sloth_here(x));
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, x->d + 3*sCELL));

	sloth_c_comma(x, 17);
	TEST_ASSERT_EQUAL(x->d + 4*sCELL + suCHAR, sloth_here(x));
	TEST_ASSERT_EQUAL(17, sloth_c_fetch(x, x->d + 4*sCELL));
}

/* -- Headers ------------------------------------------ */

void test_latest() {
	TEST_ASSERT_EQUAL(0, sloth_get_latest(x));
	sloth_set_latest(x, 13);
	TEST_ASSERT_EQUAL(13, sloth_get_latest(x));
}

void test_headers() {
	char name[] = "NEW-WORD";
	CELL w = sloth_here(x);
	sloth_header(x, (CELL)name, (CELL)strlen(name));
	TEST_ASSERT_EQUAL(sloth_aligned(w + 2*sCELL + 2*suCHAR + strlen(name)), sloth_here(x));
	TEST_ASSERT_EQUAL(sloth_aligned(sloth_here(x)), sloth_here(x));
	TEST_ASSERT_EQUAL(x->d + 3*sCELL, sloth_get_latest(x));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, x->d + 3*sCELL));
	TEST_ASSERT_EQUAL(sloth_here(x), sloth_fetch(x, x->d + 4*sCELL));
	TEST_ASSERT_EQUAL(0, sloth_c_fetch(x, x->d + 5*sCELL));
	TEST_ASSERT_EQUAL((uCHAR)strlen(name), sloth_c_fetch(x, x->d + 5*sCELL + 1*suCHAR));
	TEST_ASSERT_EQUAL_MEMORY(name, x->d + 5*sCELL + 2*suCHAR, strlen(name));
}

/* -- Primitive and word creation ---------------------- */

int my_var = 11;
void my_primitive(X* x) { my_var = 17; }

void test_primitive_and_word_creation() {
	TEST_ASSERT_EQUAL(0, x->p->last);
	TEST_ASSERT_EQUAL(-1, sloth_primitive(x, &my_primitive));
	TEST_ASSERT_EQUAL(1, x->p->last);
	TEST_ASSERT_EQUAL(&my_primitive, *x->p->p);

	sloth_code(x, "MY-PRIMITIVE", -1);
	TEST_ASSERT_EQUAL(-1, sloth_fetch(x, x->d + 4*sCELL));
	TEST_ASSERT_EQUAL_MEMORY("MY-PRIMITIVE", x->d + 5*sCELL + 2*suCHAR, strlen("MY-PRIMITIVE"));
}

/* -- Inner interpreter -------------------------------- */

void my_exit(X* x) { x->ip = -1; }

void test_op() {
	sloth_set(x, 3*sCELL, 11);
	x->ip = sloth_to_abs(x, 3*sCELL);
	TEST_ASSERT_EQUAL(11, sloth_op(x));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 4*sCELL), x->ip);
}

void test_do_prim() {
	TEST_ASSERT_EQUAL(11, my_var);
	sloth_primitive(x, &my_primitive);
	sloth_do_prim(x, -1);
	TEST_ASSERT_EQUAL(17, my_var);
}

void test_call() {
	x->ip = 11;
	sloth_call(x, 19);
	TEST_ASSERT_EQUAL(19, x->ip);
	TEST_ASSERT_EQUAL(1, x->rp);
	TEST_ASSERT_EQUAL(11, x->r[0]);
	sloth_rpop(x);

	sloth_call(x, -1);
	TEST_ASSERT_EQUAL(-1, x->ip);
}

void test_execute() {
	sloth_primitive(x, &my_primitive);

	my_var = 11;
	sloth_execute(x, -1);
	TEST_ASSERT_EQUAL(17, my_var);

	x->ip = -1;
	sloth_execute(x, sloth_to_abs(x, 3*sCELL));
	TEST_ASSERT_EQUAL(0, x->rp);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 3*sCELL), x->ip);
}

void test_inner() {
	sloth_set(x, 3*sCELL, sloth_primitive(x, my_primitive));
	sloth_set(x, 4*sCELL, sloth_primitive(x, my_exit));
	x->ip = sloth_to_abs(x, 3*sCELL);
	my_var = 11;
	sloth_inner(x);
	TEST_ASSERT_EQUAL(17, my_var);
	TEST_ASSERT_EQUAL(-1, x->ip);
}

void test_eval() {
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

void my_debug(X* x) { my_var = 23; }

void test_debug_inner() {
	sloth_set(x, 3*sCELL, sloth_primitive(x, my_primitive));
	sloth_set(x, 4*sCELL, sloth_primitive(x, my_exit));
	sloth_primitive(x, &my_debug);
	my_var = 11;
	x->ip = sloth_to_abs(x, 4*sCELL);
	sloth_debug_inner(x, -3);
	TEST_ASSERT_EQUAL(23, my_var);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 4*sCELL), x->s[0]);
}

int pre_var = 11;
void pre_xt(X* x) { pre_var = 13; }
int inner_var = 17;
void inner_xt(X* x) { inner_var = 19; }
int post_var = 23;
void post_xt(X* x) { post_var = 27; }

void test_debug_() {
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

int main() {
	UNITY_BEGIN();
	/* Sloth VM tests */
  RUN_TEST(test_context_init);
  RUN_TEST(test_data_stack);
	RUN_TEST(test_return_stack);    
	RUN_TEST(test_memory);
	RUN_TEST(test_compilation);
	RUN_TEST(test_latest);
	RUN_TEST(test_headers);
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
	return UNITY_END();
}
