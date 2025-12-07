require arrays.4th

500 SDL-FPoint array: points

: appinit
	s" Example Renderer Primitives"
	s" 1.0"
	s" com.example.renderer-primitives"
	SDL-SetAppMetadata

	SDL-INIT-VIDEO SDL-Init throw

	s" examples/renderer/primitive"
	640 480
	SDL-WINDOW-RESIZABLE
	SDL-CreateWindowAndRenderer throw
	(renderer) !
	(window) !

	(renderer) @
	640 480
	SDL-LOGICAL-PRESENTATION-LETTERBOX
	SDL-SetRenderLogicalPresentation throw

	500 0 do
		SDL-randf 440.0 f* 100.0 f+ i points SDL-FPoint.x sf!
		SDL-randf 280.0 f* 100.0 f+ i points SDL-FPoint.y sf!
	loop

	SDL-APP-CONTINUE
;

: appevent
	SDL-Event.type SDL-EVENT-QUIT = if
		SDL-APP-SUCCESS
	else
		SDL-APP-CONTINUE
	then
;

create rect SDL-FRect allot

: appiterate
	\ As you can see from this, rendering draws over 
	\ whatever was drawn before
	(renderer) @ 33 33 33 SDL-ALPHA-OPAQUE \ dark gray, full alpha
	SDL-SetRenderDrawColor throw 

	(renderer) @ SDL-RenderClear throw \ start with a blank canvas

	\ Draw a filled rectangle in the middle of the canvas
	(renderer) @ 0 0 255 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw

	100.0 rect SDL-FRect.x sf! 
	100.0 rect SDL-FRect.y sf!
	440.0 rect SDL-FRect.w sf!
	280.0 rect SDL-FRect.h sf!

	(renderer) @ rect SDL-RenderFillRect throw

	\ Draw some points across the canvas
	(renderer) @ 255 0 0 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw

	(renderer) @ 0 points 500 SDL-RenderPoints throw

	\ Draw a unfilled rectangle in-set a little bit
	(renderer) @ 0 255 0 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw

	rect SDL-FRect.x sf@ 30.0 f+ rect SDL-FRect.x sf!
	rect SDL-FRect.y sf@ 30.0 f+ rect SDL-FRect.y sf!
	rect SDL-FRect.w sf@ 60.0 f- rect SDL-FRect.w sf!
	rect SDL-FRect.h sf@ 60.0 f- rect SDL-FRect.h sf!

	(renderer) @ rect SDL-RenderRect throw

	\ Draw two lines in an X across the whole canvas
	(renderer) @ 255 255 0 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw

	(renderer) @ 0.0 0.0 640.0 480.0 SDL-RenderLine throw
	(renderer) @ 0.0 480.0 640.0 0.0 SDL-RenderLine throw

	\ Put it all on the screen!
	(renderer) @ SDL-RenderPresent throw

	SDL-APP-CONTINUE
;
