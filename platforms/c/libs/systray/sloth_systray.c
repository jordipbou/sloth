#include"sloth_systray.h"

#define SLOTH2SYSTRAY_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2systray_##f##_));

void sloth2systray_init_(X* x) {
	CELL tl = sloth_pop(x);
	char* ta = (char*)sloth_pop(x);
	CELL ipl = sloth_pop(x);
	char* ipa = (char*)sloth_pop(x);
	char tooltip[80];
	char iconpath[80];
	int i;
	for (i = 0; i < tl; i++) tooltip[i] = ta[i];
	tooltip[tl] = 0;
	for (i = 0; i < ipl; i++) iconpath[i] = ipa[i];
	iconpath[ipl] = 0;
	sloth_push(x, Systray_Init(iconpath, tooltip));
}

void sloth2systray_add_menu_item_(X* x) {
	CELL id = sloth_pop(x);
	CELL labellen = sloth_pop(x);
	char* labeladdr = (char*)sloth_pop(x);
	Systray* systrayid = (Systray*)sloth_pop(x);
	char label[255];
	int i;
	for (i = 0; i < labellen; i++) label[i] = labeladdr[i];
	label[labellen] = 0;
	Systray_AddMenuItem(systrayid, label, id);
}

void sloth2systray_poll_event_(X* x) {
	Systray* systrayid = (Systray*)sloth_pop(x);
	sloth_push(x, Systray_PollEvent(systrayid));
}

void sloth2systray_close_(X* x) {
	Systray* systrayid = (Systray*)sloth_pop(x);
	Systray_Close(systrayid);
}

void sloth_bootstrap_systray(X* x) {
	/* TODO How to add this to their own wordlist? */
	SLOTH2SYSTRAY_CODE("INIT", init);
	SLOTH2SYSTRAY_CODE("ADD-MENU-ITEM", add_menu_item);
	SLOTH2SYSTRAY_CODE("POLL-EVENT", poll_event);
	SLOTH2SYSTRAY_CODE("CLOSE", close);
}
