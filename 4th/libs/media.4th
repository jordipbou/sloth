\ -- Event helpers

create (events-event) SDL-Event allot

: events-quit
	SDL-EVENT-QUIT (events-event) SDL-Event.type l!
	(events-event) SDL-PushEvent throw
;

\ -- Audio helpers

variable (audio-stream)
variable (audio-wav-data)
variable (audio-wav-data-len)

create (audio-spec) SDL-AudioSpec allot

: audio-playWAV ( filename -- )
	SDL-INIT-AUDIO SDL-WasInit 0= if
		SDL-INIT-AUDIO SDL-Init throw
	then

	(audio-spec)
	SDL-LoadWAV throw
	(audio-wav-data-len) !
	(audio-wav-data) !

	SDL-AUDIO-DEVICE-DEFAULT-PLAYBACK
	(audio-spec)
	0 0
	SDL-OpenAudioDeviceStream throw
	(audio-stream) !
	
	(audio-stream) @ SDL-ResumeAudioStreamDevice

	(audio-stream) @
	(audio-wav-data) @
	(audio-wav-data-len) @
	SDL-PutAudioStreamData throw
;

\ Default appiterate for the case that a sound is played
\ and no appiterate word has been defined, to allow the
\ sound to be played until it ends.
: appiterate
	(audio-stream) @ SDL-GetAudioStreamAvailable -256 = if
		SDL-GetError type cr
	else
		0= if
			SDL-APP-SUCCESS
		then
	then
	
	SDL-APP-CONTINUE
;

\ -- Systray helpers

\ The idea here is that normally there will be just one
\ tray menu and, as such, we can store the references
\ in variables to make code a lot easier to read.

: tray-callback: ( xt "<spaces>name" -- )
	create (self) , ,
;

variable (tray) 0 (tray) !
variable (tray-menu) 0 (tray-menu) !
variable (tray-submenu) 0 (tray-submenu) !
variable (tray-entry) 0 (tray-entry) !

: tray-create ( title -- )
	SDL-INIT-VIDEO SDL-WasInit 0= if
		SDL-INIT-VIDEO SDL-Init throw
	then
	0 -rot SDL-CreateTray dup (tray) !
	SDL-CreateTrayMenu (tray-menu) !
;

: tray-icon
	\ TO BE IMPLEMENTED	
;

: tray-create-entry-helper ( string flag -- )
	(tray) @ 0= if
		s" My App" tray-create
	then
	3 n>r 
	(tray-submenu) @ ?dup 0= if
		(tray-menu) @ 
	then
	-1 nr> drop
	SDL-InsertTrayEntryAt (tray-entry) !
;

: tray-entry ( xt string -- )
	SDL-TRAYENTRY-BUTTON tray-create-entry-helper
	(tray-entry) @ swap SDL-SetTrayEntryCallback
;

: tray-disabled ( string -- )
	SDL-TRAYENTRY-BUTTON SDL-TRAYENTRY-DISABLED or
	tray-create-entry-helper
;

: tray-separator ( -- )  0 0 tray-entry ;

: tray-submenu ( -- )
	s" Submenu" SDL-TRAYENTRY-SUBMENU tray-create-entry-helper
	(tray-entry) @ SDL-CreateTraySubmenu (tray-submenu) !	
;

: tray-end-submenu ( -- )
	0 (tray-submenu) !
;

: tray-destroy ( -- )
	(tray) @ SDL-DestroyTray
	0 (tray) !
	0 (tray-menu) !
	0 (tray-entry) !
;
