require libs/media.4th

create event SDL-Event allot

: quit-callback
	." See you later, alligator" cr
	drop \ entry pointer
	SDL-EVENT-QUIT event SDL-Event.type l!
	event SDL-PushEvent throw
;

: appinit
	s" Hello, world!" tray-create
	tray-separator
	s" Quit" tray-entry
	['] quit-callback tray-entry-callback

	SDL-APP-CONTINUE
;

: appquit
	tray-destroy
;
