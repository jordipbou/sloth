#include "sloth_sdl3.h"

#define SLOTH2SDL3_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2SDL3_##f##_));

/* SDL_init.h */
void sloth2SDL3_Init_(X* x) {
	CELL init_flags = sloth_pop(x);
	sloth_push(x, 
		SDL_Init(init_flags) 
		? 0 : -256);
}

void sloth2SDL3_SetAppMetadata_(X* x) {
	CELL appid_len = sloth_pop(x);
	char *appid_str = (char*)sloth_pop(x);
	CELL appvsn_len = sloth_pop(x);
	char *appvsn_str = (char*)sloth_pop(x);
	CELL appnm_len = sloth_pop(x);
	char *appnm_str = (char*)sloth_pop(x);

	char appid[160], appvsn[160], appnm[160];
	int i;

	if (appid_str[appid_len] != 0) {
		for (i = 0; i < appid_len; i++) appid[i] = appid_str[i];
		appid[appid_len] = 0;
		appid_str = appid;
	}
	if (appvsn_str[appvsn_len] != 0) {
		for (i = 0; i < appvsn_len; i++) appvsn[i] = appvsn_str[i];
		appvsn[appvsn_len] = 0;
		appvsn_str = appvsn;
	}
	if (appnm_str[appnm_len] != 0) {
		for (i = 0; i < appnm_len; i++) appnm[i] = appnm_str[i];
		appnm_str = appnm;
	}
	sloth_push(x, 
		SDL_SetAppMetadata(appnm_str, appvsn_str, appid_str) 
		? 0 : -256);
}

/* SDL_error.h */
void sloth2SDL3_GetError_(X* x) {
	const char *msg = SDL_GetError();
	sloth_push(x, (CELL)msg);
	sloth_push(x, strlen(msg));
}

/* SDL_events.h */
void sloth2SDL3_Event_type_(X* x) {
	SDL_Event *event = (SDL_Event *)sloth_pop(x);
	sloth_push(x, event->type);
}

void sloth2SDL3_Event_jdevice_which_(X* x) {
	SDL_Event *event = (SDL_Event *)sloth_pop(x);
	sloth_push(x, event->jdevice.which);
}

/* SDL_joystick.h */
void sloth2SDL3_OpenJoystick_(X* x) {
	SDL_JoystickID id = (SDL_JoystickID)sloth_pop(x);
	sloth_push(x, (CELL)SDL_OpenJoystick(id));
}

void sloth2SDL3_GetJoystickName_(X* x) {
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	const char *name = SDL_GetJoystickName(joystick);
	sloth_push(x, (CELL)name);
	sloth_push(x, strlen(name));
}

void sloth2SDL3_GetJoystickID_(X* x) {
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, (CELL)SDL_GetJoystickID(joystick));
}

void sloth2SDL3_GetNumJoystickAxes_(X* x) {
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, SDL_GetNumJoystickAxes(joystick));
}

void sloth2SDL3_GetNumJoystickHats_(X* x) {
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, SDL_GetNumJoystickHats(joystick));
}

void sloth2SDL3_GetNumJoystickButtons_(X* x) {
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, SDL_GetNumJoystickButtons(joystick));
}

void sloth2SDL3_GetJoystickAxis_(X* x) {
	int axis = (int)sloth_pop(x);
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, SDL_GetJoystickAxis(joystick, axis));
}

void sloth2SDL3_GetJoystickHat_(X* x) {
	int hat = (int)sloth_pop(x);
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, SDL_GetJoystickHat(joystick, hat));
}

void sloth2SDL3_GetJoystickButton_(X* x) {
	int axis = (int)sloth_pop(x);
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	sloth_push(x, SDL_GetJoystickButton(joystick, axis));
}

void sloth2SDL3_CloseJoystick_(X* x) {
	SDL_Joystick *joystick = (SDL_Joystick *)sloth_pop(x);
	SDL_CloseJoystick(joystick);
}

/* SDL_timer.h */
void sloth2SDL3_GetTicks_(X* x) {
	sloth_push(x, SDL_GetTicks());
}

