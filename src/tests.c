#include<stdlib.h>
#include<string.h>

#include "unity.h"
#include "sloth.h"

void setUp() {}

void tearDown() {}

void test_ARRAY_cells() {
}

void test_ARRAY_bytes() {
	int i;

	/* Allocate an array object with 32 data cells + 3 header cells */
	BYTE_ARRAY a = calloc(32 + 3, sizeof(CELL));

	a->sz = 32*sizeof(CELL);

	for (i = 0; i < 32*sizeof(CELL); i++) { a->dt[i] = (BYTE)i; }
	for (i = 0; i < 32*sizeof(CELL); i++) {
		TEST_ASSERT_EQUAL_INT((BYTE)i, a->dt[i]);
	}
}

int main() {
	UNITY_BEGIN();

	RUN_TEST(test_ARRAY_cells);
	RUN_TEST(test_ARRAY_bytes);

	return UNITY_END();
}

