#ifndef RAYLIB_SYSTRAY_H
#define RAYLIB_SYSTRAY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Systray Systray;

Systray* Systray_Init(const char* iconPath, const char* tooltip);
void Systray_AddMenuItem(Systray* tray, const char* label, int id);
int Systray_PollEvent(Systray* tray);
void Systray_Close(Systray* tray);

#ifdef __cplusplus
}
#endif

#endif

