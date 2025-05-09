800 constant screen-width
450 constant screen-height

0 constant logo
1 constant title
2 constant gameplay
3 constant ending

variable current-screen
variable frames-counter

: main
	screen-width screen-height 
	s" raylib [core] example - basic screen manager" 
	init-window

	logo current-screen !
	0 frames-counter !
 
	60 set-target-fps

	begin
		window-should-close 0= while
		current-screen @ case
			logo of
				frames-counter 1+!
				frames-counter @ 120 > if
					title current-screen !
				then
			endof
			title of
				key-enter is-key-pressed
				gesture-tap is-gesture-detected
				or if gameplay current-screen ! then
			endof
			gameplay of
				key-enter is-key-pressed
				gesture-tap is-gesture-detected
				or if ending current-screen ! then
			endof
			ending of
				key-enter is-key-pressed
				gesture-tap is-gesture-detected
				or if title current-screen ! then
			endof
		endcase

		begin-drawing
			raywhite clear-background

			current-screen @ case
				logo of
					s" LOGO SCREEN" 20 20 40 lightgray draw-text
					s" WAIT for 2 SECONDS..." 290 220 20 gray draw-text
				endof
				title of
					0 0 screen-width screen-height green draw-rectangle
					s" TITLE SCREEN" 20 20 40 darkgreen draw-text
					s" PRESS ENTER or TAP to JUMP to GAMEPLAY SCREEN" 120 220 20 darkgreen draw-text
				endof
				gameplay of
					0 0 screen-width screen-height purple draw-rectangle
					s" GAMEPLAY SCREEN" 20 20 40 maroon draw-text
					s" PRESS ENTER or TAP to JUMP to ENDING SCREEN" 130 220 20 maroon draw-text
				endof
				ending of
					0 0 screen-width screen-height blue draw-rectangle
					s" ENDING SCREEN" 20 20 40 darkblue draw-text
					s" PRESS ENTER or TAP to JUMP to TITLE SCREEN" 120 220 20 darkblue draw-text
				endof
			endcase
		end-drawing
	repeat

	close-window
;

main

." Bye!" CR
