include ../../arrays.4th

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
		255 i colors SDL-Color.a c!
	loop

	SDL-APP-CONTINUE
;

variable joystick 0 joystick !

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

variable winw
variable winh 
variable text
variable text-len
variable total
variable hat
fvariable size
fvariable thirdsize
fvariable x
fvariable y
fvariable dx

create dst SDL-FRect allot
2 SDL-FRect array: cross

: appiterate
	640 winw !
	480 winh !
	s" Plug in a joystick, please." text-len ! text !

	joystick @ if
		joystick @ SDL-GetJoystickName text-len ! text !
	then

	(renderer) @ 0 0 0 255 SDL-SetRenderDrawColor throw
	(renderer) @ SDL-RenderClear throw
	(window) @ SDL-GetWindowSize throw winh ! winw !

	joystick @ if
		30.0 size f!

		\ Draw axes as bars going across middle of screen
		joystick @ SDL-GetNumJoystickAxes total !
		winh @ s>f size f@ total @ s>f f* f- 2.0 f/ y f!
		winw @ s>f 2.0 f/ x f!
		total @ 0 ?do
			joystick @ i SDL-GetJoystickAxis s>f 32767.0 f/
			x f@ f* 
			x f@ f+ 
			dx f!

			dx f@ dst SDL-FRect.x sf!
			y f@ dst SDL-FRect.y sf!
			x f@ dx f@ SDL-fabsf f- dst SDL-FRect.w sf!
			size f@ dst SDL-FRect.h sf!

			(renderer) @ 
			i colors SDL-Color.r c@
			i colors SDL-Color.g c@
			i colors SDL-Color.b c@
			i colors SDL-Color.a c@
			SDL-SetRenderDrawColor throw

			(renderer) @ dst SDL-RenderFillRect throw

			y f@ size f@ f+ y f!
		loop

		\ Draw buttons as blocks across top of window
	 	joystick @ SDL-GetNumJoystickButtons total !
	 	winw @ s>f total @ s>f size f@ f* f- 2.0 f/ x f!
	 	total @ 0 ?do
	 	 	x f@ dst SDL-FRect.x sf!
	 	 	0.0 dst SDL-FRect.y sf!
	 	 	size f@ dst SDL-FRect.w sf!
	 	 	size f@ dst SDL-FRect.h sf!
	 	 	joystick @ i SDL-GetJoystickButton if
	 	 		(renderer) @
	 	 		i colors SDL-Color.r c@
	 	 		i colors SDL-Color.g c@
	 	 		i colors SDL-Color.b c@
	 	 		i colors SDL-Color.a c@
	 	 		SDL-SetRenderDrawColor throw
	 	 	else
	 	 		(renderer) @ 0 0 0 255
	 	 		SDL-SetRenderDrawColor throw
	 	 	then
	 	 	(renderer) @ dst SDL-RenderFillRect throw
	 	 	(renderer) @ 255 255 255 i colors SDL-Color.a
	 	 	SDL-SetRenderDrawColor throw
	 	 	(renderer) @ dst SDL-RenderRect throw
	 	 	x f@ size f@ f+ x f!
	 	loop

		\ Draw hats across the bottom of the screen
		joystick @ SDL-GetNumJoystickHats total !

		winw @ s>f total @ s>f size f@ 2.0 f* f* f- 2.0 f/
		size f@ 2.0 f/ f+ x f!

		winh @ s>f size f@ f- y f!

		total @ 0 ?do
			size f@ 3.0 f/ thirdsize f!

			x f@ 0 cross SDL-FRect.x sf!
			y f@ thirdsize f@ f+ 0 cross SDL-FRect.y sf!
			size f@ 0 cross SDL-FRect.w sf!
			thirdsize f@ 0 cross SDL-FRect.h sf!

			x f@ thirdsize f@ f+ 1 cross SDL-FRect.x sf!
			y f@ 1 cross SDL-FRect.y sf!
			thirdsize f@ 1 cross SDL-FRect.w sf!
			size f@ 1 cross SDL-FRect.h sf!

			joystick @ i SDL-GetJoystickHat hat !

			(renderer) @ 90 90 90 255 SDL-SetRenderDrawColor throw
			(renderer) @ 0 cross 2 SDL-RenderFillRects throw

			(renderer) @
			i colors SDL-Color.r c@
			i colors SDL-Color.g c@
			i colors SDL-Color.b c@
			i colors SDL-Color.a c@
			SDL-SetRenderDrawColor throw

			hat @ SDL-HAT-UP and if
				x f@ thirdsize f@ f+ dst SDL-FRect.x sf!
				y f@ dst SDL-FRect.y sf!
				thirdsize f@ dst SDL-FRect.w sf!
				thirdsize f@ dst SDL-FRect.h sf!
				(renderer) @ dst SDL-RenderFillRect throw
			then

			hat @ SDL-HAT-RIGHT and if
				x f@ thirdsize f@ 2.0 f* f+ dst SDL-FRect.x sf!
				y f@ thirdsize f@ f+ dst SDL-FRect.y sf!
				thirdsize f@ dst SDL-FRect.w sf!
				thirdsize f@ dst SDL-FRect.h sf!
				(renderer) @ dst SDL-RenderFillRect throw
			then

			hat @ SDL-HAT-DOWN and if
				x f@ thirdsize f@ f+ dst SDL-FRect.x sf!
				y f@ thirdsize f@ 2.0 f* f+ dst SDL-FRect.y sf!
				thirdsize f@ dst SDL-FRect.w sf!
				thirdsize f@ dst SDL-FRect.h sf!
				(renderer) @ dst SDL-RenderFillRect throw
			then

			hat @ SDL-HAT-LEFT and if
				x f@ dst SDL-FRect.x sf!
				y f@ thirdsize f@ f+ dst SDL-FRect.y sf!
				thirdsize f@ dst SDL-FRect.w sf!
				thirdsize f@ dst SDL-FRect.h sf!
				(renderer) @ dst SDL-RenderFillRect throw
			then

			size f@ 2.0 f* x f@ f+ x f!
		loop
	then

	winw @ text-len @ SDL-DEBUG-TEXT-FONT-CHARACTER-SIZE * - s>f
	2.0 f/ x f!

	winh @ SDL-DEBUG-TEXT-FONT-CHARACTER-SIZE - s>f 2.0 f/ y f!

	(renderer) @ 255 255 255 255 SDL-SetRenderDrawColor throw
	(renderer) @ x f@ y f@ text @ text-len @ SDL-RenderDebugText throw
	(renderer) @ SDL-RenderPresent throw

	SDL-APP-CONTINUE
;

: appquit
	joystick @ if
		joystick @ SDL-CloseJoystick
	then
;
