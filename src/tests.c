#include<stdlib.h>
#include<string.h>

#include "unity.h"
#include "sloth.h"

#define PUSH(v) S_push(x, v)
#define PUSH2(v1, v2) S_push(x, v1); S_push(x, v2)

#define TEST_X(s) memset(buf, 0, sizeof(buf)); S_dump_X(buf, x); TEST_ASSERT_EQUAL_STRING(buf, s)

X* x;
B buf[255];

void setUp() {
  x = S_init();
}

void tearDown() {}

/* Arithmetics */

void test_add() {
  PUSH2(7, 11);
  S_add(x);
  TEST_X("18 ");
}

void test_sub() {
  PUSH2(7, 11);
  S_sub(x);
  TEST_X("-4 ");
}

void test_mul() {
  PUSH2(7, 11);
  S_mul(x);
  TEST_X("77 ");
}

void test_div() {
  PUSH2(77, 11);
  S_div(x);
  TEST_X("7 ");
}

void test_mod() {
  PUSH2(15, 5);
  S_mod(x);
  PUSH2(15, 4);
  S_mod(x);
  TEST_X("0 3 ");
}

/* Comparison */

void test_lt() {
  PUSH2(7, 11);
  S_lt(x);
  PUSH2(7, 7);
  S_lt(x);
  PUSH2(11, 7);
  S_lt(x);
  TEST_X("1 0 0 ");
}

void test_eq() {
  PUSH2(7, 11);
  S_eq(x);
  PUSH2(7, 7);
  S_eq(x);
  PUSH2(11, 7);
  S_eq(x);
  TEST_X("0 1 0 ");
}

void test_gt() {
  PUSH2(7, 11);
  S_gt(x);
  PUSH2(7, 7);
  S_gt(x);
  PUSH2(11, 7);
  S_gt(x);
  TEST_X("0 0 1 ");
}

/* Bit operations */

void test_and() {
  PUSH2(0, 0);
  S_and(x);
  PUSH2(0, 1);
  S_and(x);
  PUSH2(1, 0);
  S_and(x);
  PUSH2(1, 1);
  S_and(x);
  TEST_X("0 0 0 1 ");
}

void test_or() {
  PUSH2(0, 0);
  S_or(x);
  PUSH2(0, 1);
  S_or(x);
  PUSH2(1, 0);
  S_or(x);
  PUSH2(1, 1);
  S_or(x);
  TEST_X("0 1 1 1 ");
}

void test_not() {
  PUSH(0);
  S_not(x);
  PUSH(1);
  S_not(x);
  PUSH(-1);
  S_not(x);
  TEST_X("1 0 0 ");
}

/* Stack operations */

void test_swap() {
  PUSH2(7, 11);
  S_swap(x);
  TEST_X("11 7 ");
}

void test_dup() {
  PUSH(7);
  S_dup(x);
  TEST_X("7 7 ");
}

/* --- */

void test_inner_quotation() {
  S_push_R(x, "[5 4[d1-]t]i");
  S_inner(x);
  TEST_X("5 4 3 2 1 ");
}

int main() {
	UNITY_BEGIN();

  RUN_TEST(test_add);
  RUN_TEST(test_sub);
  RUN_TEST(test_mul);
  RUN_TEST(test_div);
  RUN_TEST(test_mod);

  RUN_TEST(test_lt);
  RUN_TEST(test_eq);
  RUN_TEST(test_gt);

  RUN_TEST(test_and);
  RUN_TEST(test_or);
  RUN_TEST(test_not);

  RUN_TEST(test_swap);
  RUN_TEST(test_dup);
  
  RUN_TEST(test_inner_quotation);

	return UNITY_END();
}