/* SDL_render.h */
void sloth2SDL3_CreateWindowAndRenderer_(X* x) {
	int res;
	SDL_WindowFlags window_flags = (SDL_WindowFlags)sloth_pop(x);
	CELL height = sloth_pop(x);
	CELL width = sloth_pop(x);
	CELL title_len = sloth_pop(x);
	char *title_str = (char *)sloth_pop(x);
	char title[80];
	SDL_Window *window;
	SDL_Renderer *renderer;
	if (title_str[title_len] != 0) {
		int i;
		for (i = 0; i < title_len; i++) {
			title[i] = title_str[i];
		}
		title[title_len] = 0;
		title_str = title;
	}
	if (SDL_CreateWindowAndRenderer(title_str, width, height, window_flags, &window, &renderer)) {
		sloth_push(x, (CELL)window);
		sloth_push(x, (CELL)renderer);
		sloth_push(x, 0);
	} else {
		sloth_push(x, -256);
	}
}

void sloth2SDL3_SetRenderLogicalPresentation_(X* x) {
	CELL mode = sloth_pop(x);
	int h = (int)sloth_pop(x);
	int w = (int)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x, 
		SDL_SetRenderLogicalPresentation(renderer, w, h, mode) 
		? 0 : -256);
}

void sloth2SDL3_SetRenderDrawColor_(X* x) {
	Uint8 a = (Uint8)sloth_pop(x);
	Uint8 b = (Uint8)sloth_pop(x);
	Uint8 g = (Uint8)sloth_pop(x);
	Uint8 r = (Uint8)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x, 
		SDL_SetRenderDrawColor(renderer, r, g, b, a)
		? 0 : -256);	
}

void sloth2SDL3_SetRenderDrawColorFloat_(X* x) {
	FCELL a = sloth_fpop(x);
	FCELL b = sloth_fpop(x);
	FCELL g = sloth_fpop(x);
	FCELL r = sloth_fpop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x, 
		SDL_SetRenderDrawColorFloat(renderer, r, g, b, a)
		? 0 : -256);	
}

void sloth2SDL3_RenderClear_(X* x) {
	sloth_push(x, 
		SDL_RenderClear((SDL_Renderer *)sloth_pop(x))
		? 0 : -256);
}

void sloth2SDL3_RenderRect_(X* x) {
	SDL_FRect *rect = (SDL_FRect *)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x,
		SDL_RenderRect(renderer, rect)
		? 0 : -256);
}

void sloth2SDL3_RenderFillRect_(X* x) {
	SDL_FRect *rect = (SDL_FRect *)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x,
		SDL_RenderFillRect(renderer, rect)
		? 0 : -256);
}

void sloth2SDL3_RenderFillRects_(X* x) {
	int count = (int)sloth_pop(x);
	SDL_FRect *rects = (SDL_FRect *)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x,
		SDL_RenderFillRects(renderer, rects, count)
		? 0 : -256);
}

void sloth2SDL3_RenderPresent_(X* x) {
	sloth_push(x, 
		SDL_RenderPresent((SDL_Renderer *)sloth_pop(x))
		? 0 : -256);
}

void sloth2SDL3_RenderDebugText_(X* x) {
	CELL str_len = sloth_pop(x);
	char *str = (char *)sloth_pop(x);
	float fy = sloth_fpop(x);
	float fx = sloth_fpop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	char text[255];
	if (str[str_len] != 0) {
		int i;
		for (i = 0; i < str_len; i++) text[i] = str[i];
		text[str_len] = 0;
		str = text;
	}
	sloth_push(x,
		SDL_RenderDebugText(renderer, fx, fy, str)
		? 0 : -256);
}

/* SDL_video.h */
void sloth2SDL3_GetWindowSize_(X* x) {
	int w, h;
	SDL_Window *window = (SDL_Window *)sloth_pop(x);
	if (SDL_GetWindowSize(window, &w, &h)) {
		sloth_push(x, w);
		sloth_push(x, h);
		sloth_push(x, 0);
	} else {
		sloth_push(x, -256);
	}
}

/* SDL_stdinc.h */
void sloth2SDL3_rand_(X* x) {
	Sint32 n = (Sint32)sloth_pop(x);
	sloth_push(x, SDL_rand(n));
}

void sloth2SDL3_fabsf_(X* x) {
	float mag = (float)sloth_fpop(x);
	sloth_fpush(x, SDL_fabsf(mag));
}

