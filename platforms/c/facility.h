#include"sloth.h"
#include<time.h>

/* -- Milliseconds multiplatform implementation -------- */
/* Taken from: https://stackoverflow.com/a/28827188 */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   /* for nanosleep */
#else
#include <unistd.h> /* for usleep */
#endif

void sleep_ms(int milliseconds){ /* cross-platform sleep */
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

void sloth_e_key_(X* x) {
	/* TODO */
}

void sloth_e_key_to_char_(X* x) {
	/* TODO */
}

void sloth_e_key_question_(X* x) {
	/* TODO */
}

void sloth_emit_question_(X* x) {
	/* TODO */
}

void sloth_key_question_(X* x) {
	/* TODO */
}

void sloth_page_(X* x) {
	printf("\033\143");
}

void sloth_at_x_y_(X* x) { /* TODO */ }

void sloth_bootstrap_facility_wordset(X* x) {
	sloth_code(x, "TIME&DATE", sloth_primitive(x, &sloth_time_and_date_));
	sloth_code(x, "MS", sloth_primitive(x, &sloth_ms_));
	sloth_code(x, "EKEY", sloth_primitive(x, &sloth_e_key_));
	sloth_code(x, "EKEY>CHAR", sloth_primitive(x, &sloth_e_key_to_char_));
	sloth_code(x, "EKEY?", sloth_primitive(x, &sloth_e_key_question_));
	sloth_code(x, "EMIT?", sloth_primitive(x, &sloth_emit_question_));
	sloth_code(x, "KEY?", sloth_primitive(x, &sloth_key_question_));
	sloth_code(x, "PAGE", sloth_primitive(x, &sloth_page_));
	sloth_code(x, "AT-XY", sloth_primitive(x, &sloth_at_x_y_));
}
