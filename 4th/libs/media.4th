\ Systray helpers

variable (tray)
variable (tray-menu)
variable (tray-entry)

: tray-create ( title -- )
	SDL-INIT-VIDEO SDL-WasInit 0= if
		SDL-INIT-VIDEO SDL-Init throw
	then
	0 -rot SDL-CreateTray dup (tray) !
	SDL-CreateTrayMenu (tray-menu) !
;

: tray-icon
	\ TO BE IMPLEMENTED	
;

: tray-entry ( string -- )
	2>r (tray-menu) @ -1 2r> SDL-TRAYENTRY-BUTTON
	SDL-InsertTrayEntryAt (tray-entry) !
;

: tray-entry-callback ( xt -- )
	here >r
	(self) ,
	,
	(tray-entry) @ r> SDL-SetTrayEntryCallback
;

: tray-separator ( -- )  0 0 tray-entry ;

: tray-submenu ( -- )
	\ TO BE IMPLEMENTED
;

: tray-destroy ( -- )
	(tray) @ SDL-DestroyTray
;