void sloth2SDL3_sin_(X* x) {
	FCELL rad = sloth_fpop(x);
	sloth_fpush(x, SDL_sin(rad));
}

/* ----------------------------------------------------- */

void sloth_constant(X* x, CELL v, char* n) {
	char s[] = "CONSTANT ..........::::::::::..........::::::::::";
	int i;
	sloth_push(x, v);
	for (i = 0; i < strlen(n); i++) {
		s[9+i] = n[i];
	}
	s[9+strlen(n)] = 0;
	sloth_evaluate(x, s);
}

void sloth_fconstant(X* x, FCELL v, char* n) {
	char s[] = "FCONSTANT ..........::::::::::..........::::::::::";
	int i;
	sloth_fpush(x, v);
	for (i = 0; i < strlen(n); i++) {
		s[10+i] = n[i];
	}
	s[10+strlen(n)] = 0;
	sloth_evaluate(x, s);
}

void sloth_bootstrap_SDL3(X* x) {
	/* SDL_init.h */

	sloth_constant(x, SDL_INIT_AUDIO, "SDL-INIT-AUDIO");
	sloth_constant(x, SDL_INIT_VIDEO, "SDL-INIT-VIDEO");
	sloth_constant(x, SDL_INIT_JOYSTICK, "SDL-INIT-JOYSTICK");
	sloth_constant(x, SDL_INIT_HAPTIC, "SDL-INIT-HAPTIC");
	sloth_constant(x, SDL_INIT_GAMEPAD, "SDL-INIT-GAMEPAD");
	sloth_constant(x, SDL_INIT_EVENTS, "SDL-INIT-EVENTS");
	sloth_constant(x, SDL_INIT_SENSOR, "SDL-INIT-SENSOR");
	sloth_constant(x, SDL_INIT_CAMERA, "SDL-INIT-CAMERA");

	SLOTH2SDL3_CODE("SDL-Init", Init);
	SLOTH2SDL3_CODE("SDL-SetAppMetadata", SetAppMetadata);

	/* SDL_error.h */
	SLOTH2SDL3_CODE("SDL-GetError", GetError);

	/* SDL_events.h */
	sloth_constant(x, SDL_EVENT_QUIT, "SDL-EVENT-QUIT");
	sloth_constant(x, SDL_EVENT_JOYSTICK_ADDED, "SDL-EVENT-JOYSTICK-ADDED");
	sloth_constant(x, SDL_EVENT_JOYSTICK_REMOVED, "SDL-EVENT-JOYSTICK-REMOVED");

	SLOTH2SDL3_CODE("SDL-Event.type", Event_type);
	SLOTH2SDL3_CODE("SDL-Event.jdevice.which", Event_jdevice_which);

	/* SDL_joystick.h */
	sloth_constant(x, SDL_HAT_CENTERED, "SDL-HAT-CENTERED");
	sloth_constant(x, SDL_HAT_UP, "SDL-HAT-UP");
	sloth_constant(x, SDL_HAT_RIGHT, "SDL-HAT-RIGHT");
	sloth_constant(x, SDL_HAT_DOWN, "SDL-HAT-DOWN");
	sloth_constant(x, SDL_HAT_LEFT, "SDL-HAT-LEFT");
	sloth_constant(x, SDL_HAT_RIGHTUP, "SDL-HAT-RIGHTUP");
	sloth_constant(x, SDL_HAT_RIGHTDOWN, "SDL-HAT-RIGHTDOWN");
	sloth_constant(x, SDL_HAT_LEFTUP, "SDL-HAT-LEFTUP");
	sloth_constant(x, SDL_HAT_LEFTDOWN, "SDL-HAT-LEFTDOWN");

	SLOTH2SDL3_CODE("SDL-OpenJoystick", OpenJoystick);
	SLOTH2SDL3_CODE("SDL-GetJoystickName", GetJoystickName);
	SLOTH2SDL3_CODE("SDL-GetJoystickID", GetJoystickID);
	SLOTH2SDL3_CODE("SDL-GetNumJoystickAxes", GetNumJoystickAxes);
	SLOTH2SDL3_CODE("SDL-GetNumJoystickHats", GetNumJoystickHats);
	SLOTH2SDL3_CODE("SDL-GetNumJoystickButtons", GetNumJoystickButtons);
	SLOTH2SDL3_CODE("SDL-GetJoystickAxis", GetJoystickAxis);
	SLOTH2SDL3_CODE("SDL-GetJoystickHat", GetJoystickHat);
	SLOTH2SDL3_CODE("SDL-GetJoystickButton", GetJoystickButton);
	SLOTH2SDL3_CODE("SDL-CloseJoystick", CloseJoystick);

	/* SDL_timer.h */
	SLOTH2SDL3_CODE("SDL-GetTicks", GetTicks);

	/* SDL_render.h */
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_DISABLED, "SDL-LOGICAL-PRESENTATION-DISABLED");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_STRETCH, "SDL-LOGICAL-PRESENTATION-STRETCH");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_LETTERBOX, "SDL-LOGICAL-PRESENTATION-LETTERBOX");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_OVERSCAN, "SDL-LOGICAL-PRESENTATION-OVERSCAN");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE, "SDL-LOGICAL-PRESENTATION-INTEGER-SCALE");

	sloth_constant(x, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE, "SDL-DEBUG-TEXT-FONT-CHARACTER-SIZE");

	SLOTH2SDL3_CODE("SDL-CreateWindowAndRenderer", CreateWindowAndRenderer);
	SLOTH2SDL3_CODE("SDL-SetRenderLogicalPresentation", SetRenderLogicalPresentation);
	SLOTH2SDL3_CODE("SDL-SetRenderDrawColor", SetRenderDrawColor);
	SLOTH2SDL3_CODE("SDL-SetRenderDrawColorFloat", SetRenderDrawColorFloat);
	SLOTH2SDL3_CODE("SDL-RenderClear", RenderClear);
	SLOTH2SDL3_CODE("SDL-RenderRect", RenderRect);
	SLOTH2SDL3_CODE("SDL-RenderFillRect", RenderFillRect);
	SLOTH2SDL3_CODE("SDL-RenderFillRects", RenderFillRects);
	SLOTH2SDL3_CODE("SDL-RenderPresent", RenderPresent);
	SLOTH2SDL3_CODE("SDL-RenderDebugText", RenderDebugText);

	/* SDL_video.h */
	sloth_constant(x, SDL_WINDOW_FULLSCREEN, "SDL-WINDOW-FULLSCREEN");
	sloth_constant(x, SDL_WINDOW_OPENGL, "SDL-WINDOW-OPENGL");
	sloth_constant(x, SDL_WINDOW_OCCLUDED, "SDL-WINDOW-OCCLUDED");
	sloth_constant(x, SDL_WINDOW_HIDDEN, "SDL-WINDOW-HIDDEN");
	sloth_constant(x, SDL_WINDOW_BORDERLESS, "SDL-WINDOW-BORDERLESS");
	sloth_constant(x, SDL_WINDOW_RESIZABLE, "SDL-WINDOW-RESIZABLE");

	SLOTH2SDL3_CODE("SDL-GetWindowSize", GetWindowSize);

	/* SDL_pixels.h */
	sloth_evaluate(x,
		"BEGIN-STRUCTURE SDL-Color "
		"  CFIELD: SDL-Color.r "
		"  CFIELD: SDL-Color.g "
		"  CFIELD: SDL-Color.b "
		"  CFIELD: SDL-Color.a "
		"END-STRUCTURE");

	sloth_fconstant(x, SDL_ALPHA_OPAQUE_FLOAT, "SDL-ALPHA-OPAQUE-FLOAT");

	/* SDL_rect.h */
	sloth_evaluate(x,
		"BEGIN-STRUCTURE SDL-FRect "
		"  SFFIELD: SDL-FRect.x "
		"  SFFIELD: SDL-FRect.y "
		"  SFFIELD: SDL-FRect.w "
		"  SFFIELD: SDL-FRect.h "
		"END-STRUCTURE");

	/* SDL_stdinc.h */
	sloth_fconstant(x, SDL_PI_D, "SDL-PI-D");

	SLOTH2SDL3_CODE("SDL-rand", rand);
	SLOTH2SDL3_CODE("SDL-fabsf", fabsf);
	SLOTH2SDL3_CODE("SDL-sin", sin);
}
