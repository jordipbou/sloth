#include "sloth_sdl3.h"

#define SLOTH2SDL3_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2SDL3_##f##_));

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

void sloth_bootstrap_SDL3(X* x) {
	SLOTH2SDL3_CODE("SET-APP-METADATA", SetAppMetadata);
}
