#include<stdlib.h>
#include<string.h>

#include "unity.h"
#include "sloth.h"

void setUp() {}

void tearDown() {}

void test_CONTEXT_init() {
	/* TODO */
}

#define INIT char buf[255]; X* x = init();

#define TEST(o, i) \
	strcpy((char*)x->c->d, i); \
	DEPTH(x) = 0; \
	IP(x) = 0; \
	inner(x); \
	buf[0] = 0; \
	TEST_ASSERT_EQUAL_STRING(o, dump(buf, x));

void test_STEP_0() { INIT; TEST("0 : ", "0"); }
void test_STEP_1() { INIT; TEST("1 : ", "1"); }
void test_STEP_l() {
	char buf[255];
	X* x = init();

	x->c->d[0] = 'l';
	*((C*)&(x->c->d[1])) = 123456;
	x->c->d[1 + sizeof(C)] = '1';
	x->c->d[2 + sizeof(C)] = 0;
	IP(x) = 0;

	inner(x);

	buf[0] = 0;
	TEST_ASSERT_EQUAL_STRING("123456 1 ", dump_stack(buf, x));
}

void test_STEP_add() { INIT; TEST("3 : ", "111++"); }
void test_STEP_sub() { INIT; TEST("-1 : ", "11-1-"); }
void test_STEP_mul() { INIT; TEST("6 : ", "11+111++*"); }
void test_STEP_div() { INIT; TEST("3 : ", "11+111++*11+/"); }
void test_STEP_mod() { INIT; TEST("2 : ", "11+111++*1111+++%"); }

void test_STEP_gt() { INIT; TEST("1 : ", "11+1>"); TEST("0 : ", "111+>"); TEST("0 : ", "11>"); }
void test_STEP_lt() { INIT; TEST("0 : ", "11+1<"); TEST("1 : ", "111+<"); TEST("0 : ", "11<"); }
void test_STEP_eq() { INIT; TEST("0 : ", "11+1="); TEST("0 : ", "111+="); TEST("1 : ", "11="); }

void test_STEP_and() { INIT; TEST("0 : ", "00&"); TEST("0 : ", "01&"); TEST("0 : ", "10&"); TEST("1 : ", "11&"); }
void test_STEP_or() { INIT; TEST("0 : ", "00|"); TEST("1 : ", "01|"); TEST("1 : ", "10|"); TEST("1 : ", "11|"); }
void test_STEP_not() { INIT; TEST("1 : ", "0!"); TEST("1 : ", "0!"); }
void test_STEP_invert() { INIT; TEST("-1 : ", "0~"); TEST("-2 : ", "1~"); TEST("0 : ", "01-~"); }

void test_STEP_dup() { INIT; TEST("1 1 : ", "1d"); }
void test_STEP_swap() { INIT; TEST("1 0 : ", "01s"); }
void test_STEP_over() { INIT; TEST("0 1 0 : ", "01o"); }
void test_STEP_rot() { INIT; TEST("1 2 0 : ", "0111+t"); }
void test_STEP_drop() { INIT; TEST("0 : ", "01\\"); }

void test_STEP_jump() { INIT; TEST("2 : ]#4j1+", "#8j[11+]#4j1+"); }
void test_STEP_call() { INIT; TEST("3 : ", "#8j[11+]#4c1+"); }
void native_function(X* x) { PUSH(x, 7); }
void test_STEP_native() { 
	INIT;
	x->c->d[0] = 'l';
	*((C*)&(x->c->d[1])) = (C)&native_function;
	x->c->d[1+sizeof(C)] = 'n';
	x->c->d[2+sizeof(C)] = 0;
	DEPTH(x) = 0;
	IP(x) = 0;
	inner(x);
	buf[0] = 0;
	TEST_ASSERT_EQUAL_STRING("7 : ", dump(buf, x));
}
void test_STEP_zjump_0() { INIT; TEST("2 : ]0#4z111++", "#8j[11+]0#4z111++"); }
void test_STEP_zjump_1() { INIT; TEST("3 : ", "#8j[11+]1#4z111++"); }

int main() {
	UNITY_BEGIN();

	RUN_TEST(test_CONTEXT_init);

	RUN_TEST(test_STEP_0);
	RUN_TEST(test_STEP_1);
	RUN_TEST(test_STEP_l);

	RUN_TEST(test_STEP_add);
	RUN_TEST(test_STEP_sub);
	RUN_TEST(test_STEP_mul);
	RUN_TEST(test_STEP_div);
	RUN_TEST(test_STEP_mod);

	RUN_TEST(test_STEP_gt);
	RUN_TEST(test_STEP_lt);
	RUN_TEST(test_STEP_eq);

	RUN_TEST(test_STEP_and);
	RUN_TEST(test_STEP_or);
	RUN_TEST(test_STEP_not);
	RUN_TEST(test_STEP_invert);

	RUN_TEST(test_STEP_dup);
	RUN_TEST(test_STEP_swap);
	RUN_TEST(test_STEP_over);
	RUN_TEST(test_STEP_rot);
	RUN_TEST(test_STEP_drop);

	RUN_TEST(test_STEP_jump);
	RUN_TEST(test_STEP_call);
	RUN_TEST(test_STEP_native);
	RUN_TEST(test_STEP_zjump_0);
	RUN_TEST(test_STEP_zjump_1);

	return UNITY_END();
}
