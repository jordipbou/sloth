#define SLOTH_IMPLEMENTATION
#include"sloth.h"
#include"raylib.h"

#define SLOTH2RAYLIB(f) void sloth2raylib_##f##_(X* x) { f(); }
#define SLOTH2RAYLIB_BOOL(f) void sloth2raylib_##f##_(X* x) { sloth_push(x, f() ? -1 : 0); }
#define SLOTH2RAYLIB_1ARG(f, t) void sloth2raylib_##f##_(X* x) { f(t##sloth_pop(x)); }

#define SLOTH2RAYLIB_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2raylib_##f##_));

/* == MODULE RCORE ===================================== */

/* -- Window-related functions ------------------------- */

/* TODO Find a good way to work with strings! */
/* Although I've added a zero at the end of string */
/* literals to sloth, that does not work for non */
/* compiled strings. */

void sloth2raylib_InitWindow_(X* x) {
	/* char title[255]; */
	CELL l = sloth_pop(x); /* Can be ignored */
	CELL a = sloth_pop(x);
	CELL height = sloth_pop(x);
	CELL width = sloth_pop(x);
	/*
	int i;
	for (i = 0; i < l; i++) title[i] = sloth_cfetch(x, a + i);
	title[l] = 0;
	InitWindow(width, height, title);
	*/
	InitWindow(width, height, a);
}

SLOTH2RAYLIB(CloseWindow)
SLOTH2RAYLIB_BOOL(WindowShouldClose)

/* -- Drawing-related functions ------------------------ */

SLOTH2RAYLIB_1ARG(ClearBackground, *(Color*))
SLOTH2RAYLIB(EndDrawing)

/* -- Timing-related functions ------------------------- */

SLOTH2RAYLIB_1ARG(SetTargetFPS, (int))

SLOTH2RAYLIB(BeginDrawing)

/* -- Input handling functions ------------------------- */

/* Keyboard */

void sloth2raylib_is_key_pressed_(X* x) {
	sloth_push(x, IsKeyPressed(sloth_pop(x)) ? -1 : 0);
}

/* -- Gestures and touch handling functions ------------ */

void raylib_is_gesture_detected_(X* x) {
	sloth_push(x, IsGestureDetected(sloth_pop(x)) ? -1 : 0);
}

/* == MODULE RSHAPES =================================== */

/* -- Basic shapes drawing functions ------------------- */

void sloth2raylib_DrawRectangle_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	int height = (int)sloth_pop(x);
	int width = (int)sloth_pop(x);
	int pos_y = (int)sloth_pop(x);
	int pos_x = (int)sloth_pop(x);
	DrawRectangle(pos_x, pos_y, width, height, color);
}

/* == MODULE RTEXT ===================================== */

/* -- Text drawing functions --------------------------- */

void sloth2raylib_DrawText_(X* x) {
	char text[255];
	CELL font_size, pos_y, pos_x, l, a, i;
	Color* color = (Color*)sloth_pop(x);
	/*
	color.a = (unsigned char)sloth_pop(x);
	color.b = (unsigned char)sloth_pop(x);
	color.g = (unsigned char)sloth_pop(x);
	color.r = (unsigned char)sloth_pop(x);
	*/
	font_size = sloth_pop(x);
	pos_y = sloth_pop(x);
	pos_x = sloth_pop(x);
	l = sloth_pop(x);
	a = sloth_pop(x);
	for (i = 0; i < l; i++) text[i] = sloth_cfetch(x, a + i);
	text[l] = 0;
	DrawText(text, pos_x, pos_y, font_size, *color);
}

void bootstrap_raylib(X* x) {
	/* Create a new wordlist for raylib words and set it */
	/* as current. */

	/* wordlist(x, "RAYLIB"); */
	sloth_evaluate(x, "wordlist dup constant raylib-wordlist set-current");
	sloth_evaluate(x, "get-order 1+ raylib-wordlist swap set-order");

	/* == MODULE RCORE ===================================== */
	
	/* -- Window-related functions ------------------------- */

	SLOTH2RAYLIB_CODE("INIT-WINDOW", InitWindow);
	SLOTH2RAYLIB_CODE("CLOSE-WINDOW", CloseWindow);

	/* -- Drawing-related functions ------------------------ */

	SLOTH2RAYLIB_CODE("CLEAR-BACKGROUND", ClearBackground);
	SLOTH2RAYLIB_CODE("BEGIN-DRAWING", BeginDrawing);
	SLOTH2RAYLIB_CODE("END-DRAWING", EndDrawing);


	/* -- Timing-related functions ------------------------- */

	SLOTH2RAYLIB_CODE("SET-TARGET-FPS", SetTargetFPS);

	SLOTH2RAYLIB_CODE("WINDOW-SHOULD-CLOSE", WindowShouldClose);

	sloth_code(x, "IS-KEY-PRESSED", sloth_primitive(x, &sloth2raylib_is_key_pressed_));

	sloth_code(x, "IS-GESTURE-DETECTED", sloth_primitive(x, &raylib_is_gesture_detected_));

	/* == MODULE RSHAPES ================================= */

	/* -- Basic shapes drawing functions ----------------- */

	SLOTH2RAYLIB_CODE("DRAW-RECTANGLE", DrawRectangle);

	/* == MODULE RTEXT =================================== */

	/* -- Text drawing functions ------------------------- */

	SLOTH2RAYLIB_CODE("DRAW-TEXT", DrawText);

	/* Restore wordlist */

	sloth_evaluate(x, "forth-wordlist set-current");
}

int main(int argc, char** argv) {
	X* x = sloth_new();

	sloth_bootstrap(x);

	/* It's important to include ans.4th before */
	/* bootstrapping raylib to allow evaluating */
	/* forth code in bootstrap_raylib function. */
	sloth_include(x, "../../../4th/ans.4th");

	bootstrap_raylib(x);

	sloth_include(x, "../raylib.4th");

	if (argc == 1) {
		sloth_repl(x);
	} else {
		sloth_include(x, argv[1]);
	}

	sloth_free(x);
}
