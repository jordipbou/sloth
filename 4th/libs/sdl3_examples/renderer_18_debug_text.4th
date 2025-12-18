640 constant WINDOW-WIDTH
480 constant WINDOW-HEIGHT

s" Example Renderer Debug Texture"
s" 1.0"
s" com.example.renderer-debug-text"
SDL-SetAppMetadata

SDL-INIT-VIDEO SDL-Init throw

s" examples/renderer/debug-text"
WINDOW-WIDTH WINDOW-HEIGHT
SDL-WINDOW-RESIZABLE
SDL-CreateWindowAndRenderer throw
(renderer) !
(window) !

(renderer) @
WINDOW-WIDTH WINDOW-HEIGHT
SDL-LOGICAL-PRESENTATION-LETTERBOX
SDL-SetRenderLogicalPresentation throw

: appevent
	SDL-Event.type l@ SDL-EVENT-QUIT = if
		SDL-APP-SUCCESS
	else
		SDL-APP-CONTINUE
	then
;

SDL-DEBUG-TEXT-FONT-CHARACTER-SIZE constant charsize

: appiterate
	(renderer) @ 0 0 0 SDL-ALPHA-OPAQUE SDL-SetRenderDrawColor throw
	(renderer) @ SDL-RenderClear throw

	(renderer) @ 255 255 255 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw
	(renderer) @ 272.0 100.0 s" Hello world!" 
	SDL-RenderDebugText throw
	(renderer) @ 224.0 150.0 s" This is some debug text."
	SDL-RenderDebugText throw

	(renderer) @ 51 102 255 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw
	(renderer) @ 184.0 200.0 s" You can do it in different colors."
	SDL-RenderDebugText throw

	(renderer) @ 255 255 255 SDL-ALPHA-OPAQUE
	SDL-SetRenderDrawColor throw
	(renderer) @ 4.0 4.0 SDL-SetRenderScale throw
	(renderer) @ 14.0 65.0 s" It can be scaled"
	SDL-RenderDebugText throw
	(renderer) @ 1.0 1.0 SDL-SetRenderScale throw
	(renderer) @ 64.0 350.0 s" This only does ASCII chars. So this laughing emoji won't draw: 🤣"
	SDL-RenderDebugText throw

	(renderer) @
	WINDOW-WIDTH s>f charsize s>f 46.0 f* f- 2.0 f/
	400.0
	s" (This program has been running for %llu seconds.)"
	SDL-GetTicks 1000 / 
	SDL-RenderDebugTextFormati throw

	(renderer) @ SDL-RenderPresent throw

	SDL-APP-CONTINUE
;
