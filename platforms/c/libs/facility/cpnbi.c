#include<stdio.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include "cpnbi.h"

static HANDLE hStdin;
static DWORD orig_mode;

void cpnbi_init() {
	DWORD mode = 0;

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hStdin, &orig_mode);
	mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT); // optional
	mode |= ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT;
	SetConsoleMode(hStdin, mode);
}

int cpnbi__process_event(INPUT_RECORD* record) {
	int key = CPNBI_KEY_NUL, mod = CPNBI_MOD_NONE;

	if (record->EventType == KEY_EVENT 
	&& record->Event.KeyEvent.bKeyDown) {
		WORD vk = record->Event.KeyEvent.wVirtualKeyCode;
		DWORD ctrl = record->Event.KeyEvent.dwControlKeyState;

		if (ctrl & SHIFT_PRESSED) 
			mod += CPNBI_MOD_SHIFT;
		if (ctrl & LEFT_CTRL_PRESSED || ctrl & RIGHT_CTRL_PRESSED) 
			mod += CPNBI_MOD_CTRL;
		if (ctrl & LEFT_ALT_PRESSED || ctrl & RIGHT_ALT_PRESSED) 
			mod += CPNBI_MOD_ALT;
		
		switch (vk) {
		case VK_ESCAPE: key = CPNBI_KEY_ESCAPE; break;
		case VK_RETURN: key = CPNBI_KEY_ENTER; break;
		case VK_BACK:   key = CPNBI_KEY_BACKSPACE; break;
		case VK_TAB:    key = CPNBI_KEY_TAB; break;
		case VK_UP:     key = CPNBI_KEY_UP; break;
		case VK_DOWN:   key = CPNBI_KEY_DOWN; break;
		case VK_LEFT:   key = CPNBI_KEY_LEFT; break;
		case VK_RIGHT:  key = CPNBI_KEY_RIGHT; break;
		case VK_HOME:   key = CPNBI_KEY_HOME; break;
		case VK_END:    key = CPNBI_KEY_END; break;
		case VK_INSERT: key = CPNBI_KEY_INSERT; break;
		case VK_DELETE: key = CPNBI_KEY_DELETE; break;
		case VK_PRIOR:  key = CPNBI_KEY_PAGE_UP; break;
		case VK_NEXT:   key = CPNBI_KEY_PAGE_DOWN; break;
		case VK_F1:     key = CPNBI_KEY_F1; break;
		case VK_F2:     key = CPNBI_KEY_F2; break;
		case VK_F3:     key = CPNBI_KEY_F3; break;
		case VK_F4:     key = CPNBI_KEY_F4; break;
		case VK_F5:     key = CPNBI_KEY_F5; break;
		case VK_F6:     key = CPNBI_KEY_F6; break;
		case VK_F7:     key = CPNBI_KEY_F7; break;
		case VK_F8:     key = CPNBI_KEY_F8; break;
		case VK_F9:     key = CPNBI_KEY_F9; break;
		case VK_F10:    key = CPNBI_KEY_F10; break;
		case VK_F11:    key = CPNBI_KEY_F11; break;
		case VK_F12:    key = CPNBI_KEY_F12; break;
		default:
			int ch = record->Event.KeyEvent.uChar.AsciiChar;
			if (ch >= 32 && ch <= 126) {
        key = ch;
	    }
		}
	}

	return key + mod;
}

int cpnbi_is_char_available() {
	DWORD count;
	INPUT_RECORD record;

	PeekConsoleInput(hStdin, &record, 1, &count);

	if (count > 0) {
		int res = cpnbi__process_event(&record);

		if (res >= 32 && res <= 126) return 1;
		else {
			/* Consume non useful event */
			ReadConsoleInput(hStdin, &record, 1, &count);
		}
	}

	return 0;
}

int cpnbi_is_event_available() {
	DWORD count;
	INPUT_RECORD record;

	PeekConsoleInput(hStdin, &record, 1, &count);

	if (count > 0) {
		int res = cpnbi__process_event(&record);

		if ((res % 1000) != 0) return 1;
		else {
			/* Consume non useful event */
			ReadConsoleInput(hStdin, &record, 1, &count);
		}
	}

	return 0;
}

int cpnbi_get_char() {
	DWORD count;
	INPUT_RECORD record;

	while (1) {
		ReadConsoleInput(hStdin, &record, 1, &count);

		if (count > 0) {
			int res = cpnbi__process_event(&record);

			if (res >= 32 && res <= 126) return res;
		}
	}
}

int cpnbi_get_event() {
	DWORD count;
	INPUT_RECORD record;

	while (1) {
		ReadConsoleInput(hStdin, &record, 1, &count);

		if (count > 0) {
			int res = cpnbi__process_event(&record);

			if ((res % 1000) != 0) return res;
		}
	}
}

