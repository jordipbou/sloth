variable tray
variable menu
variable entry

create callback 2 cells allot
(self) callback !

create event SDL-Event allot

: callback-quit
	drop \ entry pointer
	SDL-EVENT-QUIT event SDL-Event.type l!
	event SDL-PushEvent throw
;

' callback-quit callback cell+ !

: appinit
	SDL-INIT-VIDEO SDL-Init throw

	0 s" My Tray" SDL-CreateTray tray !

	tray @ SDL-CreateTrayMenu menu !

	menu @ -1 s" Quit" SDL-TRAYENTRY-BUTTON
	SDL-InsertTrayEntryAt entry !

	entry @	callback SDL-SetTrayEntryCallback

	SDL-APP-CONTINUE
;

: appevent
	SDL-Event.type l@ SDL-EVENT-QUIT = if
		SDL-APP-SUCCESS
	else
		SDL-APP-CONTINUE
	then
;

: appquit
	tray @ SDL-DestroyTray
;
