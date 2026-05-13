#define CPNBI_MOD_NONE				0
#define CPNBI_MOD_SHIFT				1000
#define CPNBI_MOD_CTRL				2000
#define CPNBI_MOD_ALT					4000

#define CPNBI_KEY_NUL					0

#define CPNBI_KEY_BACKSPACE		8
#define CPNBI_KEY_TAB					9
#define CPNBI_KEY_ENTER				13
#define CPNBI_KEY_ESCAPE			27 

#define CPNBI_KEY_UP					256
#define CPNBI_KEY_DOWN				257
#define CPNBI_KEY_LEFT				258
#define CPNBI_KEY_RIGHT				259

#define CPNBI_KEY_HOME				260
#define CPNBI_KEY_END					261
#define CPNBI_KEY_INSERT			262
#define CPNBI_KEY_DELETE			263
#define CPNBI_KEY_PAGE_UP			264
#define CPNBI_KEY_PAGE_DOWN		265

#define CPNBI_KEY_F1					266
#define CPNBI_KEY_F2					267
#define CPNBI_KEY_F3					268
#define CPNBI_KEY_F4					269
#define CPNBI_KEY_F5					270
#define CPNBI_KEY_F6					271
#define CPNBI_KEY_F7					272
#define CPNBI_KEY_F8					273
#define CPNBI_KEY_F9					274
#define CPNBI_KEY_F10					275
#define CPNBI_KEY_F11					276
#define CPNBI_KEY_F12					277

void cpnbi_init();
int cpnbi_is_char_available();
int cpnbi_is_event_available();
int cpnbi_get_char();
int cpnbi_get_event();
void cpnbi_shutdown();
