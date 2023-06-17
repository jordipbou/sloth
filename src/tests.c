#include<stdlib.h>
#include<string.h>

#include "unity.h"
#include "sloth.h"

X* x;
B buf[255];

void setUp() {
  x = S_init();
  memset(buf, 0, sizeof(buf));
}

void tearDown() {}

void test_dump_S() {
  S_push(x, 7);
  S_push(x, 11);
  
  S_dump_S(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "7 11 ");
}

/* Arithmetics */

void test_add() {
  S_push(x, 7);
  S_push(x, 11);
  S_add(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<1> 18 <0>");
}

void test_sub() {
  S_push(x, 7);
  S_push(x, 11);
  S_sub(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<1> -4 <0>");
}

void test_mul() {
  S_push(x, 7);
  S_push(x, 11);
  S_mul(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<1> 77 <0>");
}

void test_div() {
  S_push(x, 77);
  S_push(x, 11);
  S_div(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<1> 7 <0>");
}

void test_mod() {
  S_push(x, 15);
  S_push(x, 5);
  S_mod(x);
  S_push(x, 15);
  S_push(x, 4);
  S_mod(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<2> 0 3 <0>");
}

/* Comparison */

void test_lt() {
  S_push(x, 7);
  S_push(x, 11);
  S_lt(x);
  S_push(x, 7);
  S_push(x, 7);
  S_lt(x);
  S_push(x, 11);
  S_push(x, 7);
  S_lt(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<3> 1 0 0 <0>");
}

void test_eq() {
  S_push(x, 7);
  S_push(x, 11);
  S_eq(x);
  S_push(x, 7);
  S_push(x, 7);
  S_eq(x);
  S_push(x, 11);
  S_push(x, 7);
  S_eq(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<3> 0 1 0 <0>");
}

void test_gt() {
  S_push(x, 7);
  S_push(x, 11);
  S_gt(x);
  S_push(x, 7);
  S_push(x, 7);
  S_gt(x);
  S_push(x, 11);
  S_push(x, 7);
  S_gt(x);
  S_dump_X(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "<3> 0 0 1 <0>");
}
void test_inner_quotation() {
  S_push_R(x, "[5 4[d1-]t]i");
  S_inner(x);
  S_dump_S(buf, x);
  TEST_ASSERT_EQUAL_STRING(buf, "5 4 3 2 1 ");
}

int main() {
	UNITY_BEGIN();

  RUN_TEST(test_dump_S);

  RUN_TEST(test_add);
  RUN_TEST(test_sub);
  RUN_TEST(test_mul);
  RUN_TEST(test_div);
  RUN_TEST(test_mod);

  RUN_TEST(test_lt);
  RUN_TEST(test_eq);
  RUN_TEST(test_gt);
  
  RUN_TEST(test_inner_quotation);

	return UNITY_END();
}
