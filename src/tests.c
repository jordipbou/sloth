#include<stdlib.h>
#include<string.h>

#include "unity.h"
#include "sloth.h"

#define PUSH(v) S_push(x, v)
#define PUSH2(v1, v2) PUSH(v1); PUSH(v2)
#define PUSH3(v1, v2, v3) PUSH2(v1, v2); PUSH(v3)

#define TEST_X(s) memset(buf, 0, sizeof(buf)); S_dump_X(buf, x, 0); TEST_ASSERT_EQUAL_STRING(s, buf)

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

void test_rot() {
  PUSH3(7, 11, 13);
  S_rot(x);
  TEST_X("11 13 7 ");
}

void test_over() {
  PUSH2(7, 11);
  S_over(x);
  TEST_X("7 11 7 ");
}

void test_drop() {
  PUSH(7);
  S_drop(x);
  TEST_X("");
}

/* Jump and call */

void test_jump() {
  B c[15] = {'[', '1', ' ', '1', '+', ']', '\\', '[', '~', 'b', -9, '^', ']', 'i', 0};
  S_push_R(x, c);
  S_inner(x);
  TEST_X("2 ");
}

void test_call() {
  B c[19] = {'[', '1', ' ', '1', '+', ']', '\\', '[', '~', 'b', -9, '$', '1', ' ', '2', '+', ']', 'i', 0};
  S_push_R(x, c);
  S_inner(x);
  TEST_X("2 3 ");
}

/* Parsing */

void test_parse_quotation() {
  B* c = "[1 1+]i";
  S_push_R(x, c);
  S_parse_quotation(x);
  TEST_ASSERT_EQUAL_INT(c + 1, TS(x));
}

void test_parse_literal() {
  S_push_R(x, "19");
  S_parse_literal(x);
  TEST_X("19  : ");
}

void test_i8_literal() {
  S_push_R(x, "0");
  S_lit_i8(x);
  TEST_X("48  : ");
}

/* Execution */

void test_exec_i() {
  PUSH((I)"1 1+]i");
  S_exec_i(x);
  TEST_X("2 ");
}

void test_exec_x() {
  /* How to test */
}

void test_ifthen() {
  S_push_R(x, "1[7][11]?");
  S_inner(x);
  TEST_X("7 ");
  S_push_R(x, "0[7][11]?");
  S_inner(x);
  TEST_X("7 11 ");
}

/* --- */

void test_itfib() {
  S_push_R(x, "1 1 6[so+]ts\\");
  S_inner(x);
  TEST_X("21 ");
}

void test_recfib() {
  S_push_R(x, "8[d2<][][1-d1-][+]b");
  S_inner(x);
  TEST_X("21 ");
}

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
  RUN_TEST(test_rot);
  RUN_TEST(test_over);
  RUN_TEST(test_drop);

  RUN_TEST(test_jump);
  RUN_TEST(test_call);

  RUN_TEST(test_parse_quotation);
  RUN_TEST(test_parse_literal);
  RUN_TEST(test_i8_literal);

  RUN_TEST(test_exec_i);
  RUN_TEST(test_exec_x);
  RUN_TEST(test_ifthen);

  RUN_TEST(test_itfib);
  RUN_TEST(test_recfib);
  RUN_TEST(test_inner_quotation);

	return UNITY_END();
}
