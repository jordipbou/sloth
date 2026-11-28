#define _POSIX_C_SOURCE 199309L

#include "facility.h"
#include <time.h>

/* The Facility word set is backed by the host input/output hooks. The
   desktop executable installs a CPNBI adapter; the VM/bootstrap stays
   free of CPNBI. This word set is optional. */

/* -- Milliseconds multiplatform implementation -------- */
/* Taken from: https://stackoverflow.com/a/28827188 */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
/* Instead of including windows.h we just pre-define here */
/* the Sleep function on the windows.h API. Including the */
/* windows.h header introduces conflicts with RayLib. */
void __stdcall Sleep(unsigned long ms);
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   /* for nanosleep */
#else
#include <unistd.h> /* for usleep */
#endif

void sleep_ms(int milliseconds) { /* cross-platform sleep */
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	if (milliseconds >= 1000)
		sleep(milliseconds / 1000);
	usleep((milliseconds % 1000) * 1000);
#endif
}

/* == Facility words =================================== */

void sloth_key_question_(X* x) {
	sloth_push(x, x->input(x, SLOTH_IO_KEY_AVAILABLE) ? -1 : 0);
}

/* -- Facility extension words ========================= */

/* -- Keyboard input ----------------------------------- */

void sloth_e_key_(X* x) {
	sloth_push(x, (CELL)x->input(x, SLOTH_IO_EKEY_READ));
}

void sloth_e_key_to_char_(X* x) {
	int32_t e = (int32_t)sloth_pop(x);
	int32_t key = SLOTH_EVENT_KEY(e);
	if (!SLOTH_EVENT_IS_SPECIAL(e) && key >= 32 && key <= 126) {
		sloth_push(x, key);
		sloth_push(x, -1);
	} else {
		sloth_push(x, e);
		sloth_push(x, 0);
	}
}

static int sloth__is_special_key(int32_t key) {
	if (key >= SLOTH_KEY_UP && key <= SLOTH_KEY_RIGHT) return 1;
	if (key >= SLOTH_KEY_HOME && key <= SLOTH_KEY_PAGE_DOWN) return 1;
	if (key >= SLOTH_KEY_F1 && key <= SLOTH_KEY_F12) return 1;
	return 0;
}

void sloth_e_key_to_f_key_(X* x) {
	int32_t e = (int32_t)sloth_pop(x);
	int32_t key = SLOTH_EVENT_KEY(e);
	int32_t mods = SLOTH_EVENT_MOD(e);
	if (sloth__is_special_key(key)) {
		sloth_push(x, key | (mods << SLOTH_EVENT_MOD_OFFSET));
		sloth_push(x, -1);
	} else {
		sloth_push(x, e);
		sloth_push(x, 0);
	}
}

void sloth_e_key_question_(X* x) {
	sloth_push(x, x->input(x, SLOTH_IO_EKEY_AVAILABLE) ? -1 : 0);
}

void sloth_emit_question_(X* x) {
	sloth_push(x, x->output(x, SLOTH_IO_EMIT_AVAILABLE, 0) ? -1 : 0);
}

void sloth_k_alt_mask_(X* x) { sloth_push(x, SLOTH_MOD_ALT << SLOTH_EVENT_MOD_OFFSET); }
void sloth_k_ctrl_mask_(X* x) { sloth_push(x, SLOTH_MOD_CTRL << SLOTH_EVENT_MOD_OFFSET); }
void sloth_k_shift_mask_(X* x) { sloth_push(x, SLOTH_MOD_SHIFT << SLOTH_EVENT_MOD_OFFSET); }

void sloth_k_insert_(X* x) { sloth_push(x, SLOTH_KEY_INSERT); }
void sloth_k_delete_(X* x) { sloth_push(x, SLOTH_KEY_DELETE); }
void sloth_k_home_(X* x) { sloth_push(x, SLOTH_KEY_HOME); }
void sloth_k_prior_(X* x) { sloth_push(x, SLOTH_KEY_PAGE_UP); }
void sloth_k_next_(X* x) { sloth_push(x, SLOTH_KEY_PAGE_DOWN); }
void sloth_k_end_(X* x) { sloth_push(x, SLOTH_KEY_END); }

void sloth_k_up_(X* x) { sloth_push(x, SLOTH_KEY_UP); }
void sloth_k_down_(X* x) { sloth_push(x, SLOTH_KEY_DOWN); }
void sloth_k_left_(X* x) { sloth_push(x, SLOTH_KEY_LEFT); }
void sloth_k_right_(X* x) { sloth_push(x, SLOTH_KEY_RIGHT); }

