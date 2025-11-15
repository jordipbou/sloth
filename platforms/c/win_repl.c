#define SLOTH_IMPLEMENTATION
#include"fsloth.h"
#include"facility.h"
#include"file.h"
#include"locals.h"
#include"memory.h"
#include"cpnbi.h"

#include"sloth_sqlite.h"
#include"sloth_raylib.h"
#include"sloth_plibsys.h"
#include"sloth_systray.h"
#include"sloth_osdialog.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
	int argc = 0;
	LPWSTR *argvw;

	X* x = sloth_new();
	
	sloth_bootstrap(x);
	sloth_bootstrap_facility_wordset(x);
	sloth_bootstrap_file_wordset(x);
	sloth_bootstrap_locals_wordset(x);
	sloth_bootstrap_memory_wordset(x);

	sloth_include(x, ROOT_PATH "4th/ans.4th");

	sloth_bootstrap_sqlite(x);
	sloth_include(x, ROOT_PATH "4th/libs/sloth_sqlite.4th");

	sloth_bootstrap_raylib(x);
	sloth_include(x, ROOT_PATH "4th/libs/sloth_raylib.4th");

	sloth_bootstrap_plibsys(x);

	sloth_bootstrap_systray(x);
	sloth_bootstrap_osdialog(x);

	/* TODO Add press_key and release_key library */

	sloth_set_root_path(x, ROOT_PATH "4th/");

	// Attach console
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		FILE *fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
	}
	
	argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
	
	if (argvw == NULL || argc <= 1) {
		/* No arguments, try to load a default.4th file */
		char* default_script = "default.4th";
		sloth_push(x, default_script);
		sloth_push(x, strlen(default_script));
		sloth_catch(x, sloth_get_xt(x, sloth_find_word(x, "INCLUDED")));
		if (sloth_pop(x)) {
			/* No default.4th present */
			printf("No default.4th found\n");
		}
	} else {
	  // --- WideCharToMultiByte SAFE two-step conversion ---
	  int needed = WideCharToMultiByte(
	      CP_UTF8,
	      0,
	      argvw[1],
	      -1,
	      NULL,
	      0,
	      NULL,
	      NULL
	  );
	
	  if (needed == 0) {
	      printf("WideCharToMultiByte (query) failed. Error: %lu\n", GetLastError());
	      LocalFree(argvw);
	      return 1;
	  }
	
	  char *buf = (char*)malloc(needed);
	  if (buf) {
		  int result = WideCharToMultiByte(
	      CP_UTF8,
	      0,
	      argvw[1],
	      -1,
	      buf,
	      needed,
	      NULL,
	      NULL
		  );

		  if (result != 0) {
				sloth_include(x, buf);
			} else {
	      printf("WideCharToMultiByte (convert) failed. Error: %lu\n", GetLastError());
		  }
		} else {
	      printf("malloc failed\n");
	  }
	
	  free(buf);
	  LocalFree(argvw);
	}

	sloth_free(x);

	return 0;
}

