#define SLOTH_IMPLEMENTATION
#include "sloth.h"
#include "unity.h"

X* x;

void setUp() {
	/* Create a VM with 64 primitive slots and 1024 bytes */
	/* of dictionary. */
	x = sloth_create(64, 16384, 1024);
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
	TEST_ASSERT_EQUAL(16384, x->dz);
	TEST_ASSERT_EQUAL(1024, x->uz);
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 0*sCELL)), x->d + 3*sCELL);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 1*sCELL)), 0);
	TEST_ASSERT_EQUAL(*((CELL*)(x->d + 2*sCELL)), 0);
	TEST_ASSERT_EQUAL(*((CELL*)(x->u + 0*sCELL)), x->d + 2*sCELL);
}

void test_new() {
	X* x2 = sloth_new();
	TEST_ASSERT_NOT_NULL(x2);
	TEST_ASSERT_EQUAL(512, x2->p->pz);
	TEST_ASSERT_EQUAL(524288, x2->dz);
	TEST_ASSERT_EQUAL(1024, x2->uz);
	sloth_free(x2);
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

void test_over_() {
	sloth_push(x, 42);
	sloth_push(x, 100);
	sloth_over_(x);
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(42, x->s[2]);
	TEST_ASSERT_EQUAL(100, x->s[1]);
	TEST_ASSERT_EQUAL(42, x->s[0]);
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

	sloth_unused_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL((x->d + x->dz) - sloth_here(x), sloth_pop(x));
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

void test_get_set_latest() {
	TEST_ASSERT_EQUAL(0, sloth_get_latest(x));
	sloth_set_latest(x, 13);
	TEST_ASSERT_EQUAL(13, sloth_get_latest(x));
}

void test_get_link() {
	sloth_set(x, 100, 13);
	TEST_ASSERT_EQUAL(13, sloth_get_link(x, sloth_to_abs(x, 100)));
}

void test_get_set_xt() {
	sloth_set_xt(x, sloth_to_abs(x, 100), 17);
	TEST_ASSERT_EQUAL(17, sloth_get_xt(x, sloth_to_abs(x, 100)));
}

void test_get_set_flags() {
	sloth_set_flags(x, sloth_to_abs(x, 100), 33);
	TEST_ASSERT_EQUAL(33, sloth_get_flags(x, sloth_to_abs(x, 100)));
}

void test_has_set_unset_flag() {
	sloth_set_flags(x, sloth_to_abs(x, 100), 0);
	TEST_ASSERT_FALSE(sloth_has_flag(x, sloth_to_abs(x, 100), 2));
	sloth_set_flag(x, sloth_to_abs(x, 100), 2);
	TEST_ASSERT_TRUE(sloth_has_flag(x, sloth_to_abs(x, 100), 2));
	sloth_unset_flag(x, sloth_to_abs(x, 100), 2);
	TEST_ASSERT_FALSE(sloth_has_flag(x, sloth_to_abs(x, 100), 2));
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

void test_name_and_len() {
	char name[] = "TEST-WORD";
	CELL w = sloth_header(x, (CELL)name, (CELL)strlen(name));
	TEST_ASSERT_EQUAL_MEMORY(name, (char*)sloth_get_name_addr(x, w), strlen(name));
	TEST_ASSERT_EQUAL(strlen(name), sloth_get_namelen(x, w));
}

void test_immediate_() {
	char name[] = "IMM-WORD";
	CELL w = sloth_header(x, (CELL)name, (CELL)strlen(name));
	TEST_ASSERT_FALSE(sloth_has_flag(x, w, SLOTH_IMMEDIATE));
	sloth_immediate_(x);
	TEST_ASSERT_TRUE(sloth_has_flag(x, w, SLOTH_IMMEDIATE));
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

/* -- Exceptions --------------------------------------- */

void throw_42_prim(X* x) { sloth_throw(x, 42); }

void test_catch_no_throw() {
	CELL p = sloth_primitive(x, my_primitive);
	sloth_catch(x, p);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_catch_throw() {
	CELL p = sloth_primitive(x, throw_42_prim);
	sloth_catch(x, p);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_catch_restores_stack() {
	sloth_push(x, 1);
	sloth_push(x, 2);
	CELL p = sloth_primitive(x, throw_42_prim);
	sloth_catch(x, p);
	TEST_ASSERT_EQUAL(3, x->sp);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(2, sloth_pop(x));
	TEST_ASSERT_EQUAL(1, sloth_pop(x));
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void nested_throw_test_prim(X* x) {
	CELL p = sloth_primitive(x, throw_42_prim);
	sloth_catch(x, p);
}

void test_nested_catch() {
	CELL p = sloth_primitive(x, nested_throw_test_prim);
	sloth_catch(x, p);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(-1, x->jmpbuf_idx);
}

void test_catch_throw_prim() {
	CELL p = sloth_primitive(x, throw_42_prim);
	sloth_push(x, p);
	sloth_catch_(x);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	
	sloth_push(x, 0); // No error
	sloth_throw_(x);
	TEST_ASSERT_EQUAL(0, x->sp);
}

/* -- Inner interpreter primitives -------------------- */

void test_exit_() {
	// Test rp > 0
	sloth_rpush(x, 100);
	sloth_exit_(x);
	TEST_ASSERT_EQUAL(100, x->ip);
	TEST_ASSERT_EQUAL(0, x->rp);
	
	// Test rp == 0
	sloth_exit_(x);
	TEST_ASSERT_EQUAL(-1, x->ip);
}

void test_lit_() {
	sloth_set(x, 3*sCELL, 42);
	x->ip = sloth_to_abs(x, 3*sCELL);
	sloth_lit_(x);
	TEST_ASSERT_EQUAL(42, sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 4*sCELL), x->ip);
}

void test_rip_() {
	CELL base_ip = sloth_to_abs(x, 3*sCELL);
	sloth_set(x, 3*sCELL, 10 * sCELL); // offset
	x->ip = base_ip;
	sloth_rip_(x);
	TEST_ASSERT_EQUAL(base_ip + 9*sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(base_ip + sCELL, x->ip);
}

void test_branch_() {
	CELL base_ip = sloth_to_abs(x, 3*sCELL);
	sloth_set(x, 3*sCELL, 10 * sCELL); // offset
	x->ip = base_ip;
	sloth_branch_(x);
	TEST_ASSERT_EQUAL(base_ip + 10*sCELL, x->ip);
}

void test_zbranch_() {
	CELL base_ip = sloth_to_abs(x, 3*sCELL);
	sloth_set(x, 3*sCELL, 10 * sCELL);
	
	// Case 0: TOS is 0 (should branch)
	x->ip = base_ip;
	sloth_push(x, 0);
	sloth_zbranch_(x);
	TEST_ASSERT_EQUAL(base_ip + 10*sCELL, x->ip);
	
	// Case 1: TOS is NOT 0 (should NOT branch)
	x->ip = base_ip;
	sloth_push(x, 1);
	sloth_zbranch_(x);
	TEST_ASSERT_EQUAL(base_ip + sCELL, x->ip);
}

/* -- Strings ------------------------------------------ */

void test_string_() {
	sloth_set(x, 100, 5);
	x->ip = sloth_to_abs(x, 100);
	sloth_string_(x);
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(5, sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100 + sCELL), sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_aligned(sloth_to_abs(x, 100 + sCELL + 5)), x->ip);
}

void test_c_string_() {
	sloth_c_store(x, sloth_to_abs(x, 100), 5);
	x->ip = sloth_to_abs(x, 100);
	sloth_c_string_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100), sloth_pop(x));
	TEST_ASSERT_EQUAL(sloth_aligned(sloth_to_abs(x, 100 + 6)), x->ip);
}

/* -- Searching ---------------------------------------- */

void test_searching() {
	char name[] = "FIND-ME";

	CELL w = sloth_header(x, (CELL)name, (CELL)strlen(name));
	sloth_set_xt(x, w, 123);
	
	TEST_ASSERT_TRUE(sloth_compare_no_case(x, (CELL)name, strlen(name), (CELL)"find-me", strlen("find-me")));
	TEST_ASSERT_FALSE(sloth_compare_no_case(x, (CELL)name, strlen(name), (CELL)"other", 5));
	
	TEST_ASSERT_EQUAL(w, sloth_search_word(x, (CELL)name, strlen(name)));
	TEST_ASSERT_EQUAL(w, sloth_find_word(x, name));
	
	// Test sloth_find_ (primitive)
	CELL cstring = sloth_here(x);
	sloth_c_comma(x, strlen(name));
	for(int i=0; i<strlen(name); i++) sloth_c_comma(x, name[i]);
	
	sloth_push(x, cstring);
	sloth_find_(x);
	TEST_ASSERT_EQUAL(-1, sloth_pop(x)); // Not immediate
	TEST_ASSERT_EQUAL(123, sloth_pop(x));
	
	sloth_immediate_(x);
	sloth_push(x, cstring);
	sloth_find_(x);
	TEST_ASSERT_EQUAL(1, sloth_pop(x)); // Immediate
	TEST_ASSERT_EQUAL(123, sloth_pop(x));
}

/* -- Compile and literal ------------------------------ */

void test_compile_and_literal() {
	/* Create (LIT) to ensure it's correctly compiled */
	sloth_code(x, "(LIT)", 999);

	sloth_literal(x, 42);
	CELL h = sloth_here(x);
	TEST_ASSERT_EQUAL(999, sloth_fetch(x, h - 2*sCELL));
	TEST_ASSERT_EQUAL(42, sloth_fetch(x, h - sCELL));
	
	sloth_compile(x, 777);
	TEST_ASSERT_EQUAL(777, sloth_fetch(x, sloth_here(x) - sCELL));
}

/* -- Quotations ---------------------------------------- */

void test_quotation_primitive() {
	sloth_set(x, 100, 3*sCELL);
	x->ip = sloth_to_abs(x, 100);
	sloth_quotation_(x);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100 + 4*sCELL), x->ip);
	TEST_ASSERT_EQUAL(sloth_to_abs(x, 100 + sCELL), sloth_pop(x));
}

void test_start_quotation_interpret_mode() {
	sloth_code(x, "(QUOTATION)", 111);

	CELL here = sloth_here(x);
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

void test_start_nested_quotation_interpret_mode() {
	sloth_code(x, "(QUOTATION)", 111);

	CELL here = sloth_here(x);
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

void test_start_quotation_compile_mode() {
	sloth_code(x, "(QUOTATION)", 111);

	CELL here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_start_quotation_(x);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(0, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(2, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(2, x->sp);
	TEST_ASSERT_EQUAL(here + sCELL, sloth_pop(x));
	TEST_ASSERT_EQUAL(0, sloth_pop(x));
}

void test_start_nested_quotation_compile_mode() {
	sloth_code(x, "(QUOTATION)", 111);

	CELL here = sloth_here(x);
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

void test_end_quotation_interpret_mode() {
	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	CELL here = sloth_here(x);
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

void test_end_nested_quotation_interpret_mode() {
	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	CELL here = sloth_here(x);
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

void test_end_quotation_compile_mode() {
	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	CELL here = sloth_here(x);
	sloth_user_set(x, SLOTH_STATE, 1);
	sloth_start_quotation_(x);
	sloth_end_quotation_(x);

	TEST_ASSERT_EQUAL(111, sloth_fetch(x, here));
	TEST_ASSERT_EQUAL(sCELL, sloth_fetch(x, here + sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, here + 2*sCELL));
	TEST_ASSERT_EQUAL(1, sloth_user_get(x, SLOTH_STATE));
	TEST_ASSERT_EQUAL(0, x->sp);
}

void test_end_nested_quotation_compile_mode() {
	sloth_code(x, "(QUOTATION)", 111);
	sloth_code(x, "EXIT", 222);

	CELL here = sloth_here(x);
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

void test_user_variable_creation() {
	CELL h;
	sloth_code(x, "EXIT", 111);
	sloth_code(x, "(LIT)", 222);
	sloth_user_variable(x, "TEST-VAR", 8, 13);
	TEST_ASSERT_EQUAL(111, sloth_fetch(x, sloth_here(x) - 1*sCELL));
	TEST_ASSERT_EQUAL(x->u + 8, sloth_fetch(x, sloth_here(x) - 2*sCELL));
	TEST_ASSERT_EQUAL(222, sloth_fetch(x, sloth_here(x) - 3*sCELL));
	TEST_ASSERT_EQUAL(13, sloth_fetch(x, x->u + 8));
}

/* -- Source code preprocessing, interpreting & auditing commands */

void test_file_position() {
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

void test_read_line() {
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
	TEST_ASSERT_EQUAL('a', buf[0]);
	TEST_ASSERT_EQUAL('b', buf[1]);
	TEST_ASSERT_EQUAL('c', buf[2]);

	fclose(f);
}

void my_accept(X* x) { sloth_push(x, 15); }

void test_refill() {
	sloth_user_set(x, SLOTH_SOURCE_ID, -1);
	sloth_refill_(x);
	TEST_ASSERT_EQUAL(1, x->sp);
	TEST_ASSERT_EQUAL(0, sloth_pop(x));

	CELL ibuf = sloth_user_get(x, SLOTH_IBUF);
	sloth_code(x, "ACCEPT", sloth_primitive(x, &my_accept));
	sloth_user_set(x, SLOTH_SOURCE_ID, 0);
	sloth_refill_(x);	
	TEST_ASSERT_EQUAL(ibuf, sloth_user_get(x, SLOTH_IBUF));
	TEST_ASSERT_EQUAL(15, sloth_user_get(x, SLOTH_ILEN));
	TEST_ASSERT_EQUAL(0, sloth_user_get(x, SLOTH_IPOS));

	/* TODO Test refill for files */
}

/* TODO Tests for save_input, restore_input, included */

/* -- Bootstrapping ------------------------------------ */

void test_bootstrap() {
	sloth_bootstrap(x);
	TEST_ASSERT_NOT_EQUAL(0, sloth_find_word(x, "EXIT"));
	TEST_ASSERT_NOT_EQUAL(0, sloth_find_word(x, "DUP"));
	TEST_ASSERT_NOT_EQUAL(0, sloth_find_word(x, "@"));
}

int main() {
	UNITY_BEGIN();
	/* Sloth VM tests */
	RUN_TEST(test_context_init);
	RUN_TEST(test_new);
	RUN_TEST(test_data_stack);
	RUN_TEST(test_over_);
	RUN_TEST(test_return_stack);
	RUN_TEST(test_memory);
	RUN_TEST(test_compilation);
	/* Headers */
	RUN_TEST(test_get_set_latest);
	RUN_TEST(test_get_link);
	RUN_TEST(test_get_set_xt);
	RUN_TEST(test_get_set_flags);
	RUN_TEST(test_has_set_unset_flag);
	RUN_TEST(test_headers);
	RUN_TEST(test_name_and_len);
	RUN_TEST(test_immediate_);
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
	/* Strings */
	RUN_TEST(test_string_);
	RUN_TEST(test_c_string_);
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
	/* Bootstrap */
	RUN_TEST(test_bootstrap);
	return UNITY_END();
}
