#include"fsloth.h"

#define SLOTH2RAYLIB_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2raylib_##f##_));

/* == MODULE RCORE ===================================== */

/* -- Window-related functions ------------------------- */

void sloth2raylib_InitWindow_(X* x);
void sloth2raylib_CloseWindow_(X* x);
void sloth2raylib_WindowShouldClose_(X* x);
void sloth2raylib_IsWindowReady_(X* x);
void sloth2raylib_SetWindowState_(X* x);
void sloth2raylib_ToggleFullscreen_(X* x);
void sloth2raylib_SetWindowSize_(X* x);
void sloth2raylib_GetScreenWidth_(X* x);
void sloth2raylib_GetScreenHeight_(X* x);

/* -- Drawing-related functions ------------------------ */

void sloth2raylib_ClearBackground_(X* x);
void sloth2raylib_EndDrawing_(X* x);
void sloth2raylib_BeginMode2D_(X* x);
void sloth2raylib_EndMode2D_(X* x);

/* -- Screen-space-related functions ------------------- */

void sloth2raylib_GetScreenToWorld2D_(X* x);

/* -- Timing-related functions ------------------------- */

void sloth2raylib_SetTargetFPS_(X* x);

/* -- Misc. functions ---------------------------------- */

void sloth2raylib_SetConfigFlags_(X* x);
void sloth2raylib_BeginDrawing_(X* x);

/* -- Random values generation functions --------------- */

void sloth2raylib_GetRandomValue_(X* x);

/* -- Input handling functions ------------------------- */

/* Keyboard */

void sloth2raylib_IsKeyPressed_(X* x);
void sloth2raylib_IsKeyDown_(X* x);
void sloth2raylib_GetCharPressed_(X* x);

/* Mouse */

void sloth2raylib_IsMouseButtonPressed_(X* x);
void sloth2raylib_IsMouseButtonDown_(X* x);
void sloth2raylib_GetMouseX_(X* x);
void sloth2raylib_GetMouseY_(X* x);
void sloth2raylib_GetMousePosition_(X* x);
void sloth2raylib_GetMouseDelta_(X* x);
void sloth2raylib_GetMouseWheelMove_(X* x);

/* -- Gestures and touch handling functions ------------ */

void raylib_is_gesture_detected_(X* x);

/* == MODULE RTEXTURES ================================= */

/* -- Color/pixel related functions -------------------- */

void sloth2raylib_Fade_(X* x);

/* == MODULE RSHAPES =================================== */

/* -- Basic shapes drawing functions ------------------- */

void sloth2raylib_DrawLine_(X* x);
void sloth2raylib_DrawCircle_(X* x);
void sloth2raylib_DrawCircleV_(X* x);
void sloth2raylib_DrawRectangle_(X* x);
void sloth2raylib_DrawRectangleRec_(X* x);
void sloth2raylib_DrawRectangleLines_(X* x);

/* == MODULE RTEXT ===================================== */

/* -- Font loading/unloading functions ----------------- */

void sloth2raylib_GetFontDefault_(X* x);
void sloth2raylib_LoadFont_(X* x);
void sloth2raylib_LoadFontEx_(X* x);

/* -- Text drawing functions --------------------------- */

void sloth2raylib_DrawText_(X* x);
void sloth2raylib_DrawTextEx_(X* x);

/* -- Text strings management functions ---------------- */

void sloth2raylib_TextFormat2_(X* x);

/* == MODULE RMODELS =================================== */

/* -- Basic geometric 3d shapes drawing functions ------ */

void sloth2raylib_DrawGrid_(X* x);

/* == MODULE RAYMATH =================================== */

/* -- Utils math --------------------------------------- */

void sloth2raylib_Clamp_(X* x);

/* -- Vector2 math ------------------------------------- */

void sloth2raylib_Vector2Add_(X* x);
void sloth2raylib_Vector2Scale_(X* x);

/* == MODULE RLGL ====================================== */

/* -- Matrix operations -------------------------------- */

void sloth2raylib_rlPushMatrix_(X* x);
void sloth2raylib_rlPopMatrix_(X* x);
void sloth2raylib_rlTranslatef_(X* x);
void sloth2raylib_rlRotatef_(X* x);

/* == BOOTSTRAPPING ==================================== */

void sloth_bootstrap_raylib(X* x);

