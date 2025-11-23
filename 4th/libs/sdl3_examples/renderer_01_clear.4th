: appinit 
	s" Example Renderer Clear" 
	s" 1.0" 
	s" com.example.renderer-clear"
	SDL-SetAppMetadata

	SDL-INIT-VIDEO SDL-Init if
		s" examples/renderer/clear"
		640 480 SDL-WINDOW-RESIZABLE
		SDL-CreateWindowAndRenderer if
			(renderer) !
			(window) !

			(renderer) @
			640 480 SDL-LOGICAL-PRESENTATION-LETTERBOX
			SDL-SetRenderLogicalPresentation
		else
			\ Do SDL-CreateWindowAndRenderer error
			SDL-APP-FAILURE
		then
	else
		\ Do SDL-Init error	
		SDL-APP-FAILURE
	then

	SDL-APP-CONTINUE
;

: appevent
	SDL-Event.type SDL-EVENT-QUIT = if
		SDL-APP-SUCCESS
	else
		SDL-APP-CONTINUE
	then
;

fvariable now
: appiterate
	SDL-GetTicks s>f 1000.0 f/ now f!
	(renderer) @
	now f@ SDL-sin 0.5 f* 0.5 f+ \ red
	now f@ SDL-PI-D 2.0 f* 3.0 f/ f+ SDL-sin 0.5 f* 0.5 f+ \ green
	now f@ SDL-PI-D 4.0 f* 3.0 f/ f+ SDL-sin 0.5 f* 0.5 f+ \ blue
	SDL-ALPHA-OPAQUE-FLOAT
	SDL-SetRenderDrawColorFloat drop

	(renderer) @
	SDL-RenderClear drop

	(renderer) @
	SDL-RenderPresent drop

	SDL-APP-CONTINUE
;

' appinit (APP-INIT) !
' appevent (APP-EVENT) !
' appiterate (APP-ITERATE) !