void cpnbi_shutdown() {
	SetConsoleMode(hStdin, orig_mode);
}

#else

/* LINUX IMPLEMENTATION */

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include "cpnbi.h"

static struct termios orig_termios;

void cpnbi_init() { }

int cpnbi__getch() {
	struct termios oldt, newt;
	int ch;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return ch;
}

int cpnbi_is_char_available(void) {
	int ch;
	int oldf;
	
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	
	ch = cpnbi__getch();
	
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	
	if (ch != EOF && ch >= 32 && ch <= 126) {
		ungetc(ch, stdin);
		return 1;
	} else {
		return 0;
	}
}

int cpnbi_is_event_available(void) {
    int ch;
    int oldf;

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = cpnbi__getch();

    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int cpnbi_get_char() {
	int e;

	if ((e = cpnbi__getch()) >= 32 && e <= 126) {
		return e;
	} else {
		return CPNBI_KEY_NUL;
	}
}

int cpnbi_get_event() {
	int e, key = CPNBI_KEY_NUL, mod = CPNBI_MOD_NONE;

	if ((key = e = cpnbi__getch()) == 27 
	&& cpnbi_is_event_available()) {
		/* ANSI escape sequence (Linux only) */
		switch (e = cpnbi__getch()) {
		case 'O':
			/* F1 to F4 without modifiers */
			switch (e = cpnbi__getch()) {
			case 'P': key = CPNBI_KEY_F1; break;
			case 'Q': key = CPNBI_KEY_F2; break;
			case 'R': key = CPNBI_KEY_F3; break;
			case 'S': key = CPNBI_KEY_F4; break;
			}
			break;
		case '[':
			/* ESC [ control sequence introducer */
			switch (e = cpnbi__getch()) {
			/* UP, DOWN, RIGHT, LEFT, HOME, END without modifiers */
			case 'A': key = CPNBI_KEY_UP; break;
			case 'B': key = CPNBI_KEY_DOWN; break;
			case 'C': key = CPNBI_KEY_RIGHT; break;
			case 'D': key = CPNBI_KEY_LEFT; break;
			case 'H': key = CPNBI_KEY_HOME; break;
			case 'F': key = CPNBI_KEY_END; break;
			default:
				/* From this point on, the sequence can include */
				/* modifiers on it. */
				switch (e) {
				case '1':
					switch (e = cpnbi__getch()) {
					case '~': key = CPNBI_KEY_HOME; break;
					case ';':
						switch (mod = cpnbi__getch()) {
						case '2': mod = CPNBI_MOD_SHIFT; break;
						case '3': mod = CPNBI_MOD_ALT; break;
						case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
						case '5': mod = CPNBI_MOD_CTRL; break;
						case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
						case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						}
						switch (e = cpnbi__getch()) {
						case 'A': key = CPNBI_KEY_UP; break;
						case 'B': key = CPNBI_KEY_DOWN; break;
						case 'C': key = CPNBI_KEY_RIGHT; break;
						case 'D': key = CPNBI_KEY_LEFT; break;
						case 'H': key = CPNBI_KEY_HOME; break;
						case 'F': key = CPNBI_KEY_END; break;
						}
						break;
					default: 
						/* Number between 11 and 19 */
						switch (e) {
						case '1': key = CPNBI_KEY_F1; break;
						case '2': key = CPNBI_KEY_F2; break;
						case '3': key = CPNBI_KEY_F3; break;
						case '4': key = CPNBI_KEY_F4; break;
						case '5': key = CPNBI_KEY_F5; break;
						case '7': key = CPNBI_KEY_F6; break;
						case '8': key = CPNBI_KEY_F7; break;
						case '9': key = CPNBI_KEY_F8; break;
						}
						switch (e = cpnbi__getch()) {
						case '~': break;
						case ';':
							switch (mod = cpnbi__getch()) {
							case '2': mod = CPNBI_MOD_SHIFT; break;
							case '3': mod = CPNBI_MOD_ALT; break;
							case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
							case '5': mod = CPNBI_MOD_CTRL; break;
							case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
							case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
							case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
							}
							cpnbi__getch();
							break;
						}
						break;
					}
					break;
				case '2':
					switch (e = cpnbi__getch()) {
					case '~': key = CPNBI_KEY_INSERT; break;
					case ';':
						key = CPNBI_KEY_INSERT;
						switch (mod = cpnbi__getch()) {
						case '2': mod = CPNBI_MOD_SHIFT; break;
						case '3': mod = CPNBI_MOD_ALT; break;
						case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
						case '5': mod = CPNBI_MOD_CTRL; break;
						case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
						case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						}
						cpnbi__getch(); /* Consume ~ character */
						break;
					default: 
						/* Number between 20 and 24 */
						switch (e) {
						case '0': key = CPNBI_KEY_F9; break;
						case '1': key = CPNBI_KEY_F10; break;
						case '3': key = CPNBI_KEY_F11; break;
						case '4': key = CPNBI_KEY_F12; break;
						}
						switch (e = cpnbi__getch()) {
						case '~': break;
						case ';':
							switch (mod = cpnbi__getch()) {
							case '2': mod = CPNBI_MOD_SHIFT; break;
							case '3': mod = CPNBI_MOD_ALT; break;
							case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
							case '5': mod = CPNBI_MOD_CTRL; break;
							case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
							case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
							case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
							}
							cpnbi__getch();
							break;
						}
						break;
					}
					break;
				case '3':
					switch (e = cpnbi__getch()) {
					case '~': key = CPNBI_KEY_DELETE; break;
					case ';':
						key = CPNBI_KEY_DELETE;
						switch (mod = cpnbi__getch()) {
						case '2': mod = CPNBI_MOD_SHIFT; break;
						case '3': mod = CPNBI_MOD_ALT; break;
						case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
						case '5': mod = CPNBI_MOD_CTRL; break;
						case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
						case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						}
						cpnbi__getch(); /* Consume ~ character */
						break;
					}
					break;
				case '4':
					switch (e = cpnbi__getch()) {
					case '~': key = CPNBI_KEY_END; break;
					case ';':
						key = CPNBI_KEY_END;
						switch (mod = cpnbi__getch()) {
						case '2': mod = CPNBI_MOD_SHIFT; break;
						case '3': mod = CPNBI_MOD_ALT; break;
						case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
						case '5': mod = CPNBI_MOD_CTRL; break;
						case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
						case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						}
						cpnbi__getch(); /* Consume ~ character */
						break;
					}
					break;
				case '5':
					switch (e = cpnbi__getch()) {
					case '~': key = CPNBI_KEY_PAGE_UP; break;
					case ';':
						key = CPNBI_KEY_PAGE_UP;
						switch (mod = cpnbi__getch()) {
						case '2': mod = CPNBI_MOD_SHIFT; break;
						case '3': mod = CPNBI_MOD_ALT; break;
						case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
						case '5': mod = CPNBI_MOD_CTRL; break;
						case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
						case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						}
						cpnbi__getch(); /* Consume ~ character */
						break;
					}
					break;
				case '6':
					switch (e = cpnbi__getch()) {
					case '~': key = CPNBI_KEY_PAGE_DOWN; break;
					case ';':
						key = CPNBI_KEY_PAGE_DOWN;
						switch (mod = cpnbi__getch()) {
						case '2': mod = CPNBI_MOD_SHIFT; break;
						case '3': mod = CPNBI_MOD_ALT; break;
						case '4': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT; break;
						case '5': mod = CPNBI_MOD_CTRL; break;
						case '6': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_CTRL; break;
						case '7': mod = CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						case '8': mod = CPNBI_MOD_SHIFT + CPNBI_MOD_ALT + CPNBI_MOD_CTRL; break;
						}
						cpnbi__getch(); /* Consume ~ character */
						break;
					}
					break;
				}
				break;
			}
			break;
		}
	} else if (e == 0) {
		switch (e = cpnbi__getch()) {
		case 59: key = CPNBI_KEY_F1; break;
		case 60: key = CPNBI_KEY_F2; break;
		case 61: key = CPNBI_KEY_F3; break;
		case 62: key = CPNBI_KEY_F4; break;
		case 63: key = CPNBI_KEY_F5; break;
		case 64: key = CPNBI_KEY_F6; break;
		case 65: key = CPNBI_KEY_F7; break;
		case 66: key = CPNBI_KEY_F8; break;
		case 67: key = CPNBI_KEY_F9; break;
		case 68: key = CPNBI_KEY_F10; break;
		case 133: key = CPNBI_KEY_F11; break;
		case 134: key = CPNBI_KEY_F12; break;
		}
	} else if (e == 224) {
		switch (e = cpnbi__getch()) {
		case 134: key = CPNBI_KEY_F12; break;
		case 72: key = CPNBI_KEY_UP; break;
		case 80: key = CPNBI_KEY_DOWN; break;
		case 75: key = CPNBI_KEY_LEFT; break;
		case 77: key = CPNBI_KEY_RIGHT; break;
		case 82: key = CPNBI_KEY_INSERT; break;
		case 83: key = CPNBI_KEY_DELETE; break;
		case 71: key = CPNBI_KEY_HOME; break;
		case 79: key = CPNBI_KEY_END; break;
		case 73: key = CPNBI_KEY_PAGE_UP; break;
		case 81: key = CPNBI_KEY_PAGE_DOWN; break;
		}
	}

	return key + mod;
}

void cpnbi_shutdown() { }
#endif
