#define SLOTH_IMPLEMENTATION
#include "sloth.h"
#include "unity.h"

X* x;

void setUp() {
    /* Create a VM with 64 primitive slots and 1024 bytes */
		/* of dictionary. */
    x = sloth_create(64, 1024, 256);
    memset((void*)x->d, 0, 1024);
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
}

void test_data_stack() {
    sloth_push(x, 42);
    TEST_ASSERT_EQUAL(1, x->sp);
    TEST_ASSERT_EQUAL(42, x->s[0]);
    
    sloth_push(x, 100);
    TEST_ASSERT_EQUAL(2, x->sp);
    TEST_ASSERT_EQUAL(100, x->s[1]);
    TEST_ASSERT_EQUAL(42, x->s[0]);
    
    TEST_ASSERT_EQUAL(100, sloth_pop(x));
    TEST_ASSERT_EQUAL(1, x->sp);
    TEST_ASSERT_EQUAL(42, sloth_pop(x));
    TEST_ASSERT_EQUAL(0, x->sp);
}

int main() {
	UNITY_BEGIN();
	/* Sloth VM tests */
  RUN_TEST(test_context_init);
  RUN_TEST(test_data_stack);
    
	return UNITY_END();
}
