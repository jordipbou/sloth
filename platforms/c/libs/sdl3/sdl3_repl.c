#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"file.h"
/*
#include"facility.h"
#include"locals.h"
#include"memory.h"
*/
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

void show_exception(X* x, int err, char *msg) {
	if (err) {
		char *text;
		if (err == -38) {
			SDL_asprintf(&text, "Script (%s) not found.", msg);
		} else if (err == -13) {
			CELL ibuf = *((CELL*)(x->u+20*sCELL));
			CELL ipos = *((CELL*)(x->u+21*sCELL));
			CELL ilen = *((CELL*)(x->u+22*sCELL));
			SDL_asprintf(&text, "Word (%.*s) not found.", (int)(ilen - ipos), (char *)(ibuf + ipos));
		} else {
			SDL_asprintf(&text, "Exception %d.", err);
		}

		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"Sloth exception",
			text,
			NULL);
	}
}

/* The four default functions for init, event, iterate and quit. */

/* TODO This makes no sense to exist */
void defaultAppInit(X* x) {
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;

	SDL_SetAppMetadata("Example Renderer Clear Modified", "1.0", "com.example.renderer-clear");
	
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		sloth_push(x, SDL_APP_FAILURE);
		return;
	}
	
	if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		sloth_push(x, SDL_APP_FAILURE);
		return;
	}

	SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	sloth_user_area_set(x, SLOTH_WINDOW, (CELL)window);
	sloth_user_area_set(x, SLOTH_RENDERER, (CELL)renderer);

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

/* TODO This makes no sense to exist */
void defaultAppIterate(X* x) {
	SDL_Renderer *renderer = (SDL_Renderer *)sloth_user_area_get(x, SLOTH_RENDERER);

	const double now = ((double)SDL_GetTicks()) / 1000.0;
	const float red = (float) (0.5 + 0.5 * SDL_sin(now));
	const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
	const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
	SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);
	
	SDL_RenderClear(renderer);
	
	SDL_RenderPresent(renderer);

	sloth_push(x, SDL_APP_CONTINUE);
}

/* TODO This makes no sense to exist */
void defaultAppQuit(X* x) {
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	CELL err;
	CELL init, event, iterate, quit;

	/* Bootstrap SLOTH. */
	ctx = sloth_new();

	sloth_bootstrap(ctx);

	sloth_bootstrap_file_wordset(ctx);

/*
	sloth_bootstrap_facility_wordset(ctx);
	sloth_bootstrap_locals_wordset(ctx);
	sloth_bootstrap_memory_wordset(ctx);
*/

	sloth_user_variable(ctx, "(APP-INIT)", SLOTH_APP_INIT, 0);
	sloth_user_variable(ctx, "(APP-EVENT)", SLOTH_APP_EVENT, 0);
	sloth_user_variable(ctx, "(APP-ITERATE)", SLOTH_APP_ITERATE, 0);
	sloth_user_variable(ctx, "(APP-QUIT)", SLOTH_APP_QUIT, 0);

	sloth_user_variable(ctx, "(WINDOW)", SLOTH_WINDOW, 0);
	sloth_user_variable(ctx, "(RENDERER)", SLOTH_RENDERER, 0);

	sloth_include(ctx, ROOT_PATH "4th/ans.4th");

	sloth_bootstrap_SDL3(ctx);
	/*
	sloth_include(ctx, ROOT_PATH "4th/libs/sloth_sdl3.4th");
	*/

	/* Set ROOT PATH */
	sloth_set_root_path(ctx, ROOT_PATH "4th/");

	if (argc > 1) {
		/* First parameter will be user script to be executed */
		int err = sloth_include(ctx, argv[1]);
		if (err) {
			show_exception(ctx, err, argv[1]);
			return SDL_APP_FAILURE;
		}
	} else {
		/* No script in parameters, try to launch a generic one */
		int err = sloth_include(ctx, "main.4th");
		if (err) {
			show_exception(ctx, err, NULL);
			/* TODO Launch a REPL (console or GUI) */
		} 
	}

	/* init = sloth_find_word(ctx, "APPINIT"); */
	event = sloth_find_word(ctx, "APPEVENT");
	iterate = sloth_find_word(ctx, "APPITERATE");
	quit = sloth_find_word(ctx, "APPQUIT");

	if (/* init || */ event || iterate || quit) {
		/*
		sloth_user_area_set(ctx,
			SLOTH_APP_INIT,
			init ?
				sloth_get_xt(ctx, init)
				: sloth_primitive(ctx, &defaultAppInit));
		*/

		sloth_user_area_set(ctx,
			SLOTH_APP_EVENT,
			event ?
				sloth_get_xt(ctx, event)
				: sloth_primitive(ctx, &defaultAppEvent));

		sloth_user_area_set(ctx,
			SLOTH_APP_ITERATE,
			iterate ?
				sloth_get_xt(ctx, iterate)
				: sloth_primitive(ctx, &defaultAppIterate));

		sloth_user_area_set(ctx,
			SLOTH_APP_QUIT,
			quit ?
				sloth_get_xt(ctx, quit)
				: sloth_primitive(ctx, &defaultAppQuit));

		/* Execute the app-init xt */
		/* There is no need to execute appinit as the user script */
		/* 
		sloth_catch(ctx, sloth_user_area_get(ctx, SLOTH_APP_INIT));
		err = sloth_pop(ctx);
		if (err != 0) {
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_INFORMATION,
				"Sloth",
				"Error on AppInit when evaluating script.",
				NULL);
			return SDL_APP_FAILURE;
		}
		*/

		/* Having the option to throw an exception from the script */
		/* there is no reason to need to specify SDL_APP_CONTINUE */
		/* in the script. */
		/* return sloth_pop(ctx); */
		return SDL_APP_CONTINUE;
	} else {
		return SDL_APP_SUCCESS;
	}
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	CELL err;

	sloth_push(ctx, (CELL)event);

	sloth_catch(ctx, sloth_user_area_get(ctx, SLOTH_APP_EVENT));
	err = sloth_pop(ctx);
	if (err != 0) {
		return SDL_APP_FAILURE;
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
		return SDL_APP_FAILURE;
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
}
