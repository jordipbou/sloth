#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"facility.h"
#include"file.h"
#include"locals.h"
#include"memory.h"
#include"cpnbi.h"

#include"sloth_sqlite.h"
#include"sloth_raylib.h"
#include"sloth_plibsys.h"
#include"sloth_systray.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

/* -- Keyboard events generation ----------------------- */
/* TODO To be extracted to an external library */
void sloth_press_key_(X* x) {
	WORD vk = (WORD)sloth_pop(x);
	INPUT ip = {0};
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = vk;
	SendInput(1, &ip, sizeof(INPUT));
}

void sloth_release_key_(X* x) {
	WORD vk = (WORD)sloth_pop(x);
	INPUT ip = {0};
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = vk;
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

/* ----------------------------------------------------- */

/* TEST Add MessageBox to Forth */
void sloth_message_box_(X* x) {
	/* Message boxes need zero ended strings !!!! */
	CELL tl = sloth_pop(x);
	char *ta = (char*)sloth_pop(x);
	CELL ml = sloth_pop(x);
	char *ma = (char*)sloth_pop(x);
	MessageBox(0, ma, ta, 0);
}

/* This function/method of detecting if executing from */
/* a console was suggested by ChatGPT. It seems to work. */
int has_parent_console(void) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        return 1;
    }
    DWORD err = GetLastError();
    if (err == ERROR_ACCESS_DENIED) {
        return 1;
    }
    if (err == ERROR_INVALID_HANDLE) {
        return 0;
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
	int argc = 0;
	LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc);

	bool script_specified = (argc > 1);

	X* x = sloth_new();
	
	sloth_bootstrap(x);
	sloth_bootstrap_facility_wordset(x);
	sloth_bootstrap_file_wordset(x);
	sloth_bootstrap_locals_wordset(x);
	sloth_bootstrap_memory_wordset(x);

	SLOTH_CODE("PRESS-KEY", press_key);
	SLOTH_CODE("RELEASE-KEY", release_key);
	SLOTH_CODE("MESSAGE-BOX", message_box);

	sloth_include(x, ROOT_PATH "4th/ans.4th");

	sloth_bootstrap_sqlite(x);
	sloth_include(x, ROOT_PATH "4th/libs/sloth_sqlite.4th");

	sloth_bootstrap_raylib(x);
	sloth_include(x, ROOT_PATH "4th/libs/sloth_raylib.4th");

	sloth_bootstrap_plibsys(x);

	sloth_bootstrap_systray(x);
	sloth_set_root_path(x, ROOT_PATH "4th/");

  if (has_parent_console()) {
		/* Console */
		FILE *fp;
		freopen_s(&fp, "CONIN$", "w", stdin);
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
	
		/* Optional: disable buffering so output shows instantly */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

		if (script_specified) {
			char script_name[500];
			wcstombs(script_name, argvw[1], 500);
			sloth_include(x, script_name);
		} else {
			sloth_repl(x);
		}

  } else {
		/* Non-console (service, GUI, systray) */
		sloth_include(x, "test.4th");
	}

	sloth_free(x);

	return 0;
}
