#include <fsloth.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_stdinc.h>

/* SDL_init.h */
void sloth2SDL3_Init_(X* x);
void sloth2SDL3_SetAppMetadata_(X* x);

/* SDL_error.h */
void sloth2SDL3_GetError_(X* x);

/* SDL_events.h */
void sloth2SDL3_Event_type_(X* x);
void sloth2SDL3_Event_jdevice_which_(X* x);

/* SDL_joystick.h */
void sloth2SDL3_OpenJoystick_(X* x);
void sloth2SDL4_GetJoystickName_(X* x);
void sloth2SDL3_GetJoystickID_(X* x);
void sloth2SDL3_GetNumJoystickAxes_(X* x);
void sloth2SDL3_GetNumJoystickHats_(X* x);
void sloth2SDL3_GetNumJoystickButtons_(X* x);
void sloth2SDL3_GetJoystickAxis_(X* x);
void sloth2SDL3_GetJoystickHat_(X* x);
void sloth2SDL3_GetJoystickButton_(X* x);
void sloth2SDL3_CloseJoystick_(X* x);

/* SDL_audio.h */
void sloth2SDL3_PutAudioStreamData_(X* x);
void sloth2SDL3_GetAudioStreamQueued_(X* x);
void sloth2SDL3_ResumeAudioStreamDevice_(X* x);
void sloth2SDL3_OpenAudioDeviceStream_(X* x);
void sloth2SDL3_LoadWAV_(X* x);

/* SDL_timer.h */
void sloth2SDL3_GetTicks_(X* x);

/* SDL_render.h */
void sloth2SDL3_CreateWindowAndRenderer_(X* x);
void sloth2SDL3_SetRenderLogicalPresentation_(X* x);
void sloth2SDL3_SetRenderDrawColor_(X* x);
void sloth2SDL3_SetRenderDrawColorFloat_(X* x);
void sloth2SDL3_RenderClear_(X* x);
void sloth2SDL3_RenderPoints_(X* x);
void sloth2SDL3_RenderLine_(X* x);
void sloth2SDL3_RenderRect_(X* x);
void sloth2SDL3_RenderFillRect_(X* x);
void sloth2SDL3_RenderFillRects_(X* x);
void sloth2SDL3_RenderPresent_(X* x);
void sloth2SDL3_RenderDebugText_(X* x);

/* SDL_filesystem.h */
void sloth2SDL3_GetBasePath_(X* x);

/* SDL_video.h */
void sloth2SDL3_GetWindowSize_(X* x);

/* SDL_pixels.h */

/* SDL_rect.h */

/* SDL_stdinc.h */
void sloth2SDL3_free_(X* x);
void sloth2SDL3_asprintfs_(X* x);
void sloth2SDL3_rand_(X* x);
void sloth2SDL3_randf_(X* x);
void sloth2SDL3_fabsf_(X* x);
void sloth2SDL3_sin_(X* x);

void sloth_bootstrap_SDL3(X* x);
