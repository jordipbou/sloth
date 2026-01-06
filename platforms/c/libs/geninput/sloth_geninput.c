#include "sloth_geninput.h"

#define SLOTH_GENINPUT_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_geninput_##f##_));

#ifdef WINDOWS
#include <windows.h>
#include <stdio.h>

void sloth_geninput_press_key_(X* x) {
	CELL vk = sloth_pop(x);
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = vk; 
	input.ki.wScan = 0;
	input.ki.dwFlags = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = 0;
	SendInput(1, &input, sizeof(INPUT));
}

void sloth_geninput_release_key_(X* x) {
	CELL vk = sloth_pop(x);
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = vk; 
	input.ki.wScan = 0;
	input.ki.dwFlags = KEYEVENTF_KEYUP;
	input.ki.time = 0;
	input.ki.dwExtraInfo = 0;
	SendInput(1, &input, sizeof(INPUT));
}

void sloth_bootstrap_geninput(X* x) {
	SLOTH_GENINPUT_CODE("PRESS-KEY", press_key);
	SLOTH_GENINPUT_CODE("RELEASE-KEY", release_key);
}
#endif
