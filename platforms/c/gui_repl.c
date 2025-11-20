#define SLOTH_IMPLEMENTATION
#include"sloth.h"
#include"facility.h"
#include"file.h"
#include"locals.h"
#include"memory.h"
#include"sloth_sdl3.h"

#define SLOTH_APP_INIT			SLOTH_LAST_USER_VAR+sCELL
#define SLOTH_APP_EVENT			SLOTH_APP_INIT+sCELL
#define SLOTH_APP_ITERATE		SLOTH_APP_EVENT+sCELL
#define SLOTH_APP_QUIT			SLOTH_APP_ITERATE+sCELL

#define SLOTH_WINDOW				SLOTH_APP_QUIT+sCELL
#define SLOTH_RENDERER			SLOTH_WINDOW+sCELL

#define SLOTH_LAST_USED_VAR	SLOTH_RENDERER+sCELL

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

static X* ctx;

/* The four default functions for init, event, iterate and quit. */
void defaultAppInit(X* x) {
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;

	SDL_SetAppMetadata("Example Renderer Clear Modified", "1.0", "com.example.renderer-clear");
	
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	
	if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	sloth_user_variable(x, "(WINDOW)", SLOTH_WINDOW, window);
	sloth_user_variable(x, "(RENDERER)", SLOTH_RENDERER, renderer);

	sloth_push(x, SDL_APP_CONTINUE);
}

void defaultAppEvent(X* x) {
	SDL_Event *event = (SDL_Event *)sloth_pop(x);

	if (event->type == SDL_EVENT_QUIT) {
		sloth_push(x, SDL_APP_SUCCESS);
	} else {
		sloth_push(x, SDL_APP_CONTINUE);
	}
}

void defaultAppIterate(X* x) {
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_user_area_get(x, SLOTH_RENDERER);

	const double now = ((double)SDL_GetTicks()) / 1000.0;
	const float red = (float) (0.5 + 0.5 * SDL_sin(now));
	const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
	const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
	SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);
	
	/* clear the window to the draw color. */
	SDL_RenderClear(renderer);
	
	/* put the newly-cleared rendering on the screen. */
	SDL_RenderPresent(renderer);

	sloth_push(x, SDL_APP_CONTINUE);
}

void defaultAppQuit(X* x) {
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	CELL err;

	/* Bootstrap SLOTH. */
	ctx = sloth_new();

	sloth_bootstrap(ctx);

	sloth_bootstrap_facility_wordset(ctx);
	sloth_bootstrap_file_wordset(ctx);
	sloth_bootstrap_locals_wordset(ctx);
	sloth_bootstrap_memory_wordset(ctx);

	/* Define four user variables to default functions for init, */
	/* event, iterate and quit. */
	sloth_user_variable(
		ctx, "(APP-INIT)", SLOTH_APP_INIT, 
		sloth_primitive(ctx, &defaultAppInit));
	sloth_user_variable(
		ctx, "(APP-EVENT)", SLOTH_APP_EVENT, 
		sloth_primitive(ctx, &defaultAppEvent));
	sloth_user_variable(
		ctx, "(APP-ITERATE)", SLOTH_APP_ITERATE, 
		sloth_primitive(ctx, &defaultAppIterate));
	sloth_user_variable(
		ctx, "(APP-QUIT)", SLOTH_APP_QUIT, 
		sloth_primitive(ctx, &defaultAppQuit));

	sloth_include(ctx, ROOT_PATH "4th/ans.4th");

	sloth_bootstrap_SDL3(ctx);
	sloth_include(ctx, ROOT_PATH "4th/libs/sloth_sdl3.4th");

	/* Execute default forth user code, it can change those */
	/* default functions to its own code. */
	sloth_include(ctx, "boot.4th");

	/* Execute the app-init xt */
	sloth_catch(ctx, sloth_user_area_get(ctx, SLOTH_APP_INIT));
	err = sloth_pop(ctx);
	if (err != 0) {
		/* TODO Manage the exception in some good way !!! */
	}

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	CELL err;

	sloth_push(ctx, event);

	sloth_catch(ctx, sloth_user_area_get(ctx, SLOTH_APP_EVENT));
	err = sloth_pop(ctx);
	if (err != 0) {
		/* TODO Manage the exception in some good way !!! */
	}

	return sloth_pop(ctx);
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
	CELL err;

	sloth_catch(ctx, sloth_user_area_get(ctx, SLOTH_APP_ITERATE));
	err = sloth_pop(ctx);
	if (err != 0) {
		/* TODO Manage the exception in some good way !!! */
	}

	return sloth_pop(ctx);
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	/* SDL will clean up the window/renderer for us. */
	CELL err;

	sloth_catch(ctx, sloth_user_area_get(ctx, SLOTH_APP_QUIT));
	err = sloth_pop(ctx);
	if (err != 0) {
		/* TODO Manage the exception in some good way !!! */
	}

	return sloth_pop(ctx);
}
