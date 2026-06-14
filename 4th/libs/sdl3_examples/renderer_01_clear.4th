\ App initialization

variable (renderer)
variable (window)

s" Example Renderer Clear" 
s" 1.0" 
s" com.example.renderer-clear"
SDL-SetAppMetadata throw

SDL-INIT-VIDEO SDL-Init throw

s" examples/renderer/clear"
640 480 SDL-WINDOW-RESIZABLE
SDL-CreateWindowAndRenderer throw
(renderer) !
(window) !

(renderer) @
640 480 SDL-LOGICAL-PRESENTATION-LETTERBOX
SDL-SetRenderLogicalPresentation throw

fvariable now
: appiterate 
	SDL-GetTicks s>f 1000.0 f/ now f!
	(renderer) @
	now f@ SDL-sin 0.5 f* 0.5 f+ \ red
	now f@ SDL-PI-D 2.0 f* 3.0 f/ f+ SDL-sin 0.5 f* 0.5 f+ \ green
	now f@ SDL-PI-D 4.0 f* 3.0 f/ f+ SDL-sin 0.5 f* 0.5 f+ \ blue
	SDL-ALPHA-OPAQUE-FLOAT
	SDL-SetRenderDrawColorFloat throw

	(renderer) @ SDL-RenderClear throw

	(renderer) @ SDL-RenderPresent throw

	SDL-APP-CONTINUE
;
