800 constant screen-width
450 constant screen-height

: main
	screen-width screen-height s" raylib [core] example - basic window" init-window
	
	60 set-target-fps
	
	begin
		window-should-close 0= while
		begin-drawing
		raywhite clear-background
		s" Congrats! You created your first window!" 190 200 20 lightgray draw-text
		end-drawing
	repeat

	close-window
;

main
