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
	return UNITY_END();
}
