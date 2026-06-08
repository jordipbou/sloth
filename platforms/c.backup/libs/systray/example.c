#include <raylib.h>
#include "systray.h"

int main(void) {
	char* text1 = "Hello from Raylib";
	char* text2 = "Second message from Raylib";
	char* text = text1;
	int seltext = 1;

	SetConfigFlags(FLAG_WINDOW_HIDDEN);
	InitWindow(800, 450, "Raylib with systray");
	
	Systray* tray = Systray_Init("icon.ico", "My Raylib App");
	Systray_AddMenuItem(tray, "Open", 1);
	Systray_AddMenuItem(tray, "Exit", 2);
	
	while (!WindowShouldClose()) {
		int ev = Systray_PollEvent(tray);
		if (ev == 1) { 
			if (IsWindowState(FLAG_WINDOW_HIDDEN))
				ClearWindowState(FLAG_WINDOW_HIDDEN);
			else
				SetWindowState(FLAG_WINDOW_HIDDEN);
			if (seltext == 1) { text = text2; seltext = 2; }
			else { text = text1; seltext = 1; }
		}
		if (ev == 2) break;
		
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText(text, 10, 10, 20, BLACK);
		EndDrawing();
	}
	
	Systray_Close(tray);
	CloseWindow();
	
	return 0;
}

