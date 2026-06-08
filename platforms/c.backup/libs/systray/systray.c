#include "systray.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#error "systray currently only supports Windows."
#endif

#include <string.h>
#include <stdlib.h>

struct Systray {
    NOTIFYICONDATAA nid;
    HMENU menu;
    HWND hwnd; /* internal message window for tray notifications */
    UINT msgId;
    int lastEvent;
};

// Internal WndProc
static LRESULT CALLBACK Systray_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Systray* tray = (Systray*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (!tray) return DefWindowProcA(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_COMMAND:
            tray->lastEvent = LOWORD(wParam);
            break;
        case /*tray message*/ WM_USER + 1:
            if (lParam == WM_RBUTTONUP) {
                POINT p;
                GetCursorPos(&p);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(tray->menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, p.x, p.y, 0, hwnd, NULL);
            }
            break;
        case WM_DESTROY:
            Shell_NotifyIconA(NIM_DELETE, &tray->nid);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

Systray* Systray_Init(const char* iconPath, const char* tooltip) {
    HINSTANCE hInst = GetModuleHandleA(NULL);

    Systray* tray = (Systray*)calloc(1, sizeof(Systray));
    if (!tray) return NULL;

    tray->msgId = WM_USER + 1;
    tray->lastEvent = 0;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = Systray_WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "RaylibSystrayClass";
    RegisterClassA(&wc);

    tray->hwnd = CreateWindowExA(0, "RaylibSystrayClass", "", 0, 0, 0, 0, 0,
                              NULL, NULL, hInst, NULL);
    SetWindowLongPtrA(tray->hwnd, GWLP_USERDATA, (LONG_PTR)tray);

    tray->menu = CreatePopupMenu();

    ZeroMemory(&tray->nid, sizeof(tray->nid));
    tray->nid.cbSize = sizeof(NOTIFYICONDATAA);
    tray->nid.hWnd = tray->hwnd;
    tray->nid.uID = 1;
    tray->nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray->nid.uCallbackMessage = tray->msgId;
    tray->nid.hIcon = (HICON)LoadImageA(NULL, iconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    if (!tray->nid.hIcon) {
        // fallback to default application icon
        tray->nid.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    }
    if (tooltip) {
        strncpy(tray->nid.szTip, tooltip, sizeof(tray->nid.szTip) - 1);
        tray->nid.szTip[sizeof(tray->nid.szTip) - 1] = '\0';
    }

    Shell_NotifyIconA(NIM_ADD, &tray->nid);

    return tray;
}

void Systray_AddMenuItem(Systray* tray, const char* label, int id) {
    if (!tray) return;
    AppendMenuA(tray->menu, MF_STRING, id, label);
}

int Systray_PollEvent(Systray* tray) {
    if (!tray) return 0;
    int id = tray->lastEvent;
    tray->lastEvent = 0;
    return id;
}

void Systray_Close(Systray* tray) {
    if (!tray) return;
    Shell_NotifyIconA(NIM_DELETE, &tray->nid);
    DestroyMenu(tray->menu);
    DestroyWindow(tray->hwnd);
    if (tray->nid.hIcon) DestroyIcon(tray->nid.hIcon);
    free(tray);
}
