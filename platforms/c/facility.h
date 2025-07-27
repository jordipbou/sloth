#include"sloth.h"
#include<time.h>
#include<stdio.h>

#ifdef _WIN32
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>

    int kbhit(void) {
        struct termios oldt, newt;
        int ch;
        int oldf;

        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

        ch = getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);

        if (ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }

        return 0;
    }
#endif

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

/* == Facility words =================================== */

void sloth_at_x_y_(X* x) {
	int _y = (int)sloth_pop(x);
	int _x = (int)sloth_pop(x);
	printf("\x1b[%d;%dH", _y + 1, _x + 1);
}

void sloth_key_question_(X* x) {
	sloth_push(x, kbhit() == 1 ? -1 : 0);
}

void sloth_page_(X* x) {
	printf("\033\143");
}

/* == Facility extension words ========================= */

/* -- Structures --------------------------------------- */

void sloth_fetch_plus_(X* x) {
	sloth_fetch_(x);
	sloth_plus_(x);
}

/* +FIELD has been implemented here as if Forth code. That */
/* makes use of DOES> (and its a good way to check how to */
/* implement a word with DOES> from C) but it could be made */
/* without CREATE DOES> just creating a new word that has */
/* @ + directly compiled in its own code. */
void sloth_plus_field_(X* x) {
	sloth_create_(x);
	sloth_over_(x);
	sloth_comma(x, sloth_pop(x));
	sloth_plus_(x);
	sloth_do_does(x, sloth_get_xt(x, sloth_find_word(x, "@+")));
}

void sloth_begin_structure_(X* x) {
	sloth_create_(x);
	sloth_here_(x);
	sloth_push(x, 0);
	sloth_comma(x, 0);
	sloth_do_does(x, sloth_get_xt(x, sloth_find_word(x, "@")));
}

void sloth_c_field_colon_(X* x) {
	/* sloth_c_aligned_(x); */
	sloth_push(x, 1*suCHAR);
	sloth_plus_field_(x);
}

void sloth_field_colon_(X* x) {
	sloth_push(x, sloth_aligned(sloth_pop(x)));
	sloth_push(x, 1*sCELL);
	sloth_plus_field_(x);
}

/* -- Keyboard input ----------------------------------- */

#define SLOTH_KB_ESC 27

#define SLOTH_KB_F1 1001
#define SLOTH_KB_F2 1002
#define SLOTH_KB_F3 1003
#define SLOTH_KB_F4 1004
#define SLOTH_KB_F5 1005
#define SLOTH_KB_F6 1006
#define SLOTH_KB_F7 1007
#define SLOTH_KB_F8 1008
#define SLOTH_KB_F9 1009
#define SLOTH_KB_F10 1010
#define SLOTH_KB_F11 1011
#define SLOTH_KB_F12 1012

#define SLOTH_KB_UP 2001
#define SLOTH_KB_DOWN 2002
#define SLOTH_KB_RIGHT 2003
#define SLOTH_KB_LEFT 2004

/* TODO: I need to think (or adapt) keycodes */
void sloth_e_key_(X* x) {
	int e;
	switch (e = getch()) {
	case 27: 
		if (kbhit()) {
			/* ANSI escape sequence (Linux only) */
			switch (e = getch()) {
			case 'O': 
				switch (e = getch()) {
				case 'P': e = SLOTH_KB_F1; break;
				case 'Q': e = SLOTH_KB_F2; break;
				case 'R': e = SLOTH_KB_F3; break;
				case 'S': e = SLOTH_KB_F4; break;
				}
				break;
			case '[':
				switch (e = getch()) {
				case 'A': e = SLOTH_KB_UP; break;
				case 'B': e = SLOTH_KB_DOWN; break;
				case 'C': e = SLOTH_KB_RIGHT; break;
				case 'D': e = SLOTH_KB_LEFT; break;
				case '1':
					switch (e = getch()) {
					case '1': getch(); e = SLOTH_KB_F1; break;
					case '2': getch(); e = SLOTH_KB_F2; break;
					case '3': getch(); e = SLOTH_KB_F3; break;
					case '4': getch(); e = SLOTH_KB_F4; break;
					case '5': getch(); e = SLOTH_KB_F5; break;
					case '7': getch(); e = SLOTH_KB_F6; break;
					case '8': getch(); e = SLOTH_KB_F7; break;
					case '9': getch(); e = SLOTH_KB_F8; break;
					}
					break;
				case '2':
					switch (e = getch()) {
					case '0': getch(); e = SLOTH_KB_F9; break;
					case '1': getch(); e = SLOTH_KB_F10; break;
					case '3': getch(); e = SLOTH_KB_F11; break;
					case '4': getch(); e = SLOTH_KB_F12; break;
					}
					break;
				}
				break;
			}
		}
		break;
	}
	sloth_push(x, e);
}

void sloth_e_key_to_char_(X* x) {
	/* TODO */
}

void sloth_e_key_to_f_key_(X* x) {
	/* TODO */
}

void sloth_e_key_question_(X* x) {
	/* TODO */
}

void sloth_emit_question_(X* x) {
	/* TODO */
}

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

void sloth_bootstrap_facility_wordset(X* x) {

	/* == Facility words ================================= */

	sloth_code(x, "AT-XY", sloth_primitive(x, &sloth_at_x_y_));
	sloth_code(x, "KEY?", sloth_primitive(x, &sloth_key_question_));
	sloth_code(x, "PAGE", sloth_primitive(x, &sloth_page_));

	/* == Facility extension words ======================= */

	/* -- Structures ------------------------------------- */

	/* Move @+ to INTERNAL wordlist, necessary for +FIELD ?? */
	sloth_code(x, "@+", sloth_primitive(x, &sloth_fetch_plus_));
	sloth_code(x, "+FIELD", sloth_primitive(x, &sloth_plus_field_));
	sloth_code(x, "BEGIN-STRUCTURE", sloth_primitive(x, &sloth_begin_structure_));
	sloth_code(x, "CFIELD:", sloth_primitive(x, &sloth_c_field_colon_));
	sloth_code(x, "FIELD:", sloth_primitive(x, &sloth_field_colon_));

	/* -- Keyboard input --------------------------------- */

	sloth_code(x, "EKEY", sloth_primitive(x, &sloth_e_key_));
	sloth_code(x, "EKEY>CHAR", sloth_primitive(x, &sloth_e_key_to_char_));
	sloth_code(x, "EKEY>FKEY", sloth_primitive(x, &sloth_e_key_to_f_key_));
	sloth_code(x, "EKEY?", sloth_primitive(x, &sloth_e_key_question_));
	sloth_code(x, "EMIT?", sloth_primitive(x, &sloth_emit_question_));

	/* -- Time and date ---------------------------------- */

	sloth_code(x, "TIME&DATE", sloth_primitive(x, &sloth_time_and_date_));
	sloth_code(x, "MS", sloth_primitive(x, &sloth_ms_));
}
