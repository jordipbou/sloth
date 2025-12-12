variable tray
variable menu
variable entry

: callback-quit
			
;

: appinit
	SDL-INIT-VIDEO SDL-Init throw

	0 s" My Tray" SDL-CreateTray tray !
	
	tray @ SDL-CreateTrayMenu menu !

	menu @ -1 s" Quit" SDL-TRAYENTRY-BUTTON
	SDL-InsertTrayEntryAt entry !


