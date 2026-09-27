#include "memory.h"

/* -- Locals words ------------------------------------- */

void sloth_allocate_(X* x) {
	void *ptr;
	if (!sloth__check_data_stack(x, 1, 1)) return;
	ptr = malloc(sloth_pop(x));
	if (ptr) {
		sloth_push(x, (CELL)ptr);
		sloth_push(x, 0);
	} else {
		sloth_push(x, 0);
		sloth_push(x, -59);
	}
}

void sloth_free_(X* x) {
	if (!sloth__check_data_stack(x, 1, 1)) return;
	free((void *)sloth_pop(x));
	sloth_push(x, 0);
}

void sloth_resize_(X* x) {
	CELL u;
	void *addr, *ptr;
	if (!sloth__check_data_stack(x, 2, 1)) return;
	u = sloth_pop(x);
	addr = (void *)sloth_pop(x);
	ptr = realloc(addr, u);
	if (ptr) {
		sloth_push(x, (CELL)ptr);
		sloth_push(x, 0);
	} else {
		sloth_push(x, (CELL)addr);
		sloth_push(x, -61);
	}
}

/* -- Special memory access words proposal ------------- */

void sloth_b_fetch_(X* x) { 
	CELL a;
	if (!sloth__check_data_stack(x, 1, 1)) return;
	a = sloth_pop(x);
	sloth_push(x, (CELL)*((BYTE_*)a));
}
void sloth_b_store_(X* x) {
	CELL a;
	BYTE_ v;
	if (!sloth__check_data_stack(x, 2, 0)) return;
	a = sloth_pop(x);
	v = (BYTE_)sloth_pop(x);
	*((BYTE_*)a) = v;
}

void sloth_w_fetch_(X* x) { 
	CELL a;
	if (!sloth__check_data_stack(x, 1, 1)) return;
	a = sloth_pop(x);
	sloth_push(x, (CELL)*((int16_t*)a));
}
void sloth_w_store_(X* x) {
	CELL a;
	int16_t v;
	if (!sloth__check_data_stack(x, 2, 0)) return;
	a = sloth_pop(x);
	v = (int16_t)sloth_pop(x);
	*((int16_t*)a) = v;
}

void sloth_l_fetch_(X* x) {
	CELL a;
	if (!sloth__check_data_stack(x, 1, 1)) return;
	a = sloth_pop(x);
	sloth_push(x, (CELL)*((int32_t*)a));
}
void sloth_l_store_(X* x) {
	CELL a;
	int32_t v;
	if (!sloth__check_data_stack(x, 2, 0)) return;
	a = sloth_pop(x);
	v = (int32_t)sloth_pop(x);
	*((int32_t*)a) = v;
}

void sloth_x_fetch_(X* x) {
	CELL a;
	if (!sloth__check_data_stack(x, 1, 1)) return;
	a = sloth_pop(x);
	sloth_push(x, (CELL)*((int64_t*)a));
}
void sloth_x_store_(X* x) {
	CELL a;
	int64_t v;
	if (!sloth__check_data_stack(x, 2, 0)) return;
	a = sloth_pop(x);
	v = (int64_t)sloth_pop(x);
	*((int64_t*)a) = v;
}

/* == Bootstrapping ==================================== */

void sloth_bootstrap_memory_word_set(X* x) {
	
	/* -- Locals words ----------------------------------- */

	sloth_code(x, "ALLOCATE", sloth_primitive(x, &sloth_allocate_));
	sloth_code(x, "FREE", sloth_primitive(x, &sloth_free_));
	sloth_code(x, "RESIZE", sloth_primitive(x, &sloth_resize_));

	/* -- Special memory proposal words ------------------ */

	sloth_code(x, "B@", sloth_primitive(x, &sloth_b_fetch_));
	sloth_code(x, "B!", sloth_primitive(x, &sloth_b_store_));
	sloth_code(x, "W@", sloth_primitive(x, &sloth_w_fetch_));
	sloth_code(x, "W!", sloth_primitive(x, &sloth_w_store_));
	sloth_code(x, "L@", sloth_primitive(x, &sloth_l_fetch_));
	sloth_code(x, "L!", sloth_primitive(x, &sloth_l_store_));
	sloth_code(x, "X@", sloth_primitive(x, &sloth_x_fetch_));
	sloth_code(x, "X!", sloth_primitive(x, &sloth_x_store_));
}
