#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"raylib.h"
#include"raymath.h"

#define SLOTH2RAYLIB(f) void sloth2raylib_##f##_(X* x) { f(); }
#define SLOTH2RAYLIB_1ARG(f, t) void sloth2raylib_##f##_(X* x) { f(t##sloth_pop(x)); }

#define SLOTH2RAYLIB_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2raylib_##f##_));

/* == MODULE RCORE ===================================== */

/* -- Window-related functions ------------------------- */

void sloth2raylib_InitWindow_(X* x) {
	char title[255];
	int l = (int)sloth_pop(x);
	char *a = (char*)sloth_pop(x);
	CELL height = sloth_pop(x);
	CELL width = sloth_pop(x);

	/* Copy the string only if it's not a zero ended string */
	if (a[l] != 0) {
		int i;
		for (i = 0; i < l; i++) title[i] = sloth_cfetch(x, a + i);
		title[l] = 0;
		InitWindow(width, height, title);
	} else {
		InitWindow(width, height, a);
	}
}

void sloth2raylib_CloseWindow_(X* x) {
	CloseWindow();
}

void sloth2raylib_WindowShouldClose_(X* x) {
	sloth_push(x, WindowShouldClose());
}

void sloth2raylib_GetScreenWidth_(X* x) {
	sloth_push(x, GetScreenWidth());
}

void sloth2raylib_GetScreenHeight_(X* x) {
	sloth_push(x, GetScreenHeight());
}

/* -- Drawing-related functions ------------------------ */

void sloth2raylib_ClearBackground_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	ClearBackground(color);
}

void sloth2raylib_EndDrawing_(X* x) {
	EndDrawing();
}

void sloth2raylib_BeginMode2D_(X* x) {
	BeginMode2D(*((Camera2D*)sloth_pop(x)));
}

void sloth2raylib_EndMode2D_(X* x) {
	EndMode2D();
}

/* -- Screen-space-related functions ------------------- */

void sloth2raylib_GetScreenToWorld2D_(X* x) {
	Vector2 *dest = (Vector2*)sloth_pop(x);
	Camera2D camera = *((Camera2D*)sloth_pop(x));
	Vector2 position = *((Vector2*)sloth_pop(x));
	Vector2 res = GetScreenToWorld2D(position, camera);
	memcpy(dest, &res, sizeof(Vector2));
}

/* -- Timing-related functions ------------------------- */

void sloth2raylib_SetTargetFPS_(X* x) {
	SetTargetFPS((int)sloth_pop(x));
}

void sloth2raylib_BeginDrawing_(X* x) {
	BeginDrawing();
}

/* -- Random values generation functions --------------- */

void sloth2raylib_GetRandomValue_(X* x) {
	CELL b = sloth_pop(x);
	sloth_push(x, GetRandomValue(sloth_pop(x), b));
}

/* -- Input handling functions ------------------------- */

/* Keyboard */

void sloth2raylib_IsKeyPressed_(X* x) {
	sloth_push(x, IsKeyPressed(sloth_pop(x)) ? -1 : 0);
}

void sloth2raylib_IsKeyDown_(X* x) {
	sloth_push(x, IsKeyDown(sloth_pop(x)) ? -1 : 0);
}

/* Mouse */

void sloth2raylib_IsMouseButtonPressed_(X* x) {
	sloth_push(x, IsMouseButtonPressed(sloth_pop(x)) ? -1 : 0);
}

void sloth2raylib_IsMouseButtonDown_(X* x) {
	sloth_push(x, IsMouseButtonDown(sloth_pop(x)) ? -1 : 0);
}

void sloth2raylib_GetMouseX_(X* x) {
	sloth_push(x, GetMouseX());
}

void sloth2raylib_GetMouseY_(X* x) {
	sloth_push(x, GetMouseY());
}

void sloth2raylib_GetMousePosition_(X* x) {
	Vector2 *dest = (Vector2*)sloth_pop(x);
	Vector2 position = GetMousePosition(x);
	memcpy(dest, &position, sizeof(Vector2));
}

void sloth2raylib_GetMouseDelta_(X* x) {
	Vector2 *dest = (Vector2*)sloth_pop(x);
	Vector2 delta = GetMouseDelta();
	memcpy(dest, &delta, sizeof(Vector2));
}

void sloth2raylib_GetMouseWheelMove_(X* x) {
	sloth_fpush(x, GetMouseWheelMove());	
}

/* -- Gestures and touch handling functions ------------ */

void raylib_is_gesture_detected_(X* x) {
	sloth_push(x, IsGestureDetected(sloth_pop(x)) ? -1 : 0);
}

/* == MODULE RTEXTURES ================================= */

/* -- Color/pixel related functions -------------------- */

void sloth2raylib_Fade_(X* x) {
	Color *dest = (Color*)sloth_pop(x);	
	SFLOAT alpha = (SFLOAT)sloth_fpop(x);
	Color color = *((Color*)sloth_pop(x));
	Color faded_color = Fade(color, alpha);
	memcpy(dest, &faded_color, sizeof(Color));
}

/* == MODULE RSHAPES =================================== */

