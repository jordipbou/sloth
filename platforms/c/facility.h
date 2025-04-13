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

void _time_and_date(X* x) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	push(x, tm->tm_sec);
	push(x, tm->tm_min);
	push(x, tm->tm_hour);
	push(x, tm->tm_mday);
	push(x, tm->tm_mon + 1);
	push(x, tm->tm_year + 1900);
}

void _ms(X* x) { 
	sleep_ms(pop(x)); 
}

void _e_key(X* x) {
	/* TODO */
}

void _e_key_to_char(X* x) {
	/* TODO */
}

void _e_key_question(X* x) {
	/* TODO */
}

void _emit_question(X* x) {
	/* TODO */
}

void _key_question(X* x) {
	/* TODO */
}

void _page(X* x) {
	/* TODO */
}

void _at_x_y(X* x) { /* TODO */ }

void bootstrap_facility_wordset(X* x) {
	code(x, "TIME&DATE", primitive(x, &_time_and_date));
	code(x, "MS", primitive(x, &_ms));
	code(x, "EKEY", primitive(x, &_e_key));
	code(x, "EKEY>CHAR", primitive(x, &_e_key_to_char));
	code(x, "EKEY?", primitive(x, &_e_key_question));
	code(x, "EMIT?", primitive(x, &_emit_question));
	code(x, "KEY?", primitive(x, &_key_question));
	code(x, "PAGE", primitive(x, &_page));
	code(x, "AT-XY", primitive(x, &_at_x_y));
}
