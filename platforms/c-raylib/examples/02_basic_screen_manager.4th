800 constant screen-width
450 constant screen-height

0 constant logo
1 constant title
2 constant gameplay
3 constant ending

variable current-screen
variable frames-counter

: main
	screen-width screen-height s" raylib [core] example - basic screen manager" rl-init-window

	logo current-screen !
	0 frames-counter !
 
	60 rl-set-target-fps

	begin
		rl-window-should-close 0= while
		current-screen @ case
			logo of
				frames-counter 1+!
				frames-counter @ 120 > if
					title current-screen !
				then
			endof
			title of
				key-enter rl-is-key-pressed
				gesture-tap rl-is-gesture-detected
				or if gameplay current-screen ! then
			endof
			gameplay of
				key-enter rl-is-key-pressed
				gesture-tap rl-is-gesture-detected
				or if ending current-screen ! then
			endof
			ending of
				key-enter rl-is-key-pressed
				gesture-tap rl-is-gesture-detected
				or if title current-screen ! then
			endof
		endcase

		rl-begin-drawing
			raywhite rl-clear-background

			current-screen @ case
				logo of
					s" LOGO SCREEN" 20 20 40 lightgray rl-draw-text
					s" WAIT for 2 SECONDS..." 290 220 20 gray rl-draw-text
				endof
				title of
					0 0 screen-width screen-height green rl-draw-rectangle
					s" TITLE SCREEN" 20 20 40 darkgreen rl-draw-text
					s" PRESS ENTER or TAP to JUMP to GAMEPLAY SCREEN" 120 220 20 darkgreen rl-draw-text
				endof
				gameplay of
					0 0 screen-width screen-height purple rl-draw-rectangle
					s" GAMEPLAY SCREEN" 20 20 40 maroon rl-draw-text
					s" PRESS ENTER or TAP to JUMP to ENDING SCREEN" 130 220 20 maroon rl-draw-text
				endof
				ending of
					0 0 screen-width screen-height blue rl-draw-rectangle
					s" ENDING SCREEN" 20 20 40 darkblue rl-draw-text
					s" PRESS ENTER or TAP to JUMP to TITLE SCREEN" 120 220 20 darkblue rl-draw-text
				endof
			endcase
		rl-end-drawing
	repeat

	rl-close-window
;

main

." Bye!" CR
