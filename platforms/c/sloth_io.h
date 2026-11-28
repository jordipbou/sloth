#ifndef SLOTH_IO_H
#define SLOTH_IO_H

#include <stdint.h>

struct sloth_VM;

/* Host input/output contract.  The hooks are host callbacks; the Forth
   words stay ordinary F primitives and call these functions. */
typedef int32_t (*SlothInput)(struct sloth_VM* x, int op);
typedef int32_t (*SlothOutput)(struct sloth_VM* x, int op, uint8_t c);

/* Input operations.  KEY_READ returns a byte, KEY_AVAILABLE a flag,
   EKEY_READ an event and EKEY_AVAILABLE a flag. */
#define SLOTH_IO_KEY_READ          0
#define SLOTH_IO_KEY_AVAILABLE     1
#define SLOTH_IO_EKEY_READ         2
#define SLOTH_IO_EKEY_AVAILABLE    3

/* Output operations.  EMIT_WRITE consumes c; EMIT_AVAILABLE returns a flag. */
#define SLOTH_IO_EMIT_WRITE        0
#define SLOTH_IO_EMIT_AVAILABLE    1

#if defined(_WIN32) || defined(WIN32)
	#define SLOTH_RETURN_KEY       13
	#define SLOTH_BACKSPACE_KEY    8
#else
	#define SLOTH_RETURN_KEY       10
	#define SLOTH_BACKSPACE_KEY    127
#endif

/* Event encoding for 32/64-bit cells, compatible with CPNBI.
   bits 0-21: key value (0-0x10FFFF characters, above that special keys)
   bits 22-29: modifiers
   bits 30-31: reserved event type
   A 16-bit CELL build would need a compact profile (for example a 9-bit
   key: 0-255 characters and 256-277 special keys, plus 7 modifier bits)
   or a CELL-sized token table; see the 16-bit SOMEDAY in proj.sloth.org. */
#define SLOTH_EVENT_VALUE_MASK     0x3FFFFF
#define SLOTH_EVENT_MOD_OFFSET     22
#define SLOTH_EVENT_MOD_MASK       0xFF
#define SLOTH_EVENT_TYPE_OFFSET    30
#define SLOTH_EVENT_TYPE_MASK      0x3

#define SLOTH_MOD_SHIFT            (1 << 0)
#define SLOTH_MOD_ALT              (1 << 1)
#define SLOTH_MOD_CTRL             (1 << 2)
#define SLOTH_MOD_SUPER            (1 << 3)
#define SLOTH_MOD_HYPER            (1 << 4)
#define SLOTH_MOD_META             (1 << 5)
#define SLOTH_MOD_CAPS_LOCK        (1 << 6)
#define SLOTH_MOD_NUM_LOCK         (1 << 7)

#define SLOTH_KEY_BASE             0x110000
#define SLOTH_KEY_UP               (SLOTH_KEY_BASE + 256)
#define SLOTH_KEY_DOWN             (SLOTH_KEY_BASE + 257)
#define SLOTH_KEY_LEFT             (SLOTH_KEY_BASE + 258)
#define SLOTH_KEY_RIGHT            (SLOTH_KEY_BASE + 259)
#define SLOTH_KEY_HOME             (SLOTH_KEY_BASE + 260)
#define SLOTH_KEY_END              (SLOTH_KEY_BASE + 261)
#define SLOTH_KEY_INSERT           (SLOTH_KEY_BASE + 262)
#define SLOTH_KEY_DELETE           (SLOTH_KEY_BASE + 263)
#define SLOTH_KEY_PAGE_UP          (SLOTH_KEY_BASE + 264)
#define SLOTH_KEY_PAGE_DOWN        (SLOTH_KEY_BASE + 265)
#define SLOTH_KEY_F1               (SLOTH_KEY_BASE + 266)
#define SLOTH_KEY_F2               (SLOTH_KEY_BASE + 267)
#define SLOTH_KEY_F3               (SLOTH_KEY_BASE + 268)
#define SLOTH_KEY_F4               (SLOTH_KEY_BASE + 269)
#define SLOTH_KEY_F5               (SLOTH_KEY_BASE + 270)
#define SLOTH_KEY_F6               (SLOTH_KEY_BASE + 271)
#define SLOTH_KEY_F7               (SLOTH_KEY_BASE + 272)
#define SLOTH_KEY_F8               (SLOTH_KEY_BASE + 273)
#define SLOTH_KEY_F9               (SLOTH_KEY_BASE + 274)
#define SLOTH_KEY_F10              (SLOTH_KEY_BASE + 275)
#define SLOTH_KEY_F11              (SLOTH_KEY_BASE + 276)
#define SLOTH_KEY_F12              (SLOTH_KEY_BASE + 277)

#define SLOTH_EVENT_KEY(e)         ((e) & SLOTH_EVENT_VALUE_MASK)
#define SLOTH_EVENT_MOD(e)         (((e) >> SLOTH_EVENT_MOD_OFFSET) & SLOTH_EVENT_MOD_MASK)
#define SLOTH_EVENT_IS_SPECIAL(e)  (SLOTH_EVENT_KEY(e) > 0x10FFFF)

#endif