/* -- Basic shapes drawing functions ------------------- */

void sloth2raylib_DrawLine_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	int endPosY = (int)sloth_pop(x);
	int endPosX = (int)sloth_pop(x);
	int startPosY = (int)sloth_pop(x);
	int startPosX = (int)sloth_pop(x);
	DrawLine(startPosX, startPosY, endPosX, endPosY, color);
}

void sloth2raylib_DrawCircle_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	float radius = (float)sloth_fpop(x);
	int centerY = (int)sloth_pop(x);
	int centerX = (int)sloth_pop(x);
	DrawCircle(centerX, centerY, radius, color);
}

void sloth2raylib_DrawCircleV_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	float radius = (float)sloth_fpop(x);
	Vector2 center = *((Vector2*)sloth_pop(x));
	DrawCircleV(center, radius, color);
}

void sloth2raylib_DrawRectangle_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	int height = (int)sloth_pop(x);
	int width = (int)sloth_pop(x);
	int pos_y = (int)sloth_pop(x);
	int pos_x = (int)sloth_pop(x);
	DrawRectangle(pos_x, pos_y, width, height, color);
}

void sloth2raylib_DrawRectangleRec_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	Rectangle rectangle = *((Rectangle*)sloth_pop(x));
	DrawRectangleRec(rectangle, color);
}

void sloth2raylib_DrawRectangleLines_(X* x) {
	Color color = *((Color*)sloth_pop(x));
	int height = (int)sloth_pop(x);
	int width = (int)sloth_pop(x);
	int posY = (int)sloth_pop(x);
	int posX = (int)sloth_pop(x);
	DrawRectangleLines(posX, posY, width, height, color);
}

/* == MODULE RTEXT ===================================== */

/* -- Font loading/unloading functions ----------------- */

void sloth2raylib_GetFontDefault_(X* x) {
	Font *dest = (Font*)sloth_pop(x);
	Font font = GetFontDefault();
	memcpy(dest, &font, sizeof(Font));
}

/* -- Text drawing functions --------------------------- */

void sloth2raylib_DrawText_(X* x) {
	char text[255];
	Color* color = (Color*)sloth_pop(x);
	int font_size = (int)sloth_pop(x);
	int pos_y = (int)sloth_pop(x);
	int pos_x = (int)sloth_pop(x);
	int l = (int)sloth_pop(x);
	char *a = (char*)sloth_pop(x);
	if (a[l] != 0) {
		int i;
		for (i = 0; i < l; i++) text[i] = sloth_cfetch(x, a + i);
		text[l] = 0;
		DrawText(text, pos_x, pos_y, font_size, *color);
	} else {
		DrawText(a, pos_x, pos_y, font_size, *color);
	}
}

/* -- Text strings management functions ---------------- */

void sloth2raylib_TextFormat_(X* x) {
	 /* TODO I will need to pass number of parameters, */
	 /* I suppose. */
}

/* == MODULE RMODELS =================================== */

/* -- Basic geometric 3d shapes drawing functions ------ */

void sloth2raylib_DrawGrid_(X* x) {
	float spacing = (float)sloth_fpop(x);
	int slices = (int)sloth_pop(x);
	DrawGrid(slices, spacing);
}

/* == MODULE RAYMATH =================================== */

/* -- Utils math --------------------------------------- */

void sloth2raylib_Clamp_(X* x) {
	float max = (float)sloth_fpop(x);
	float min = (float)sloth_fpop(x);
	float value = (float)sloth_fpop(x);
	sloth_fpush(x, Clamp(value, min, max));
}

/* -- Vector2 math ------------------------------------- */

void sloth2raylib_Vector2Add_(X* x) {
	Vector2 *dest = (Vector2*)sloth_pop(x);
	Vector2 v2 = *((Vector2*)sloth_pop(x));
	Vector2 v1 = *((Vector2*)sloth_pop(x));
	Vector2 res = Vector2Add(v1, v2);
	memcpy(dest, &res, sizeof(Vector2));
}

void sloth2raylib_Vector2Scale_(X* x) {
	Vector2 *dest = (Vector2*)sloth_pop(x);
	float scale = (float)sloth_fpop(x);
	Vector2 v = *((Vector2*)sloth_pop(x));
	Vector2 res = Vector2Scale(v, scale);
	memcpy(dest, &res, sizeof(Vector2));
}

/* == MODULE RLGL ====================================== */

/* -- Matrix operations -------------------------------- */

void sloth2raylib_rlPushMatrix_(X* x) {
	rlPushMatrix();
}

void sloth2raylib_rlPopMatrix_(X* x) {
	rlPopMatrix();
}

void sloth2raylib_rlTranslate_(X* x) {
	float dz = (float)sloth_fpop(x);
	float dy = (float)sloth_fpop(x);
	float dx = (float)sloth_fpop(x);
	rlTranslatef(dx, dy, dz);
}

void sloth2raylib_rlRotatef_(X* x) {
	float dz = (float)sloth_fpop(x);
	float dy = (float)sloth_fpop(x);
	float dx = (float)sloth_fpop(x);
	float angle = (float)sloth_fpop(x);
	rlRotatef(angle, dx, dy, dz);
}

