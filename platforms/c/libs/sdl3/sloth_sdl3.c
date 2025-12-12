#include "sloth_sdl3.h"

#define SLOTH2SDL3_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2SDL3_##f##_));

#define STR_BUF_LEN 255

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
void sloth2SDL3_Event_type_(X* x) { /* NOOP */ }
void sloth2SDL3_Event_event_(X* x) { 
	sloth_push(x, sizeof(Uint32) + sloth_pop(x));
}

void sloth2SDL3_JoyDeviceEvent_timestamp_(X* x) {
	sloth_push(x, 
		sizeof(SDL_EventType) + sizeof(Uint32) + sloth_pop(x));
}

void sloth2SDL3_JoyDeviceEvent_which_(X* x) {
	sloth_push(x, 
		sizeof(SDL_EventType) + sizeof(Uint32) + sizeof(Uint64) 
		+ sloth_pop(x));
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

/* SDL_audio.h */
void sloth2SDL3_PutAudioStreamData_(X* x) {
	int len = (int)sloth_pop(x);
	const void *buf = (void *)sloth_pop(x);
	SDL_AudioStream *stream = (SDL_AudioStream *)sloth_pop(x);
	sloth_push(x,
		SDL_PutAudioStreamData(stream, buf, len)
		? 0 : -256);
}

void sloth2SDL3_GetAudioStreamQueued_(X* x) {
	SDL_AudioStream *stream = (SDL_AudioStream *)sloth_pop(x);
	int nbytes = SDL_GetAudioStreamQueued(stream);
	if (nbytes == -1) {
		sloth_push(x, -256);
	} else {
		sloth_push(x, nbytes);
		sloth_push(x, 0);
	}
}

void sloth2SDL3_ResumeAudioStreamDevice_(X* x) {
	SDL_AudioStream *stream = (SDL_AudioStream *)sloth_pop(x);
	sloth_push(x,
		SDL_ResumeAudioStreamDevice(stream)
		? 0 : -256);
}

void sloth2SDL3_OpenAudioDeviceStream_(X* x) {
	void *userdata = (void *)sloth_pop(x);
	SDL_AudioStreamCallback callback = (SDL_AudioStreamCallback)sloth_pop(x);
	SDL_AudioSpec *spec = (SDL_AudioSpec *)sloth_pop(x);
	SDL_AudioDeviceID devid = (SDL_AudioDeviceID)sloth_pop(x);
	SDL_AudioStream *stream;
	stream = SDL_OpenAudioDeviceStream(devid, spec, callback, userdata);
	if (stream) {
		sloth_push(x, (CELL)stream);
		sloth_push(x, 0);
	} else {
		sloth_push(x, -256);
	}
}

void sloth2SDL3_LoadWAV_(X* x) {
	SDL_AudioSpec *spec = (SDL_AudioSpec *)sloth_pop(x);
	CELL path_len = sloth_pop(x);
	char *path = (char *)sloth_pop(x);
	char path_tmp[STR_BUF_LEN];
	Uint8 *audio_buf;
	Uint32 audio_len;
	if (path[path_len] != 0) {
		int i;
		for (i = 0; i < path_len; i++) path_tmp[i] = path[i];
		path_tmp[path_len] = 0;
		path = path_tmp;
	}
	if (SDL_LoadWAV(path, spec, &audio_buf, &audio_len)) {
		sloth_push(x, (CELL)audio_buf);
		sloth_push(x, audio_len);
		sloth_push(x, 0);
	} else {
		sloth_push(x, -256);
	}
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

void sloth2SDL3_SetRenderScale_(X* x) {
	float scaleY = (float)sloth_fpop(x);
	float scaleX = (float)sloth_fpop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x,
		SDL_SetRenderScale(renderer, scaleY, scaleX)
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

void sloth2SDL3_RenderPoints_(X* x) {
	int count = (int)sloth_pop(x);
	SDL_FPoint *points = (SDL_FPoint *)sloth_pop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x,
		SDL_RenderPoints(renderer, points, count)
		? 0 : -256);
}

void sloth2SDL3_RenderLine_(X* x) {
	float y2 = (float)sloth_fpop(x);
	float x2 = (float)sloth_fpop(x);
	float y1 = (float)sloth_fpop(x);
	float x1 = (float)sloth_fpop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	sloth_push(x,
		SDL_RenderLine(renderer, x1, y1, x2, y2)
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
	char text[STR_BUF_LEN];
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

void sloth2SDL3_RenderDebugTextFormati_(X* x) {
	CELL arg1 = sloth_pop(x);	
	CELL fmt_len = sloth_pop(x);
	char *fmt_str = (char *)sloth_pop(x);
	float dy = (float)sloth_fpop(x);
	float dx = (float)sloth_fpop(x);
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_pop(x);
	char fmt[STR_BUF_LEN];
	if (fmt_str[fmt_len] != 0) {
		int i;
		for (i = 0; i < fmt_len; i++) fmt[i] = fmt_str[i];
		fmt[fmt_len] = 0;
		fmt_str = fmt;
	}
	sloth_push(x,
		SDL_RenderDebugTextFormat(renderer, dx, dy, fmt_str, arg1)
		? 0 : -256);
}

/* SDL_filesystem.h */
void sloth2SDL3_GetBasePath_(X* x) {
	const char *base_path = SDL_GetBasePath();
	if (base_path == 0) {
		sloth_push(x, -256);
	} else {
		sloth_push(x, (CELL)base_path);
		sloth_push(x, strlen(base_path));
		sloth_push(x, 0);
	}
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

/* SDL_tray.h */
void sloth2SDL3_CreateTray_(X* x) {
	CELL tooltip_len = sloth_pop(x);
	char *tooltip_str = (char *)sloth_pop(x);
	char tooltip[STR_BUF_LEN];
	SDL_Surface *icon = (SDL_Surface *)sloth_pop(x);
	if (tooltip_str[tooltip_len] != 0) {
		int i;
		for (i = 0; i < tooltip_len; i++) tooltip[i] = tooltip_str[i];
		tooltip[tooltip_len] = 0;
		tooltip_str = tooltip;
	}
	sloth_push(x, (CELL)SDL_CreateTray(icon, tooltip_str));
}

void sloth2SDL3_CreateTrayMenu_(X* x) {
	SDL_Tray *tray = (SDL_Tray *)sloth_pop(x);
	sloth_push(x, (CELL)SDL_CreateTrayMenu(tray));
}

void sloth2SDL3_InsertTrayEntryAt_(X* x) {
	SDL_TrayEntryFlags flags = (SDL_TrayEntryFlags)sloth_pop(x);
	CELL label_len = sloth_pop(x);
	char *label_str = (char *)sloth_pop(x);
	char label[STR_BUF_LEN];
	int pos = (int)sloth_pop(x);
	SDL_TrayMenu *menu = (SDL_TrayMenu *)sloth_pop(x);
	if (label_str[label_len] != 0) {
		int i;
		for (i = 0; i < label_len; i++) label[i] = label_str[i];
		label[label_len] = 0;
		label_str = label;
	}
	sloth_push(x, 
		(CELL)SDL_InsertTrayEntryAt(menu, pos, label_str, flags));
}

/* SDL_stdinc.h */
void sloth2SDL3_malloc_(X* x) {
	size_t size = (size_t)sloth_pop(x);
	void *ptr = SDL_malloc(size);
	if (ptr) {
		sloth_push(x, (CELL)ptr);
		sloth_push(x, 0);
	} else {
		sloth_push(x, -256);
	}
}

void sloth2SDL3_free_(X* x) {
	void *mem = (void *)sloth_pop(x);
	SDL_free(mem);
}

void sloth2SDL3_asprintfs_(X* x) {
	CELL str_len = sloth_pop(x);
	char *str = (char *)sloth_pop(x);
	CELL fmt_len = sloth_pop(x);
	char *fmt = (char *)sloth_pop(x);
	char str_temp[STR_BUF_LEN], fmt_temp[STR_BUF_LEN];
	char *strp;
	int nbytes;
	if (str[str_len] != 0) {
		int i;
		for (i = 0; i < str_len; i++) str_temp[i] = str[i];
		str_temp[str_len] = 0;
		str = str_temp;
	}
	if (fmt[fmt_len] != 0) {
		int i;
		for (i = 0; i < fmt_len; i++) fmt_temp[i] = fmt[i];
		fmt_temp[fmt_len] = 0;
		fmt = fmt_temp;
	}
	nbytes = SDL_asprintf(&strp, fmt, str);
	if (nbytes < 0) {
		sloth_push(x, -256);	
	} else {
		sloth_push(x, (CELL)strp);
		sloth_push(x, nbytes);
		sloth_push(x, 0);
	}
}

void sloth2SDL3_rand_(X* x) {
	Sint32 n = (Sint32)sloth_pop(x);
	sloth_push(x, SDL_rand(n));
}

void sloth2SDL3_randf_(X* x) {
	sloth_fpush(x, SDL_randf());
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
	sloth_constant(x, SDL_APP_CONTINUE, "SDL-APP-CONTINUE");
	sloth_constant(x, SDL_APP_FAILURE, "SDL-APP-FAILURE");
	sloth_constant(x, SDL_APP_SUCCESS, "SDL-APP-SUCCESS");

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

	sloth_constant(x, sizeof(SDL_Event), "SDL-Event"); 
	SLOTH2SDL3_CODE("SDL-Event.type", Event_type);
	SLOTH2SDL3_CODE("SDL-Event.event", Event_event);

	sloth_constant(x, sizeof(SDL_JoyDeviceEvent), "SDL-JoyDeviceEvent");
	SLOTH2SDL3_CODE("SDL-JoyDeviceEvent.type", Event_type);
	SLOTH2SDL3_CODE("SDL-JoyDeviceEvent.timestamp", JoyDeviceEvent_timestamp);
	SLOTH2SDL3_CODE("SDL-JoyDeviceEvent.which", JoyDeviceEvent_which);

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

	/* SDL_audio.h */
	sloth_evaluate(x,
		"BEGIN-STRUCTURE SDL-AudioSpec "
		"  INTFIELD: SDL-AudioSpec.format "
		"  INTFIELD: SDL-AudioSpec.channels "
		"  INTFIELD: SDL-AudioSpec.freq "
		"END-STRUCTURE ");

	sloth_constant(x, SDL_AUDIO_U8, "SDL-AUDIO-U8");
	sloth_constant(x, SDL_AUDIO_S16LE, "SDL-AUDIO-S16LE");

	sloth_constant(x, SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, "SDL-AUDIO-DEVICE-DEFAULT-PLAYBACK");

	SLOTH2SDL3_CODE("SDL-PutAudioStreamData", PutAudioStreamData);
	SLOTH2SDL3_CODE("SDL-GetAudioStreamQueued", GetAudioStreamQueued);
	SLOTH2SDL3_CODE("SDL-ResumeAudioStreamDevice", ResumeAudioStreamDevice);
	SLOTH2SDL3_CODE("SDL-OpenAudioDeviceStream", OpenAudioDeviceStream);
	SLOTH2SDL3_CODE("SDL-LoadWAV", LoadWAV);

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
	SLOTH2SDL3_CODE("SDL-SetRenderScale", SetRenderScale);
	SLOTH2SDL3_CODE("SDL-SetRenderDrawColor", SetRenderDrawColor);
	SLOTH2SDL3_CODE("SDL-SetRenderDrawColorFloat", SetRenderDrawColorFloat);
	SLOTH2SDL3_CODE("SDL-RenderClear", RenderClear);
	SLOTH2SDL3_CODE("SDL-RenderPoints", RenderPoints);
	SLOTH2SDL3_CODE("SDL-RenderLine", RenderLine);
	SLOTH2SDL3_CODE("SDL-RenderRect", RenderRect);
	SLOTH2SDL3_CODE("SDL-RenderFillRect", RenderFillRect);
	SLOTH2SDL3_CODE("SDL-RenderFillRects", RenderFillRects);
	SLOTH2SDL3_CODE("SDL-RenderPresent", RenderPresent);
	SLOTH2SDL3_CODE("SDL-RenderDebugText", RenderDebugText);
	SLOTH2SDL3_CODE("SDL-RenderDebugTextFormati", RenderDebugTextFormati);

	/* SDL_filesystem.h */
	SLOTH2SDL3_CODE("SDL-GetBasePath", GetBasePath);

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

	sloth_constant(x, SDL_ALPHA_OPAQUE, "SDL-ALPHA-OPAQUE");
	sloth_fconstant(x, SDL_ALPHA_OPAQUE_FLOAT, "SDL-ALPHA-OPAQUE-FLOAT");

	/* SDL_tray.h */
	SLOTH2SDL3_CODE("SDL-CreateTray", CreateTray);
	SLOTH2SDL3_CODE("SDL-CreateTrayMenu", CreateTrayMenu);
	SLOTH2SDL3_CODE("SDL-InsertTrayEntryAt", InsertTrayEntryAt);

	/* SDL_rect.h */
	sloth_evaluate(x,
		"BEGIN-STRUCTURE SDL-FRect "
		"  SFFIELD: SDL-FRect.x "
		"  SFFIELD: SDL-FRect.y "
		"  SFFIELD: SDL-FRect.w "
		"  SFFIELD: SDL-FRect.h "
		"END-STRUCTURE");

	sloth_evaluate(x,
		"BEGIN-STRUCTURE SDL-FPoint "
		" SFFIELD: SDL-FPoint.x "
		" SFFIELD: SDL-FPoint.y "
		"END-STRUCTURE");

	/* SDL_stdinc.h */
	sloth_fconstant(x, SDL_PI_D, "SDL-PI-D");

	SLOTH2SDL3_CODE("SDL-malloc", malloc);
	SLOTH2SDL3_CODE("SDL-free", free);
	SLOTH2SDL3_CODE("SDL-asprintfs", asprintfs);
	SLOTH2SDL3_CODE("SDL-rand", rand);
	SLOTH2SDL3_CODE("SDL-randf", randf)
	SLOTH2SDL3_CODE("SDL-fabsf", fabsf);
	SLOTH2SDL3_CODE("SDL-sin", sin);
}
