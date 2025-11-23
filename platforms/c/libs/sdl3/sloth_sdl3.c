#include "sloth_sdl3.h"

#define SLOTH2SDL3_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2SDL3_##f##_));

/* SDL_init.h */
void sloth2SDL3_Init_(X* x) {
	CELL init_flags = sloth_pop(x);
	sloth_push(x, SDL_Init(init_flags) ? -1 : 0);
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
	SDL_SetAppMetadata(appnm_str, appvsn_str, appid_str);
}

/* SDL_events.h */
void sloth2SDL3_Event_type_(X* x) {
	SDL_Event *event = (SDL_Event *)sloth_pop(x);
	sloth_push(x, event->type);
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
		sloth_push(x, -1);
	} else {
		sloth_push(x, 0);
	}
}

void sloth2SDL3_SetRenderLogicalPresentation_(X* x) {
	CELL mode = sloth_pop(x);
	int h = (int)sloth_pop(x);
	int w = (int)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	int r = SDL_SetRenderLogicalPresentation(renderer, w, h, mode);
	sloth_push(x, r == 0 ? 0 : -1);
}

void sloth2SDL3_SetRenderDrawColorFloat_(X* x) {
	FCELL a = sloth_fpop(x);
	FCELL b = sloth_fpop(x);
	FCELL g = sloth_fpop(x);
	FCELL r = sloth_fpop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x, SDL_SetRenderDrawColorFloat(renderer, r, g, b, a));	
}

void sloth2SDL3_RenderClear_(X* x) {
	sloth_push(x, SDL_RenderClear((SDL_Renderer *)sloth_pop(x)));
}

void sloth2SDL3_RenderPresent_(X* x) {
	sloth_push(x, SDL_RenderPresent((SDL_Renderer *)sloth_pop(x)));
}

/* SDL_stdinc.h */
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

	/* SDL_events.h */
	sloth_constant(x, SDL_EVENT_QUIT, "SDL-EVENT-QUIT");

	SLOTH2SDL3_CODE("SDL-Event.type", Event_type);

	/* SDL_timer.h */
	SLOTH2SDL3_CODE("SDL-GetTicks", GetTicks);

	/* SDL_render.h */
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_DISABLED, "SDL-LOGICAL-PRESENTATION-DISABLED");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_STRETCH, "SDL-LOGICAL-PRESENTATION-STRETCH");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_LETTERBOX, "SDL-LOGICAL-PRESENTATION-LETTERBOX");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_OVERSCAN, "SDL-LOGICAL-PRESENTATION-OVERSCAN");
	sloth_constant(x, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE, "SDL-LOGICAL-PRESENTATION-INTEGER-SCALE");

	SLOTH2SDL3_CODE("SDL-CreateWindowAndRenderer", CreateWindowAndRenderer);
	SLOTH2SDL3_CODE("SDL-SetRenderLogicalPresentation", SetRenderLogicalPresentation);
	SLOTH2SDL3_CODE("SDL-SetRenderDrawColorFloat", SetRenderDrawColorFloat);
	SLOTH2SDL3_CODE("SDL-RenderClear", RenderClear);
	SLOTH2SDL3_CODE("SDL-RenderPresent", RenderPresent);

	/* SDL_video.h */
	sloth_constant(x, SDL_WINDOW_FULLSCREEN, "SDL-WINDOW-FULLSCREEN");
	sloth_constant(x, SDL_WINDOW_OPENGL, "SDL-WINDOW-OPENGL");
	sloth_constant(x, SDL_WINDOW_OCCLUDED, "SDL-WINDOW-OCCLUDED");
	sloth_constant(x, SDL_WINDOW_HIDDEN, "SDL-WINDOW-HIDDEN");
	sloth_constant(x, SDL_WINDOW_BORDERLESS, "SDL-WINDOW-BORDERLESS");
	sloth_constant(x, SDL_WINDOW_RESIZABLE, "SDL-WINDOW-RESIZABLE");

	/* SDL_pixels.h */
	sloth_fconstant(x, SDL_ALPHA_OPAQUE_FLOAT, "SDL-ALPHA-OPAQUE-FLOAT");

	/* SDL_stdinc.h */
	sloth_fconstant(x, SDL_PI_D, "SDL-PI-D");

	SLOTH2SDL3_CODE("SDL-sin", sin);
}
