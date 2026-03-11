#define SLOTH_IMPLEMENTATION
#include "sloth.h"
#include "unity.h"

X* x;

void setUp() {
    /* Create a VM with 64 primitive slots, */
		/* 1024 bytes of dictionary, and 256 bytes of */
		/* user area */
    x = sloth_create(64, 1024, 256);
    memset((void*)x->d, 0, 1024);
    memset((void*)x->u, 0, 256);
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
    TEST_ASSERT_EQUAL(1024, x->sz);
    TEST_ASSERT_EQUAL(256, x->uz);
    TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_data_stack() {
    sloth_push(x, 42);
    TEST_ASSERT_EQUAL(1, x->sp);
    TEST_ASSERT_EQUAL(42, sloth_pick(x, 0));
    
    sloth_push(x, 100);
    TEST_ASSERT_EQUAL(2, x->sp);
    TEST_ASSERT_EQUAL(100, sloth_pick(x, 0));
    TEST_ASSERT_EQUAL(42, sloth_pick(x, 1));
    
    TEST_ASSERT_EQUAL(100, sloth_pop(x));
    TEST_ASSERT_EQUAL(1, x->sp);
    TEST_ASSERT_EQUAL(42, sloth_pop(x));
    TEST_ASSERT_EQUAL(0, x->sp);
}

void test_return_stack() {
    sloth_rpush(x, 123);
    TEST_ASSERT_EQUAL(1, x->rp);
    TEST_ASSERT_EQUAL(123, sloth_rpop(x));
    TEST_ASSERT_EQUAL(0, x->rp);
}

void test_memory_cell() {
    CELL addr = x->d;
    sloth_store(x, addr, (CELL)0xDEADBEEF);
    TEST_ASSERT_EQUAL((CELL)0xDEADBEEF, sloth_fetch(x, addr));
    
    sloth_store(x, addr + sCELL, (CELL)0xCAFEBABE);
    TEST_ASSERT_EQUAL((CELL)0xCAFEBABE, sloth_fetch(x, addr + sCELL));
}

void test_memory_char() {
    CELL addr = x->d;
    sloth_cstore(x, addr, 0xAB);
    TEST_ASSERT_EQUAL(0xAB, sloth_cfetch(x, addr));
    
    sloth_cstore(x, addr + 1, 0xCD);
    TEST_ASSERT_EQUAL(0xCD, sloth_cfetch(x, addr + 1));
}

void test_address_conversion() {
    CELL rel = 16;
    CELL abs = sloth_to_abs(x, rel);
    TEST_ASSERT_EQUAL(x->d + rel, abs);
    TEST_ASSERT_EQUAL(rel, sloth_to_rel(x, abs));
}

void my_prim(X* x) {
    CELL a = sloth_pop(x);
    sloth_push(x, a * 2);
}

void test_primitive_execution() {
    CELL xt = sloth_primitive(x, my_prim);
    sloth_push(x, 21);
    sloth__do_prim(x, xt);
    TEST_ASSERT_EQUAL(42, sloth_pop(x));
}

void test_inner_interpreter_primitive() {
    CELL xt = sloth_primitive(x, my_prim);
    /* Write the primitive XT to memory at relative */
		/* address 0 (absolute x->d) */
    sloth_store(x, x->d, xt);
    /* Set IP to absolute x->d */
    x->ip = x->d;
    
    sloth_push(x, 10);
    /* sloth_op(x) should read xt and advance IP */
    CELL op = sloth_op(x);
    TEST_ASSERT_EQUAL(xt, op);
    TEST_ASSERT_EQUAL(x->d + sCELL, x->ip);
    
    sloth__execute(x, op);
    TEST_ASSERT_EQUAL(20, sloth_pop(x));
}

void test_inner_interpreter_call() {
    /* XT for a primitive */
    CELL xt_prim = sloth_primitive(x, my_prim);
    /* XT for exit */
    CELL xt_exit_p = sloth_primitive(x, sloth_exit_);
    
    /* Code at relative 100 (absolute code_xt) */
    CELL code_xt = sloth_to_abs(x, 100);
    sloth_store(x, code_xt, xt_prim);
    sloth_store(x, code_xt + sCELL, xt_exit_p);
    
    sloth_push(x, 15);
    /* sloth_eval with q > 0 (an absolute address) */
		/* will trigger the inner interpreter */
    sloth_eval(x, code_xt); 
    
    TEST_ASSERT_EQUAL(30, sloth_pop(x));
    TEST_ASSERT_EQUAL(0, x->rp);
    TEST_ASSERT_EQUAL(-1, x->ip);
}

void my_thrower(X* x) {
    CELL e = sloth_pop(x);
    sloth_throw(x, e);
}

void test_exceptions() {
    CELL xt_thrower = sloth_primitive(x, my_thrower);
    
    /* Push error code */
    sloth_push(x, -42);
    /* catch expects an XT to eval */
    sloth_catch(x, xt_thrower);
    
    CELL result = sloth_pop(x);
    TEST_ASSERT_EQUAL(-42, result);
}

void test_catch_success() {
    CELL xt_prim = sloth_primitive(x, my_prim);
    sloth_push(x, 10);
    sloth_catch(x, xt_prim);
    
    CELL error = sloth_pop(x);
    CELL value = sloth_pop(x);
    
    TEST_ASSERT_EQUAL(0, error);
    TEST_ASSERT_EQUAL(20, value);
}

void test_stack_underflow_detection() {
    /* Using sloth_drop_ which has underflow check */
    CELL xt_drop = sloth_primitive(x, sloth_drop_);
    
    /* Catch the underflow */
    sloth_catch(x, xt_drop);
    TEST_ASSERT_EQUAL(SLOTH_STACK_UNDERFLOW, sloth_pop(x));
}

void test_return_stack_underflow_detection() {
    CELL xt_r_from = sloth_primitive(x, sloth_r_from_);
    
    sloth_catch(x, xt_r_from);
    TEST_ASSERT_EQUAL(SLOTH_RETURN_STACK_UNDERFLOW, sloth_pop(x));
}

void test_sloth_op() {
    sloth_store(x, x->d, 1);
    sloth_store(x, x->d + sCELL, 2);
    x->ip = x->d;
    
    TEST_ASSERT_EQUAL(1, sloth_op(x));
    TEST_ASSERT_EQUAL(x->d + sCELL, x->ip);
    TEST_ASSERT_EQUAL(2, sloth_op(x));
    TEST_ASSERT_EQUAL(x->d + 2 * sCELL, x->ip);
}

int debug_count = 0;
void debug_spy(X* x) {
    /* sloth__debug pushes x->ip before calling debug_xt */
    sloth_pop(x); 
    debug_count++;
}

void test_debug_inner() {
    CELL xt_prim = sloth_primitive(x, my_prim);
    CELL xt_exit_p = sloth_primitive(x, sloth_exit_);
    CELL xt_debug_spy = sloth_primitive(x, debug_spy);
    
    CELL code_xt = sloth_to_abs(x, 200);
    sloth_store(x, code_xt, xt_prim);
    sloth_store(x, code_xt + sCELL, xt_exit_p);
    
    debug_count = 0;
    x->ip = code_xt;
    sloth_rpush(x, -1); /* Dummy return address to stop inner loop */
    
    sloth_push(x, 10);
    sloth__debug_inner(x, xt_debug_spy);
    
    TEST_ASSERT_EQUAL(20, sloth_pop(x));
    /* debug_spy should have been called twice: */
		/* once for xt_prim, once for xt_exit_p */
    TEST_ASSERT_EQUAL(2, debug_count);
}

void test_sloth_new() {
    X* x2 = sloth_new();
    TEST_ASSERT_NOT_NULL(x2);
    TEST_ASSERT_EQUAL(524288, x2->sz);
    sloth_free(x2);
}

/* Forth Kernel Tests */

void test_kernel_helpers() {
    /* Test sloth_set/get (dictionary) */
    sloth_set(x, 100, 0x12345678);
    TEST_ASSERT_EQUAL(0x12345678, sloth_get(x, 100));

    /* Test sloth_user_area_set/get */
    sloth_user_area_set(x, 20, 0x87654321);
    TEST_ASSERT_EQUAL(0x87654321, sloth_user_area_get(x, 20));
}

void test_kernel_memory_management() {
    /* Initialize HERE to point to some offset */
    sloth_set(x, SLOTH_HERE, sloth_to_abs(x, 64));
    TEST_ASSERT_EQUAL(sloth_to_abs(x, 64), sloth_here(x));

    /* Test allot */
    sloth_allot(x, 32);
    TEST_ASSERT_EQUAL(sloth_to_abs(x, 96), sloth_here(x));

    /* Test aligned/align */
    TEST_ASSERT_EQUAL(0, sloth_aligned(0));
    TEST_ASSERT_EQUAL(sCELL, sloth_aligned(1));
    TEST_ASSERT_EQUAL(sCELL, sloth_aligned(sCELL));
    
    sloth_set(x, SLOTH_HERE, sloth_to_abs(x, 97));
    sloth_align(x);
    TEST_ASSERT_EQUAL(ALIGNED(sloth_to_abs(x, 97), sCELL), sloth_here(x));
}

void test_kernel_compilation() {
    sloth_set(x, SLOTH_HERE, sloth_to_abs(x, 128));
    
    /* Test comma */
    sloth_comma(x, 0xAAAA);
    TEST_ASSERT_EQUAL(0xAAAA, sloth_fetch(x, sloth_to_abs(x, 128)));
    TEST_ASSERT_EQUAL(sloth_to_abs(x, 128 + sCELL), sloth_here(x));

    /* Test ccomma */
    CELL h = sloth_here(x);
    sloth_ccomma(x, 0x55);
    TEST_ASSERT_EQUAL(0x55, sloth_cfetch(x, h));
    TEST_ASSERT_EQUAL(h + suCHAR, sloth_here(x));

    /* Test compile */
    h = sloth_here(x);
    sloth_compile(x, 0xBBBB);
    TEST_ASSERT_EQUAL(0xBBBB, sloth_fetch(x, h));
    TEST_ASSERT_EQUAL(h + sCELL, sloth_here(x));
}

void test_kernel_headers_and_flags() {
    /* We need to set up CURRENT and wordlists for header tests */
    /* CURRENT points to a variable that holds the latest word NT */
    CELL latest_var_abs_addr = x->d + 256; /* Use some space in dict */
    sloth_user_area_set(x, SLOTH_CURRENT, latest_var_abs_addr);
    sloth_store(x, latest_var_abs_addr, 0); /* No latest word yet */

    /* Set up a word name in memory */
    char* name = "TESTWORD";
    CELL name_addr = sloth_to_abs(x, 500);
    memcpy((void*)name_addr, name, strlen(name));

    /* Initialize HERE */
    sloth_set(x, SLOTH_HERE, sloth_to_abs(x, 600));

    /* Create header */
    CELL w = sloth_header(x, name_addr, strlen(name));

    TEST_ASSERT_EQUAL(w, sloth_get_latest(x));
    TEST_ASSERT_EQUAL(0, sloth_get_link(x, w)); /* First word, link is 0 */
    TEST_ASSERT_EQUAL(strlen(name), sloth_get_namelen(x, w));
    
    /* Check XT is set correctly (after flags and name) */
    CELL xt = sloth_get_xt(x, w);
    TEST_ASSERT_NOT_EQUAL(0, xt);
    TEST_ASSERT_TRUE(xt > w);

    /* Test flags */
    TEST_ASSERT_EQUAL(0, sloth_get_flags(x, w));
    sloth_set_flag(x, w, SLOTH_IMMEDIATE);
    TEST_ASSERT_TRUE(sloth_has_flag(x, w, SLOTH_IMMEDIATE));
    TEST_ASSERT_FALSE(sloth_has_flag(x, w, SLOTH_HIDDEN));

    sloth_set_flag(x, w, SLOTH_HIDDEN);
    TEST_ASSERT_TRUE(sloth_has_flag(x, w, SLOTH_HIDDEN));

    sloth_unset_flag(x, w, SLOTH_IMMEDIATE);
    TEST_ASSERT_FALSE(sloth_has_flag(x, w, SLOTH_IMMEDIATE));
    TEST_ASSERT_TRUE(sloth_has_flag(x, w, SLOTH_HIDDEN));

    /* Test XT setter */
    sloth_set_xt(x, w, 0xDEAD);
    TEST_ASSERT_EQUAL(0xDEAD, sloth_get_xt(x, w));
}

void test_kernel_literal() {
    /* Setup for sloth_literal: it needs to find "(LIT)" */
    CELL latest_var_abs_addr = x->d + 256;
    sloth_user_area_set(x, SLOTH_CURRENT, latest_var_abs_addr);
    sloth_store(x, latest_var_abs_addr, 0);

    /* We need search order to find words */
    sloth_user_area_set(x, SLOTH_ORDER, 1);
    /* CONTEXT 0 points to the variable holding the latest word */
    sloth_user_area_set(x, SLOTH_CONTEXT, latest_var_abs_addr);

    /* Create (LIT) word */
    char* lit_name = "(LIT)";
    CELL name_addr = sloth_to_abs(x, 500);
    memcpy((void*)name_addr, lit_name, strlen(lit_name));
    sloth_set(x, SLOTH_HERE, sloth_to_abs(x, 600));
    CELL w = sloth_header(x, name_addr, strlen(lit_name));
    sloth_set_xt(x, w, 0x1111);

    /* Now test sloth_literal */
    sloth_set(x, SLOTH_HERE, sloth_to_abs(x, 800));
    sloth_literal(x, 0x2222);

    /* It should have compiled 0x1111 (XT of (LIT)) and then 0x2222 */
    TEST_ASSERT_EQUAL(0x1111, sloth_fetch(x, sloth_to_abs(x, 800)));
    TEST_ASSERT_EQUAL(0x2222, sloth_fetch(x, sloth_to_abs(x, 800 + sCELL)));
    TEST_ASSERT_EQUAL(sloth_to_abs(x, 800 + 2 * sCELL), sloth_here(x));
}

int main() {
	UNITY_BEGIN();
	/* Sloth VM tests */
    RUN_TEST(test_context_init);
    RUN_TEST(test_data_stack);
    RUN_TEST(test_return_stack);
    RUN_TEST(test_memory_cell);
    RUN_TEST(test_memory_char);
    RUN_TEST(test_address_conversion);
    RUN_TEST(test_primitive_execution);
    RUN_TEST(test_inner_interpreter_primitive);
    RUN_TEST(test_inner_interpreter_call);
    RUN_TEST(test_exceptions);
    RUN_TEST(test_catch_success);
    RUN_TEST(test_stack_underflow_detection);
    RUN_TEST(test_return_stack_underflow_detection);
    RUN_TEST(test_sloth_op);
    RUN_TEST(test_debug_inner);
    RUN_TEST(test_sloth_new);
    
    /* Forth Kernel tests */
    RUN_TEST(test_kernel_helpers);
    RUN_TEST(test_kernel_memory_management);
    RUN_TEST(test_kernel_compilation);
    RUN_TEST(test_kernel_headers_and_flags);
    RUN_TEST(test_kernel_literal);

	return UNITY_END();
}
