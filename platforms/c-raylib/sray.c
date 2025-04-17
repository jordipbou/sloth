#define SLOTH_IMPLEMENTATION
#include"sloth.h"
#include"raylib.h"

#define SLOTH2RAYLIB(f) void sloth_raylib_##f##_(X* x) { f(); }
#define SRAY_FUNC_I(w, f) void raylib_##w##_(X* x) { f((int)sloth_pop(x)); }
#define SRAY_FUNC_B(w, f) void raylib_##w##_(X* x) { sloth_push(x, f() ? -1 : 0); }
#define SRAY_FUNC_1(f, t) void sloth_raylib_##f##_(X* x) { f(t##sloth_pop(x)); }

#define SLOTH2RAYLIB_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_raylib_##f##_));

/* == MODULE RCORE ===================================== */

/* -- Window-related functions ------------------------- */

void raylib_init_window_(X* x) {
	char title[255];
	CELL l = sloth_pop(x);
	CELL a = sloth_pop(x);
	CELL height = sloth_pop(x);
	CELL width = sloth_pop(x);
	int i;
	for (i = 0; i < l; i++) title[i] = sloth_cfetch(x, a + i);
	title[l] = 0;
	InitWindow(width, height, title);
}

SLOTH2RAYLIB(CloseWindow)
SRAY_FUNC_B(window_should_close, WindowShouldClose)

/* -- Drawing-related functions ------------------------ */

SRAY_FUNC_1(ClearBackground, *(Color*));

/* -- Timing-related functions ------------------------- */

SRAY_FUNC_I(set_target_fps, SetTargetFPS)

SLOTH2RAYLIB(BeginDrawing)

/* -- Input handling functions ------------------------- */

/* Keyboard */

void raylib_is_key_pressed_(X* x) {
	sloth_push(x, IsKeyPressed(sloth_pop(x)) ? -1 : 0);
}

/* -- Gestures and touch handling functions ------------ */

void raylib_is_gesture_detected_(X* x) {
	sloth_push(x, IsGestureDetected(sloth_pop(x)) ? -1 : 0);
}

/* -- Basic shapes drawing functions ------------------- */

void raylib_draw_rectangle_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	int height = (int)sloth_pop(x);
	int width = (int)sloth_pop(x);
	int pos_y = (int)sloth_pop(x);
	int pos_x = (int)sloth_pop(x);
	DrawRectangle(pos_x, pos_y, width, height, color);
}

void raylib_draw_text_(X* x) {
	char text[255];
	CELL font_size, pos_y, pos_x, l, a, i;
	Color color;
	color.a = (unsigned char)sloth_pop(x);
	color.b = (unsigned char)sloth_pop(x);
	color.g = (unsigned char)sloth_pop(x);
	color.r = (unsigned char)sloth_pop(x);
	font_size = sloth_pop(x);
	pos_y = sloth_pop(x);
	pos_x = sloth_pop(x);
	l = sloth_pop(x);
	a = sloth_pop(x);
	for (i = 0; i < l; i++) text[i] = sloth_cfetch(x, a + i);
	text[l] = 0;
	DrawText(text, pos_x, pos_y, font_size, color);
}

void raylib_end_drawing_(X* x) {
	EndDrawing();
}

void bootstrap_raylib(X* x) {
	/* Create a new wordlist for raylib words and set it */
	/* as current. */

	/* wordlist(x, "RAYLIB"); */
	sloth_evaluate(x, "wordlist dup constant raylib-wordlist set-current");
	sloth_evaluate(x, "get-order 1+ raylib-wordlist swap set-order");

	sloth_code(
		x, 
		"INIT-WINDOW", 
		sloth_primitive(x, &raylib_init_window_));

	SLOTH2RAYLIB_CODE("CLOSE-WINDOW", CloseWindow);
	/*
	sloth_code(
		x,
		"CLOSE-WINDOW",
		sloth_primitive(x, &raylib_close_window_));
	*/

	sloth_code(
		x,
		"SET-TARGET-FPS",
		sloth_primitive(x, &raylib_set_target_fps_));

	sloth_code(
		x,
		"WINDOW-SHOULD-CLOSE",
		sloth_primitive(x, &raylib_window_should_close_));

	SLOTH2RAYLIB_CODE("BEGIN-DRAWING", BeginDrawing);

	SLOTH2RAYLIB_CODE("CLEAR-BACKGROUND", ClearBackground);

	sloth_code(
		x,
		"DRAW-TEXT",
		sloth_primitive(x, &raylib_draw_text_));

	sloth_code(
		x,
		"END-DRAWING",
		sloth_primitive(x, &raylib_end_drawing_));

	sloth_code(x, "IS-KEY-PRESSED", sloth_primitive(x, &raylib_is_key_pressed_));

	sloth_code(x, "IS-GESTURE-DETECTED", sloth_primitive(x, &raylib_is_gesture_detected_));

	sloth_code(x, "DRAW-RECTANGLE", sloth_primitive(x, &raylib_draw_rectangle_));

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
