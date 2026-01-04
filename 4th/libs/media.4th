\ -- Event helpers

create (events-event) SDL-Event allot

: events-quit
	SDL-EVENT-QUIT (events-event) SDL-Event.type l!
	(events-event) SDL-PushEvent throw
;

\ -- Joystick helpers

\ TODO Allow up to 16 joysticks with 4 axis, 
\ 128 buttons and 1 hat each

\ TODO Add joystick-hat-event 

\ TODO Modify joystick words to accept joystick id,
\ and maybe add defjoy-button-event to use the first one?!

\ Allows only one joystick right now
variable (joystick-id)
create (joystick-button) 128 allot
variable (joystick-hat)

[:
	128 0 do 0 (joystick-button) i + b! loop
;] execute

: joystick-button-event ( n -- n )
	>r

	SDL-INIT-JOYSTICK SDL-WasInit 0= if
		SDL-INIT-JOYSTICK SDL-Init throw
	then

	(joystick-id) @ ?dup if 
		r@ SDL-GetJoystickButton if
			\ Button pressed
			r@ (joystick-button) + b@ if
				\ Was pressed before
				0 
			else
				\ Was not pressed before
				-1 r@ (joystick-button) + !
				1
			then
		else
			\ Button released
			r@ (joystick-button) + b@ if
				\ Was pressed before
				0 r@ (joystick-button) + !
				-1
			else
				\ Was not pressed before
				0
			then
		then
	else
		\ No event if no joystick present
		0
	then

	r> drop
;

: joystick-hat-event ( -- n )
	SDL-INIT-JOYSTICK SDL-WasInit 0= if
		SDL-INIT-JOYSTICK SDL-Init throw
	then

	(joystick-id) @ ?dup if 
		0 SDL-GetJoystickHat dup
		(joystick-hat) @ = if
				drop -1
			else
			dup (joystick-hat) !
		then
	else
		\ Just return as centered
		SDL-HAT-CENTERED
	then
;

\ Gives basic joystick add/remove handling.
: joystick-add-remove ( event -- )
	SDL-INIT-JOYSTICK SDL-WasInit 0= if
		SDL-INIT-JOYSTICK SDL-Init throw
	then

	>r r@ SDL-Event.type l@ CASE
		SDL-EVENT-JOYSTICK-ADDED OF
			(joystick-id) @ 0= if
				r@ SDL-JoyDeviceEvent.which l@ SDL-OpenJoystick
				?dup if
					(joystick-id) !	
				else
					\ TODO Modify this to throw an error
					s" Failed to open joystick ID %u: %s"
					r@ SDL-JoyDeviceEvent.which l@ 
					SDL-GetError
				then
			then
		ENDOF
		SDL-EVENT-JOYSTICK-REMOVED OF 
			(joystick-id) @ if
				r@ SDL-JoyDeviceEvent.which l@
				(joystick-id) @ SDL-GetJoystickID = if
					(joystick-id) @ SDL-CloseJoystick
					0 (joystick-id) !
				then
			then
		ENDOF
	ENDCASE r> drop 
;

\ This can be rewritten by the user if needed, but
\ this will allow basic event handling of adding/removing
\ joysticks.
: appevent
	dup joystick-add-remove
	SDL-Event.type l@ SDL-EVENT-QUIT = if
		SDL-APP-SUCCESS
	else
		SDL-APP-CONTINUE
	then
;

\ -- Audio helpers

variable (audio-device) 0 (audio-device) !
variable (audio-stream) 0 (audio-stream) !
variable (audio-wav-data)
variable (audio-wav-data-len)

create (audio-spec) SDL-AudioSpec allot

: audio-playWAV ( filename -- )
	SDL-INIT-AUDIO SDL-WasInit 0= if
		SDL-INIT-AUDIO SDL-Init throw
	then

	(audio-device) @ 0= if
		SDL-AUDIO-F32 (audio-spec) SDL-AudioSpec.format	INT!
		2 (audio-spec) SDL-AudioSpec.channels INT!
		48000 (audio-spec) SDL-AudioSpec.freq INT!

		SDL-AUDIO-DEVICE-DEFAULT-PLAYBACK
		(audio-spec)
		SDL-OpenAudioDevice throw
		(audio-device) !
	then

	(audio-spec)
	SDL-LoadWAV throw
	(audio-wav-data-len) !
	(audio-wav-data) !

	(audio-stream) @ if
		(audio-stream) @ SDL-DestroyAudioStream
	then

	(audio-spec)
	0
	SDL-CreateAudioStream throw
	(audio-stream) !

	(audio-device) @
	(audio-stream) @
	SDL-BindAudioStream throw

	(audio-stream) @
	(audio-wav-data) @
	(audio-wav-data-len) @
	SDL-PutAudioStreamData throw
;

\ Default appiterate for the case that a sound is played
\ and no appiterate word has been defined, to allow the
\ sound to be played until it ends.
: appiterate
	SDL-INIT-AUDIO SDL-WasInit if
		(audio-stream) @ SDL-GetAudioStreamAvailable -256 = if
			SDL-GetError type cr
		else
			0= if
				SDL-APP-SUCCESS
			then
		then
	then
	
	SDL-APP-CONTINUE
;

: audio-close
	SDL-INIT-AUDIO SDL-WasInit if
		(audio-device) @ ?dup if SDL-CloseAudioDevice then
	then
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
		SDL-PROP-APP-METADATA-NAME-STRING SDL-GetAppMetadataProperty
		tray-create
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
	(tray) @ if
		(tray) @ SDL-DestroyTray
		0 (tray) !
		0 (tray-menu) !
		0 (tray-entry) !
	then
;

\ -- Default app quit for all systems

: appquit
	audio-close
	tray-destroy
;
