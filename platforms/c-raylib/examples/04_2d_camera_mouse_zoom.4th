800 constant screen-width
450 constant screen-height

: structure: ( n -- )
	here swap allot constant
;

camera2d structure: camera
0.0E camera camera2d.offset.x sf!
0.0E camera camera2d.offset.y sf!
0.0E camera camera2d.target.x sf!
0.0E camera camera2d.target.y sf!
0.0E camera camera2d.rotation sf!
1.0E camera camera2d.zoom sf!

variable zoom-mode 0 zoom-mode !

: sf, here sf! 1 sfloats allot ;

create mouse-text-pos -44E sf, -24E sf,

: to-camera-offset
	dup vector2.x sf@
	vector2.y sf@
	camera camera2d.offset.y sf!
	camera camera2d.offset.x sf!
;

: to-camera-target
	dup vector2.x sf@
	vector2.y sf@
	camera camera2d.target.y sf!
	camera camera2d.target.x sf!
;

: log-scale-zoom
	camera camera2d.zoom sf@ fln
	f+ fexp
	0.125E 64.0E
	clamp
	camera camera2d.zoom sf!
;

: main ( -- )
	screen-width screen-height
	s" raylib [core] example - 2d camera mouse zoom"
	init-window

	60 set-target-fps

	begin Tmark
		window-should-close 0= while

		key-one is-key-pressed if 0 zoom-mode !
		else key-two is-key-pressed if 1 zoom-mode ! then
		then

		mouse-button-left is-mouse-button-down if
			Tget-mouse-delta 
			-1.0E camera camera2d.zoom sf@ f/
			Tvector2scale

			camera camera2d.target.x
			swap
			Tvector2add

			to-camera-target
		then

		0 zoom-mode @ = if
			\ Zoom based on mouse wheel
			get-mouse-wheel-move ?dup if
				Tget-mouse-position
				camera
				Tget-screen-to-world-2d

				to-camera-target

				Tget-mouse-position
				to-camera-offset

				0.2E f* \ scale
				log-scale-zoom
			then
		else
			\ Zoom based on mouse right click
			mouse-button-right is-mouse-button-pressed if
				Tget-mouse-position
				camera
				Tget-screen-to-world-2d

				to-camera-target

				Tget-mouse-position
				to-camera-offset
			then
			mouse-button-right is-mouse-button-down if
				\ zoom increment
				\ uses log scaling to provide consistent
				\ zoom speed
				Tget-mouse-delta vector2.x sf@ \ deltaX
				0.005E f* \ scale
				log-scale-zoom
			then
		then

		begin-drawing
			raywhite clear-background

			camera begin-mode-2d

				\ draw the 3d grid, rotated 90 degrees
				\ and centered around 0,0 just so we have
				\ something in the XY plane
				rl-push-matrix
					0E 25E 50E f* 0E rl-translate
					90E 1E 0E 0E rl-rotatef
					100 50E draw-grid
				rl-pop-matrix

				get-screen-width 2 /
				get-screen-height 2 / 
				50E maroon
				draw-circle

			end-mode-2d

			\ Draw mouse reference
			Tget-mouse-position
			4E darkgray draw-circle-v

\			\ get-font-default
\			\ s" [%i, %i]" get-mouse-x get-mouse-y text-format-2
\			\ mouse-position mouse-text-pos vector2add
\			\ 20 2 black
\			\ draw-text-ex
\
			s" [1][2] Select mouse zoom mode (Wheel or Move)"
			20 20 20 darkgray
			draw-text

			zoom-mode @ if
				s" Mouse left button drag to move, mouse press and move to zoom"
				20 50 20 darkgray
				draw-text
			else
				s" Mouse left button drag to move, mouse wheel to zoom"
				20 50 20 darkgray
				draw-text
			then

		end-drawing

	Tfree repeat

	close-window
;

main
	
