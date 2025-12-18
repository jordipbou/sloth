variable stream
variable wav-data
variable wav-data-len

create spec SDL-AudioSpec allot

s" Example Audio Load Wave"
s" 1.0"
s" com.example.audio-load-wav"
SDL-SetAppMetadata

SDL-INIT-VIDEO SDL-INIT-AUDIO or
SDL-Init throw

s" examples/audio/load-wav"
640 480
SDL-WINDOW-RESIZABLE
SDL-CreateWindowAndRenderer throw
(renderer) !
(window) !

(renderer) @
640 480
SDL-LOGICAL-PRESENTATION-LETTERBOX
SDL-SetRenderLogicalPresentation throw

s" %ssample.wav"
SDL-GetBasePath throw
SDL-asprintfs throw over swap \ strp strp n

spec
SDL-LoadWAV throw
wav-data-len !
wav-data !

SDL-free \ Free the string returned by SDL-asprintfs

SDL-AUDIO-DEVICE-DEFAULT-PLAYBACK
spec
0 0
SDL-OpenAudioDeviceStream throw
stream !

stream @ SDL-ResumeAudioStreamDevice

: appiterate
	stream @ SDL-GetAudioStreamQueued throw
	wav-data-len @ < if
		stream @
		wav-data @
		wav-data-len @
		SDL-PutAudioStreamData throw
	then

	(renderer) @ SDL-RenderClear throw
	(renderer) @ SDL-RenderPresent throw

	SDL-APP-CONTINUE
;

: appquit
	wav-data @ SDL-free
;
