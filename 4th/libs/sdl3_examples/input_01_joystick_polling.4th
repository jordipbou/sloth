begin-structure SDL-Color
	\ Using cfield is not correct on non 8 byte/char platforms
	cfield: SDL-Color.r
	cfield: SDL-Color.g
	cfield: SDL-Color.b
	cfield: SDL-Color.a
end-structure

\ Creating an array with this word stores the size of the
\ items in the word itself. 
: array: ( n sz -- ) ( i -- addr )
	create dup , * allot
	does> dup @ swap cell+ -rot * +
;

variable joystick 0 joystick !
64 SDL-Color array: colors

: appinit
	s" Example Input Joystick Polling"
	s" 1.0"
	s" com.example.input-joystick-polling"
	SDL-SetAppMetadata drop

	SDL-INIT-VIDEO SDL-INIT-JOYSTICK or
	SDL-Init if
		." Couldn't initialize SDL:" SDL-GetError type cr
		SDL-APP-FAILURE
	then

	s" examples/input/joystick-polling"
	640 480
	SDL-WINDOW-RESIZABLE
	SDL-CreateWindowAndRenderer if
		." Couldn't create window/renderer: " SDL-GetError type cr
		SDL-APP-FAILURE
	else
		(renderer) !
		(window) !
	then

	\ Initialize arrays
	64 0 do
		255 SDL-rand i colors SDL-Color.r c!
		255 SDL-rand i colors SDL-Color.g c!
		255 SDL-rand i colors SDL-Color.b c!
		255 colors SDL-Color.a c!
	loop

	SDL-APP-CONTINUE
;

: appevent ( SDL_Event -- SDL_AppResult )
	>r r@ SDL-Event.type CASE
	SDL-EVENT-QUIT OF r> drop SDL-APP-SUCCESS exit ENDOF
	SDL-EVENT-JOYSTICK-ADDED OF 
		joystick @ 0= if
			r@ SDL-Event.jdevice.which SDL-OpenJoystick
			?dup if
				joystick !	
			else
				s" Failed to open joystick ID %u: %s"
				r@ SDL-Event.jdevice.which
				SDL-GetError
			then
		then
	ENDOF
	SDL-EVENT-JOYSTICK-REMOVED OF 
		joystick @ if
			r@ SDL-Event.jdevice.which
			joystick @ SDL-GetJoystickID = if
				joystick @ SDL-CloseJoystick
				0 joystick !
			then
		then
	ENDOF
	ENDCASE
	r> drop SDL-APP-CONTINUE
;

: appiterate
;