void sloth_k_f1_(X* x) { sloth_push(x, SLOTH_KEY_F1); }
void sloth_k_f2_(X* x) { sloth_push(x, SLOTH_KEY_F2); }
void sloth_k_f3_(X* x) { sloth_push(x, SLOTH_KEY_F3); }
void sloth_k_f4_(X* x) { sloth_push(x, SLOTH_KEY_F4); }
void sloth_k_f5_(X* x) { sloth_push(x, SLOTH_KEY_F5); }
void sloth_k_f6_(X* x) { sloth_push(x, SLOTH_KEY_F6); }
void sloth_k_f7_(X* x) { sloth_push(x, SLOTH_KEY_F7); }
void sloth_k_f8_(X* x) { sloth_push(x, SLOTH_KEY_F8); }
void sloth_k_f9_(X* x) { sloth_push(x, SLOTH_KEY_F9); }
void sloth_k_f10_(X* x) { sloth_push(x, SLOTH_KEY_F10); }
void sloth_k_f11_(X* x) { sloth_push(x, SLOTH_KEY_F11); }
void sloth_k_f12_(X* x) { sloth_push(x, SLOTH_KEY_F12); }

/* -- Time and date ------------------------------------ */

void sloth_time_and_date_(X* x) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	sloth_push(x, tm->tm_sec);
	sloth_push(x, tm->tm_min);
	sloth_push(x, tm->tm_hour);
	sloth_push(x, tm->tm_mday);
	sloth_push(x, tm->tm_mon + 1);
	sloth_push(x, tm->tm_year + 1900);
}

void sloth_ms_(X* x) {
	sleep_ms(sloth_pop(x));
}

/* == Bootstrapping ==================================== */

void sloth_bootstrap_facility_word_set(X* x) {

	/* == Facility words ================================= */

	sloth_code(x, "KEY?", sloth_primitive(x, &sloth_key_question_));

	/* == Facility extension words ======================= */

	/* -- Keyboard input --------------------------------- */

	sloth_code(x, "EKEY", sloth_primitive(x, &sloth_e_key_));
	sloth_code(x, "EKEY>CHAR", sloth_primitive(x, &sloth_e_key_to_char_));
	sloth_code(x, "EKEY>FKEY", sloth_primitive(x, &sloth_e_key_to_f_key_));
	sloth_code(x, "EKEY?", sloth_primitive(x, &sloth_e_key_question_));
	sloth_code(x, "EMIT?", sloth_primitive(x, &sloth_emit_question_));

	sloth_code(x, "K-ALT-MASK", sloth_primitive(x, &sloth_k_alt_mask_));
	sloth_code(x, "K-CTRL-MASK", sloth_primitive(x, &sloth_k_ctrl_mask_));
	sloth_code(x, "K-SHIFT-MASK", sloth_primitive(x, &sloth_k_shift_mask_));
	sloth_code(x, "K-INSERT", sloth_primitive(x, &sloth_k_insert_));
	sloth_code(x, "K-DELETE", sloth_primitive(x, &sloth_k_delete_));
	sloth_code(x, "K-UP", sloth_primitive(x, &sloth_k_up_));
	sloth_code(x, "K-DOWN", sloth_primitive(x, &sloth_k_down_));
	sloth_code(x, "K-LEFT", sloth_primitive(x, &sloth_k_left_));
	sloth_code(x, "K-RIGHT", sloth_primitive(x, &sloth_k_right_));
	sloth_code(x, "K-HOME", sloth_primitive(x, &sloth_k_home_));
	sloth_code(x, "K-PRIOR", sloth_primitive(x, &sloth_k_prior_));
	sloth_code(x, "K-NEXT", sloth_primitive(x, &sloth_k_next_));
	sloth_code(x, "K-END", sloth_primitive(x, &sloth_k_end_));
	sloth_code(x, "K-F1", sloth_primitive(x, &sloth_k_f1_));
	sloth_code(x, "K-F2", sloth_primitive(x, &sloth_k_f2_));
	sloth_code(x, "K-F3", sloth_primitive(x, &sloth_k_f3_));
	sloth_code(x, "K-F4", sloth_primitive(x, &sloth_k_f4_));
	sloth_code(x, "K-F5", sloth_primitive(x, &sloth_k_f5_));
	sloth_code(x, "K-F6", sloth_primitive(x, &sloth_k_f6_));
	sloth_code(x, "K-F7", sloth_primitive(x, &sloth_k_f7_));
	sloth_code(x, "K-F8", sloth_primitive(x, &sloth_k_f8_));
	sloth_code(x, "K-F9", sloth_primitive(x, &sloth_k_f9_));
	sloth_code(x, "K-F10", sloth_primitive(x, &sloth_k_f10_));
	sloth_code(x, "K-F11", sloth_primitive(x, &sloth_k_f11_));
	sloth_code(x, "K-F12", sloth_primitive(x, &sloth_k_f12_));

	/* -- Time and date ---------------------------------- */

	sloth_code(x, "TIME&DATE", sloth_primitive(x, &sloth_time_and_date_));
	sloth_code(x, "MS", sloth_primitive(x, &sloth_ms_));
}
