#include"sloth_osdialog.h"

#define SLOTH2OSDIALOG_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2osdialog_##f##_));

void sloth2osdialog_message_(X* x) {
	char message[500];
	CELL msglen = sloth_pop(x);
	char* msg = (char*)sloth_pop(x);
	CELL buttons = sloth_pop(x);
	CELL level = sloth_pop(x);
	if (msg[msglen] != 0) {
		int i;
		for (i = 0; i < msglen; i++) {
			message[i] = msg[i];
		}
		message[msglen] = 0;
		msg = message;
	} 
	osdialog_message(level, buttons, msg);
}

void sloth_bootstrap_osdialog(X* x) {
	SLOTH2OSDIALOG_CODE("MESSAGE", message);	
}
