require libs/media.4th

create event SDL-Event allot

[:
	." See you later, alligator" cr
	drop \ entry pointer
	SDL-EVENT-QUIT event SDL-Event.type l!
	event SDL-PushEvent throw
;] tray-callback: quit-callback

[: 
	." Ernie, my man!" cr
;] tray-callback: write-callback

: appinit
	s" Hello, world!" tray-create
	tray-submenu
	write-callback s" An entry" tray-entry
	tray-end-submenu
	s" Just testing" tray-disabled
	tray-separator
	quit-callback s" Quit" tray-entry

	SDL-APP-CONTINUE
;

: appquit
	tray-destroy
;