/* == BOOTSTRAPPING ==================================== */

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
	SLOTH2RAYLIB_CODE("WINDOW-SHOULD-CLOSE", WindowShouldClose);
	SLOTH2RAYLIB_CODE("GET-SCREEN-WIDTH", GetScreenWidth);
	SLOTH2RAYLIB_CODE("GET-SCREEN-HEIGHT", GetScreenHeight);

	/* -- Drawing-related functions ------------------------ */

	SLOTH2RAYLIB_CODE("CLEAR-BACKGROUND", ClearBackground);
	SLOTH2RAYLIB_CODE("BEGIN-DRAWING", BeginDrawing);
	SLOTH2RAYLIB_CODE("END-DRAWING", EndDrawing);
	SLOTH2RAYLIB_CODE("BEGIN-MODE-2D", BeginMode2D);
	SLOTH2RAYLIB_CODE("END-MODE-2D", EndMode2D);

	/* -- Screen-space-related functions ------------------- */

	SLOTH2RAYLIB_CODE("GET-SCREEN-TO-WORLD-2D", GetScreenToWorld2D);

	/* -- Timing-related functions ------------------------- */

	SLOTH2RAYLIB_CODE("SET-TARGET-FPS", SetTargetFPS);

	/* == Input handling functions ========================= */

	/* -- Keyboard -- */

	SLOTH2RAYLIB_CODE("IS-KEY-PRESSED", IsKeyPressed);
	SLOTH2RAYLIB_CODE("IS-KEY-DOWN", IsKeyDown);

	sloth_code(x, "IS-GESTURE-DETECTED", sloth_primitive(x, &raylib_is_gesture_detected_));

	/* -- Random values generation functions --------------- */

	SLOTH2RAYLIB_CODE("GET-RANDOM-VALUE", GetRandomValue);

	/* -- Input handling functions ------------------------- */
	
	/* Keyboard */
	
	/* Mouse */

	SLOTH2RAYLIB_CODE("IS-MOUSE-BUTTON-PRESSED", IsMouseButtonPressed);
	SLOTH2RAYLIB_CODE("IS-MOUSE-BUTTON-DOWN", IsMouseButtonDown);
	SLOTH2RAYLIB_CODE("GET-MOUSE-X", GetMouseX);
	SLOTH2RAYLIB_CODE("GET-MOUSE-Y", GetMouseY);
	SLOTH2RAYLIB_CODE("GET-MOUSE-POSITION", GetMousePosition);
	SLOTH2RAYLIB_CODE("GET-MOUSE-DELTA", GetMouseDelta);
	SLOTH2RAYLIB_CODE("GET-MOUSE-WHEEL-MOVE", GetMouseWheelMove);

	/* == MODULE RTEXTURES =============================== */

	/* -- Color/pixel related functions ------------------ */

	SLOTH2RAYLIB_CODE("FADE", Fade);

	/* == MODULE RSHAPES ================================= */

	/* -- Basic shapes drawing functions ----------------- */

	SLOTH2RAYLIB_CODE("DRAW-LINE", DrawLine);
	SLOTH2RAYLIB_CODE("DRAW-CIRCLE", DrawCircle);
	SLOTH2RAYLIB_CODE("DRAW-CIRCLE-V", DrawCircleV);
	SLOTH2RAYLIB_CODE("DRAW-RECTANGLE", DrawRectangle);
	SLOTH2RAYLIB_CODE("DRAW-RECTANGLE-REC", DrawRectangleRec);
	SLOTH2RAYLIB_CODE("DRAW-RECTANGLE-LINES", DrawRectangleLines);

	/* == MODULE RTEXT =================================== */

	/* -- Font loading/unloading functions --------------- */

	SLOTH2RAYLIB_CODE("GET-FONT-DEFAULT", GetFontDefault);

	/* -- Text drawing functions ------------------------- */

	SLOTH2RAYLIB_CODE("DRAW-TEXT", DrawText);

	/* == MODULE RMODELS ================================= */
	
	/* -- Basic geometric 3d shapes drawing functions ---- */

	SLOTH2RAYLIB_CODE("DRAW-GRID", DrawGrid);

	/* == MODULE RAYMATH ================================= */
	
	/* -- Utils math ------------------------------------- */

	SLOTH2RAYLIB_CODE("CLAMP", Clamp);
	
	/* -- Vector2 math ----------------------------------- */

	SLOTH2RAYLIB_CODE("VECTOR2ADD", Vector2Add);
	SLOTH2RAYLIB_CODE("VECTOR2SCALE", Vector2Scale);

	/* == MODULE RLGL ==================================== */

	/* -- Matrix operations ------------------------------ */

	SLOTH2RAYLIB_CODE("RL-PUSH-MATRIX", rlPushMatrix);
	SLOTH2RAYLIB_CODE("RL-POP-MATRIX", rlPopMatrix);
	SLOTH2RAYLIB_CODE("RL-TRANSLATE", rlTranslate);
	SLOTH2RAYLIB_CODE("RL-ROTATEF", rlRotatef);

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
