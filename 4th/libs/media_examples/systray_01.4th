require libs/media.4th

create event SDL-Event allot

[:
	\ TODO Why this one has entry on stack when called
	\ but the write-callback does not?
	.s cr
	." See you later, alligator" cr
	events-quit
	\ drop \ entry pointer
	\ SDL-EVENT-QUIT event SDL-Event.type l!
	\ event SDL-PushEvent throw
;] tray-callback: quit-callback

[: 
	\ TODO Why this one has no entry when called
	.s cr
	." Ernie, my man!" cr
;] tray-callback: write-callback

s" Hello, world!" 
tray-create

tray-submenu
	write-callback 
	s" An entry" 
	tray-entry
tray-end-submenu

s" Just testing" 
tray-disabled

tray-separator

quit-callback 
s" Quit" 
tray-entry

: appquit
	tray-destroy
;
