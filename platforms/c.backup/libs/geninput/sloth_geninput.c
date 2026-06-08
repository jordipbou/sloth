#include "sloth_geninput.h"

#define SLOTH_GENINPUT_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_geninput_##f##_));

#ifdef WINDOWS
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL3/SDL_scancode.h>

typedef struct WinScancode {
    uint8_t scancode;
    uint8_t extended;
} WinScancode;

/*
 * Windows Set-1 scancode table indexed by SDL_Scancode.
 * Extended keys are flagged explicitly.
 */
static const WinScancode s_scancode_table[SDL_SCANCODE_COUNT] = {

    /* Letters */
    [SDL_SCANCODE_A] = {0x1E, 0},
    [SDL_SCANCODE_B] = {0x30, 0},
    [SDL_SCANCODE_C] = {0x2E, 0},
    [SDL_SCANCODE_D] = {0x20, 0},
    [SDL_SCANCODE_E] = {0x12, 0},
    [SDL_SCANCODE_F] = {0x21, 0},
    [SDL_SCANCODE_G] = {0x22, 0},
    [SDL_SCANCODE_H] = {0x23, 0},
    [SDL_SCANCODE_I] = {0x17, 0},
    [SDL_SCANCODE_J] = {0x24, 0},
    [SDL_SCANCODE_K] = {0x25, 0},
    [SDL_SCANCODE_L] = {0x26, 0},
    [SDL_SCANCODE_M] = {0x32, 0},
    [SDL_SCANCODE_N] = {0x31, 0},
    [SDL_SCANCODE_O] = {0x18, 0},
    [SDL_SCANCODE_P] = {0x19, 0},
    [SDL_SCANCODE_Q] = {0x10, 0},
    [SDL_SCANCODE_R] = {0x13, 0},
    [SDL_SCANCODE_S] = {0x1F, 0},
    [SDL_SCANCODE_T] = {0x14, 0},
    [SDL_SCANCODE_U] = {0x16, 0},
    [SDL_SCANCODE_V] = {0x2F, 0},
    [SDL_SCANCODE_W] = {0x11, 0},
    [SDL_SCANCODE_X] = {0x2D, 0},
    [SDL_SCANCODE_Y] = {0x15, 0},
    [SDL_SCANCODE_Z] = {0x2C, 0},

    /* Numbers (top row) */
    [SDL_SCANCODE_1] = {0x02, 0},
    [SDL_SCANCODE_2] = {0x03, 0},
    [SDL_SCANCODE_3] = {0x04, 0},
    [SDL_SCANCODE_4] = {0x05, 0},
    [SDL_SCANCODE_5] = {0x06, 0},
    [SDL_SCANCODE_6] = {0x07, 0},
    [SDL_SCANCODE_7] = {0x08, 0},
    [SDL_SCANCODE_8] = {0x09, 0},
    [SDL_SCANCODE_9] = {0x0A, 0},
    [SDL_SCANCODE_0] = {0x0B, 0},

    /* Symbols */
    [SDL_SCANCODE_MINUS]        = {0x0C, 0},
    [SDL_SCANCODE_EQUALS]       = {0x0D, 0},
    [SDL_SCANCODE_LEFTBRACKET]  = {0x1A, 0},
    [SDL_SCANCODE_RIGHTBRACKET] = {0x1B, 0},
    [SDL_SCANCODE_BACKSLASH]    = {0x2B, 0},
    [SDL_SCANCODE_SEMICOLON]    = {0x27, 0},
    [SDL_SCANCODE_APOSTROPHE]   = {0x28, 0},
    [SDL_SCANCODE_GRAVE]        = {0x29, 0},
    [SDL_SCANCODE_COMMA]        = {0x33, 0},
    [SDL_SCANCODE_PERIOD]       = {0x34, 0},
    [SDL_SCANCODE_SLASH]        = {0x35, 0},

    /* Control keys */
    [SDL_SCANCODE_RETURN]    = {0x1C, 0},
    [SDL_SCANCODE_ESCAPE]   = {0x01, 0},
    [SDL_SCANCODE_BACKSPACE]= {0x0E, 0},
    [SDL_SCANCODE_TAB]      = {0x0F, 0},
    [SDL_SCANCODE_SPACE]    = {0x39, 0},
    [SDL_SCANCODE_CAPSLOCK] = {0x3A, 0},

    /* Function keys */
    [SDL_SCANCODE_F1]  = {0x3B, 0},
    [SDL_SCANCODE_F2]  = {0x3C, 0},
    [SDL_SCANCODE_F3]  = {0x3D, 0},
    [SDL_SCANCODE_F4]  = {0x3E, 0},
    [SDL_SCANCODE_F5]  = {0x3F, 0},
    [SDL_SCANCODE_F6]  = {0x40, 0},
    [SDL_SCANCODE_F7]  = {0x41, 0},
    [SDL_SCANCODE_F8]  = {0x42, 0},
    [SDL_SCANCODE_F9]  = {0x43, 0},
    [SDL_SCANCODE_F10] = {0x44, 0},
    [SDL_SCANCODE_F11] = {0x57, 0},
    [SDL_SCANCODE_F12] = {0x58, 0},

    /* Modifiers */
    [SDL_SCANCODE_LSHIFT] = {0x2A, 0},
    [SDL_SCANCODE_RSHIFT] = {0x36, 0},
    [SDL_SCANCODE_LCTRL]  = {0x1D, 0},
    [SDL_SCANCODE_RCTRL]  = {0x1D, 1},
    [SDL_SCANCODE_LALT]   = {0x38, 0},
    [SDL_SCANCODE_RALT]   = {0x38, 1},
    [SDL_SCANCODE_LGUI]   = {0x5B, 1},
    [SDL_SCANCODE_RGUI]   = {0x5C, 1},

    /* Navigation (extended) */
    [SDL_SCANCODE_UP]       = {0x48, 1},
    [SDL_SCANCODE_DOWN]     = {0x50, 1},
    [SDL_SCANCODE_LEFT]     = {0x4B, 1},
    [SDL_SCANCODE_RIGHT]    = {0x4D, 1},
    [SDL_SCANCODE_INSERT]   = {0x52, 1},
    [SDL_SCANCODE_DELETE]   = {0x53, 1},
    [SDL_SCANCODE_HOME]     = {0x47, 1},
    [SDL_SCANCODE_END]      = {0x4F, 1},
    [SDL_SCANCODE_PAGEUP]   = {0x49, 1},
    [SDL_SCANCODE_PAGEDOWN] = {0x51, 1},

    /* Keypad */
    [SDL_SCANCODE_KP_0] = {0x52, 0},
    [SDL_SCANCODE_KP_1] = {0x4F, 0},
    [SDL_SCANCODE_KP_2] = {0x50, 0},
    [SDL_SCANCODE_KP_3] = {0x51, 0},
    [SDL_SCANCODE_KP_4] = {0x4B, 0},
    [SDL_SCANCODE_KP_5] = {0x4C, 0},
    [SDL_SCANCODE_KP_6] = {0x4D, 0},
    [SDL_SCANCODE_KP_7] = {0x47, 0},
    [SDL_SCANCODE_KP_8] = {0x48, 0},
    [SDL_SCANCODE_KP_9] = {0x49, 0},
    [SDL_SCANCODE_KP_ENTER] = {0x1C, 1},
    [SDL_SCANCODE_KP_PLUS]  = {0x4E, 0},
    [SDL_SCANCODE_KP_MINUS] = {0x4A, 0},
    [SDL_SCANCODE_KP_MULTIPLY] = {0x37, 0},
    [SDL_SCANCODE_KP_DIVIDE]   = {0x35, 1},
};

/* Public API */
WinScancode SDLScancodeToWinScancode(SDL_Scancode sdl)
{
    if ((unsigned)sdl >= SDL_SCANCODE_COUNT)
        return (WinScancode){0, 0};

    return s_scancode_table[sdl];
}

void SendKey(SDL_Scancode sdl, int pressed)
{
    WinScancode w = SDLScancodeToWinScancode(sdl);
    if (!w.scancode)
        return;

    INPUT in = {0};
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = w.scancode;
    in.ki.dwFlags = KEYEVENTF_SCANCODE |
                    (pressed ? 0 : KEYEVENTF_KEYUP) |
                    (w.extended ? KEYEVENTF_EXTENDEDKEY : 0);

    SendInput(1, &in, sizeof(INPUT));
}

void sloth_geninput_press_key_(X* x) { SendKey(sloth_pop(x), 1); }
void sloth_geninput_release_key_(X* x) { SendKey(sloth_pop(x), 0); }

void sloth_bootstrap_geninput(X* x) {
	SLOTH_GENINPUT_CODE("PRESS-KEY", press_key);
	SLOTH_GENINPUT_CODE("RELEASE-KEY", release_key);
}
#endif
