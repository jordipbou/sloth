800 constant screen-width
450 constant screen-height

: structure: ( n -- )
	here swap allot constant
;

Rectangle structure: player
400E player rectangle.x sf!
280E player rectangle.y sf!
40E player rectangle.width sf!
40E player rectangle.height sf!

100 constant max-buildings

: array: ( n len -- ) ( i -- addr )
	create dup , * allot
	does> dup @ rot * + cell+
;

max-buildings rectangle array: buildings
max-buildings color array: build-colors

fvariable spacing 
0E spacing f!

camera2d structure: camera
player rectangle.x sf@ camera camera2d.target.x sf!
player rectangle.y sf@ camera camera2d.target.y sf!
screen-width s>f 2E f/ camera camera2d.offset.x sf!
screen-height s>f 2E f/ camera camera2d.offset.y sf!
0E camera camera2d.rotation sf!
1E camera camera2d.zoom sf!

: main
	max-buildings 0 do
		50 200 get-random-value s>f i buildings rectangle.width sf!
		100 800 get-random-value s>f i buildings rectangle.height sf!
		screen-height s>f 130.E f- i buildings rectangle.height sf@ f- i buildings rectangle.y sf!
		-6000.E spacing f@ f+ i buildings rectangle.x sf!

		spacing f@ i buildings rectangle.width sf@ f+ spacing f!

		200 240 get-random-value i build-colors color.r c!
		200 240 get-random-value i build-colors color.g c!
		200 250 get-random-value i build-colors color.b c!
		255 i build-colors color.a c!
	loop

	screen-width screen-height
	s" raylib [core] example - 2d camera"
	init-window

	60 set-target-fps

	begin Tmark
		window-should-close 0= while
		
		\ -- Update
	
		\ Player movement
		key-right is-key-down if
			player dup rectangle.x sf@ 2E f+ rectangle.x sf!
		then
		key-left is-key-down if
			player dup rectangle.x sf@ 2E f- rectangle.x sf!
		then

		\ Camera target follows player
		player rectangle.x sf@ 20E f+ camera camera2d.target.x sf!
		player rectangle.y sf@ 20E f+ camera camera2d.target.y sf!

		\ Camera rotation controls
		key-a is-key-down if
			camera dup camera2d.rotation sf@ 1E f- camera2d.rotation sf!
		then
		key-s is-key-down if
			camera dup camera2d.rotation sf@ 1E f+ camera2d.rotation sf!
		then

		\ Limit camera rotation to 80 degrees (-40 to 40)
		camera camera2d.rotation sf@ 40E f> if
			40E camera camera2d.rotation sf!
		else
			camera camera2d.rotation sf@ -40E f< if
				-40E camera camera2d.rotation sf!
			then
		then

		\ Camera zoom controls
		\ Uses log scaling to provide consistent zoom speed
		get-mouse-wheel-move 0.1E f*
		camera camera2d.zoom sf@ fln
		f+ fexp
		camera camera2d.zoom sf!

		camera camera2d.zoom sf@ 3E f> if
			3E camera camera2d.zoom sf!
		else
			camera camera2d.zoom sf@ 0.1E f< if
				0.1E camera camera2d.zoom sf!
			then
		then

		\ Camera reset (zoom and rotation)
		key-r is-key-pressed if
			1E camera camera2d.zoom sf!
			0E camera camera2d.rotation sf!
		then

		\ -- Draw

		begin-drawing
			
			raywhite clear-background

			camera begin-mode-2d

				-6000 320 13000 8000 darkgray draw-rectangle	

				max-buildings 0 do
					i buildings i build-colors draw-rectangle-rec
				loop

				player RED draw-rectangle-rec

				camera camera2d.target.x sf@ f>s
				0 screen-height - 10 *
				camera camera2d.target.x sf@ f>s
				screen-height 10 *
				green
				draw-line

				0 screen-width - 10 *
				camera camera2d.target.y sf@ f>s
				screen-width 10 *
				camera camera2d.target.y sf@ f>s
				green
				draw-line

			end-mode-2d

			s" SCREEN AREA" 640 10 20 red draw-text

			0 0 screen-width 5 red draw-rectangle
			0 5 5 screen-height 10 - red draw-rectangle
			screen-width 5 - 5 5 screen-height 10 - red draw-rectangle
			0 screen-height 5 - screen-width 5 red draw-rectangle

			10 10 250 113 
			0.5E skyblue Tfade
			draw-rectangle

			10 10 250 113 blue draw-rectangle-lines

			s" Free 2d camera controls:" 20 20 10 black draw-text
			s" - Right/Left to move Offset" 40 40 10 darkgray draw-text
			s" - Mouse Wheel to Zoom in-out" 40 60 10 darkgray draw-text
			s" - A / S to Rotate" 40 80 10 darkgray draw-text
			s" - R to reset Zoom and Rotation" 40 100 10 darkgray draw-text

		end-drawing

	Tfree repeat

	\ De-initialization

	close-window	\ Close window and OpenGL contex
;

main

