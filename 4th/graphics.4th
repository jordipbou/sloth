variable default-window-width
variable default-window-height
2variable default-title
variable default-fps

800 default-window-width !
600 default-window-height !
s" Sloth window" default-title 2!
60 default-fps !

: ?init-window ( -- )
	is-window-ready 0= if
		default-window-width @
		default-window-height @
		default-title 2@
		init-window

		default-fps @ set-target-fps
	then
;

\ Create a new GUI thread, open window and execute drawing
\ loop in the new thread.
\ Use a defered word to allow updating the drawing whenever
\ is required.

defer gui-loop-action
[: ;] is gui-loop-action

: gui-loop ( -- )
	begin
		window-should-close 0= while
		\ shouldn't be called with a catch ?
		gui-loop-action		
	repeat	
;

variable gui-thread

: ?init-gui ( -- )
	p-libsys-init \ this should be optional
	is-window-ready 0= if
		?init-window
		' gui-loop p-uthread-create gui-thread !
	then
;

\ refill? returns TRUE if a key was available and 
\ it was a RETURN key. 
\ It returns FALSE is there was no available key
\ or the available key was not RETURN.
\ If a key is available, store it in the input
\ buffer.
: refill? ( -- flag )
	key? if
		key dup (return-key) = if
			-1
		else
			dup (delete-key) = if
				\ TODO
			else
				\ TODO
			then
		then
	else
		0
	then
;

: gui-quit ( -- )
	(empty-return-stack)
	0 (source-id) !		\ Set source to user input device
	postpone [
	begin
		window-should-close 0=
	while
		refill? if
			['] interpret catch
			case
				0 of state @ 0= if
					." OK"
					depth 0 > if space depth . then
					print-fdepth
				else
					." Compiling"
				then cr endof
				postpone [
				-1 of ( TODO Aborted ) endof
				-2 of ( TODO display message from abort-quote ) endof
				( default ) dup ." Exception # " . cr
			endcase
			\ After doing what was needed, empty
			\ the input buffer to refill again.
		then
	repeat
;
