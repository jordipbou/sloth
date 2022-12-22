#include<stdio.h>
#include<string.h>

#ifdef _WIN32
  #include <conio.h>
#else
	#include <unistd.h>
	#include <termios.h>
#endif

#include "capybara.h"

#define TIB_SIZE			10*sizeof(CELL)
#define TSB_SIZE			10*sizeof(CELL)
#define DSTACK_SIZE		256*sizeof(CELL)
#define RSTACK_SIZE		256*sizeof(CELL)

typedef struct {
	CELL len;
	BYTE str[];
} STR;

typedef struct _ENTRY {
	struct _ENTRY* next;
	CELL flags;
	STR* name;
	STR* source;
	CELL clen;
	BYTE* code;
	BYTE data[];
} ENTRY;														// (WORD type is taken by Windows API)

#define F_IMMEDIATE			1

typedef struct {
	CTX ctx;
	CELL state;
	CELL flags;
	BYTE TIB[TIB_SIZE];				// Terminal input buffer
	CELL IN;
	CELL CTSB;								// Counted value of string in temporal string buffer
	BYTE TSB[TSB_SIZE];				// Temporal string buffer
	CELL DSTACK[DSTACK_SIZE];		// Data stack
	CELL RSTACK[RSTACK_SIZE];		// Return stack
	CELL PP, DP, RP;	// Parse pointer, data stack pointer, return stack pointer
	ENTRY* dict;
} SLOTH;

#define ST_INTERPRETING			0
#define ST_COMPILING				1

#define ERR_OK							0
#define ERR_UNDEFINED_WORD	1

SLOTH* init_sloth() {
	// Create capybara context
	CTX* ctx = init(10*pagesize(), 10*pagesize());
	if (!ctx) return NULL;

	SLOTH* sloth = (SLOTH*)ctx;

	sloth->state = ST_INTERPRETING;
	sloth->flags = ERR_OK;

	// Reserve enough space for TIB, DATA STACK and RETURN STACK...
	sloth->ctx.dhere += TIB_SIZE + DSTACK_SIZE + RSTACK_SIZE;
	sloth->ctx.bottom = sloth->ctx.dhere;

	sloth->IN = 0;
	sloth->DP = sloth->RP = 0;

	sloth->dict = NULL;

	return sloth;
}

void deinit_sloth(SLOTH* sloth) {
	deinit((CTX*)sloth);
}

// Error detection

#define DUF(s, n)			s->DP < n

#define RUF(s, n)			s->RP < n

// Stack operations

#define PUSH(s, v)		s->DSTACK[s->DP++] = v
#define POP(s)				s->DSTACK[--s->DP]
#define PEEK(s)				s->DSTACK[s->DP-1]

#define RPUSH(s, a)		s->RSTACK[s->RP++] = a
#define RPOP(s)				s->RSTACK[--s->RP]

// Words

void _lit(SLOTH* s) { PUSH(s, s->ctx.Lx); }

void _call(SLOTH* s) { 
	// Get from Lx address of code of word to be called
	BYTE* NEXT = (BYTE*)s->ctx.Ax;
	while (NEXT != 0) {
		NEXT = CALL(NEXT, ((CTX*)s));
		if (NEXT != 0) {
			((void (*)(CTX*))(((CTX*)s)->Fx))((CTX*)s);
		}
	}
}
void _ret(SLOTH* s) { if (s->RP == 0) s->ctx.Lx = 0; else s->ctx.Lx = RPOP(s); }

#define BINOP(s, op)		{ CELL x = POP(s); CELL y = POP(s); PUSH(s, y op x); }

// Arithmetics
void _add(SLOTH* s) { BINOP(s, +); }
void _sub(SLOTH* s) { BINOP(s, -); }
void _mul(SLOTH* s) { BINOP(s, *); }
void _div(SLOTH* s) { BINOP(s, /); }
void _mod(SLOTH* s) { BINOP(s, %); }

// Logic
void _and(SLOTH* s) { BINOP(s, &); }
void _or(SLOTH* s) { BINOP(s, |); }
void _not(SLOTH* s) { PUSH(s, ~POP(s)); }

// Stack manipulation
void _drop(SLOTH* s) { POP(s); }
void _dup(SLOTH* s) { CELL x = POP(s); PUSH(s, x); PUSH(s, x); }
void _swap(SLOTH *s) { CELL x = POP(s); CELL y = POP(s); PUSH(s, x); PUSH(s, y); }

void _2dup(SLOTH *s) { CELL x = POP(s); CELL y = POP(s); PUSH(s, y); PUSH(s, x); PUSH(s, y); PUSH(s, x); }

void _fetch(SLOTH *s) { CELL addr = POP(s); PUSH(s, *((CELL*)addr)); }
void _store(SLOTH *s) { CELL* addr = (CELL*)POP(s); *addr = POP(s); }

void _here(SLOTH *s) { PUSH(s, (CELL)s->ctx.dhere); }
void _state(SLOTH *s) { PUSH(s, (CELL)(&s->state)); }

void _allot(SLOTH* s) { allot((CTX*)s, POP(s)); }
void _align(SLOTH* s) { align((CTX*)s); }

// INPUT/OUTPUT

// Source code for getch is taken from:
// Crossline readline (https://github.com/jcwangxp/Crossline).
// It's a fantastic readline cross-platform replacement, but only getch was
// needed and there's no need to include everything else.
#ifdef _WIN32	// Windows
int _getch(void) {	fflush (stdout); return _getch(); }
#else
int _getch()
{
	char ch = 0;
	struct termios old_term, cur_term;
	fflush (stdout);
	if (tcgetattr(STDIN_FILENO, &old_term) < 0)	{ perror("tcsetattr"); }
	cur_term = old_term;
	cur_term.c_lflag &= ~(ICANON | ECHO | ISIG); // echoing off, canonical off, no signal chars
	cur_term.c_cc[VMIN] = 1;
	cur_term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &cur_term) < 0)	{ perror("tcsetattr"); }
	if (read(STDIN_FILENO, &ch, 1) < 0)	{ /* perror("read()"); */ } // signal will interrupt
	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old_term) < 0)	{ perror("tcsetattr"); }
	return ch;
}
#endif

// key ( -- char )
void _key(SLOTH* s) { PUSH(s, _getch()); }

// emit ( x -- )
void _emit(SLOTH *s) {	
	CELL K = POP(s);
	if (K == 127) printf ("\b \b"); 
	else printf ("%c", (char)K); 
}

// . ( n -- )
void _dot(SLOTH* s) {
	CELL x = POP(s);	
	printf("%ld ", x);
}

void _type(SLOTH* s) {
	CELL len = POP(s);
	BYTE* addr = (BYTE*)POP(s);
	for (CELL i = 0; i < len; i++) {
		PUSH(s, *(addr + i));
		_emit(s);
	}
}

void _accept(SLOTH* s) {
	CELL max = POP(s);
	BYTE* addr = (BYTE*)POP(s);
	for (CELL count = 0; count < max; count++) {
		_key(s);
		CELL k = POP(s);
		if (k == '\n' || k == '\r') {
			*(addr + count) = 0;
			PUSH(s, count);
			printf(" ");
			return;
		} else {
			PUSH(s, k);
			_emit(s);
			if (k == 127) {
				count-=2;
			} else {
				*(addr + count) = k;
			}
		}
	}
}

void _in(SLOTH* s) {
	PUSH(s, (CELL)(&s->IN));
}

void _source(SLOTH *s) {
	PUSH(s, (CELL)(s->TIB));
	PUSH(s, strlen(s->TIB));
}

// REFILL ( -- flag )
void _refill(SLOTH *s) {
	_source(s);	_drop(s);
	PUSH(s, TIB_SIZE);
	_accept(s); _drop(s);
	PUSH(s, 0);
	_in(s); _store(s);
	PUSH(s, -1);
}

// PARSE-NAME ( "<spaces>name<space>" -- c-addr u )
void _parse_name(SLOTH* s) {
	while(s->TIB[s->IN] == ' ') { s->IN++; }
	CELL c_addr = (CELL)(s->TIB + s->IN);
	CELL u = 0;
	while(s->TIB[s->IN] != ' ' && s->TIB[s->IN] != 0) { u++; s->IN++; }
	PUSH(s, c_addr); PUSH(s, u);
}

// STR>CSTR ( c-addr u -- c-addr )
void _to_counted_string(SLOTH* s) {
	CELL len = POP(s);
	BYTE* caddr = (BYTE*)POP(s);
	s->CTSB = len;
	for (CELL i = 0; i < len; i++) {
		s->TSB[i] = *(caddr + i);
	}
	s->TSB[len] = 0;
	PUSH(s, (CELL)(&s->CTSB));
}

#define sC								sizeof(CELL)
#define AS_CELLS(bytes)		((bytes + sC - 1) & ~(sC - 1)) / sC
#define AS_BYTES(cells)		(cells * sizeof(CELL))

ENTRY* create(SLOTH* sloth, BYTE* name, BYTE* src) {
	// Store name
	STR* n = (STR*)sloth->ctx.dhere;
	allot((CTX*)sloth, sizeof(CELL) + AS_BYTES(AS_CELLS(strlen(name))));
	n->len = strlen(name);
	strcpy(n->str, name);
	STR* s = (STR*)sloth->ctx.dhere;
	if (src != NULL) {
		// Store source code
		allot((CTX*)sloth, sizeof(CELL) + AS_BYTES(AS_CELLS(strlen(src))));
		s->len = strlen(src);
		strcpy(s->str, src);
	}
	ENTRY* w = (ENTRY*)sloth->ctx.dhere;
	allot((CTX*)sloth, sizeof(ENTRY));
	w->next = sloth->dict;
	w->flags = 0;
	w->name = n;
	w->source = src == NULL ? NULL : s;
	w->clen = 0;
	w->code = NULL;

	return w;
}

CELL compile_ret(CTX* ctx) {
	// Calls ret function that sets Lx to top of return stack (or 0)
	// and when returned here, sets NEXT address (rax) to Lx and returns
	// to C again.
	// This will be implemented in machine code without returning to C except
	// when return stack has 0 depth.
	CELL bytes = compile_cfunc(ctx, (FUNC)&_ret);
	bytes += compile_next(ctx);
	// mov rax, [rdx + <offset of Lx>]
	bytes += compile_bytes(ctx, "\x48\x8B\x42", 3);
	bytes += compile_byte(ctx, offsetof(CTX, Lx));
	bytes += compile_byte(ctx, 0xC3);
	return bytes;
}

void _create(SLOTH* s) {
	_parse_name(s);	
	_to_counted_string(s);
	_drop(s);
	BYTE* dhere = s->ctx.dhere;
	ENTRY* w = create(s, s->TSB, "");
	_align(s);
	BYTE* chere = unprotect((CTX*)s);
	if (chere) {
		CELL bytes = compile_Lx((CTX*)s, (CELL)(s->ctx.dhere));
		bytes += compile_cfunc((CTX*)s, (FUNC)&_lit);
		bytes += compile_next((CTX*)s);
		bytes += compile_ret((CTX*)s);
		if (protect((CTX*)s, chere)) {
			w->clen = bytes;
			w->code = chere;
			w->next = s->dict;
			s->dict = w;
			return;
		}
	}
	s->ctx.dhere = dhere;
}

void _colon(SLOTH* s) {
	_parse_name(s);
	_to_counted_string(s);
	_drop(s);
	ENTRY* w = create(s, s->TSB, "");
	PUSH(s, (CELL)w);
	w->code = s->ctx.chere;
	s->state = ST_COMPILING;
}

void _semicolon(SLOTH* s) {
	s->state = ST_INTERPRETING;
	ENTRY* w = (ENTRY*)POP(s);
	w->clen = s->ctx.chere - w->code;
	w->next = s->dict;
	s->dict = w;
}

ENTRY* add_cfunc(SLOTH* sloth, BYTE* name, FUNC func) {
	BYTE* dhere = sloth->ctx.dhere;
	ENTRY* w = create(sloth, name, NULL);
	BYTE* chere = unprotect((CTX*)sloth);
	if (chere) {
		CELL bytes = compile_cfunc((CTX*)sloth, (FUNC)func);
		bytes += compile_next((CTX*)sloth);
		bytes += compile_ret((CTX*)sloth);
		if (protect((CTX*)sloth, chere)) {
			w->clen = bytes;
			w->code = chere;

			w->next = sloth->dict;
			sloth->dict = w;

			return w;
		}
	}
	sloth->ctx.dhere = dhere;
	return NULL;
}

void dump_words(SLOTH* s) {
	ENTRY* w = s->dict;
	while (w != NULL) {
		printf("%s ", w->name->str);
		w = w->next;
	}
	printf("\n");
}

void _dump_stack(SLOTH* s) {
	printf(" <%ld> ", s->DP);
	for (CELL i = 0; i < s->DP; i++) printf("%ld ", s->DSTACK[i]);
}

ENTRY* find(SLOTH* sloth, BYTE* name) {
	ENTRY* w = sloth->dict;
	while (w != NULL && strcmp(w->name->str, name)) {
		w = w->next;
	}
	return w;
}

// FIND ( c-addr -- c-addr 0 | xt 1 | xt -1 )
void _find(SLOTH* s) {
	STR* str = (STR*)POP(s);	
	ENTRY* w = find(s, str->str);
	if (w == NULL) {
		PUSH(s, (CELL)str);
		PUSH(s, 0);
	} else if (w->flags & F_IMMEDIATE) {
		PUSH(s, (CELL)w);
		PUSH(s, 1);
	} else {
		PUSH(s, (CELL)w);
		PUSH(s, -1);
	}
}

void eval_word(SLOTH* sloth, BYTE* name) {
	ENTRY* w = find(sloth, name);
	if (w) {
		BYTE* NEXT = w->code;
		while (NEXT != 0) {
			NEXT = CALL(NEXT, ((CTX*)sloth));
			if (NEXT != 0) {
				((void (*)(CTX*))(((CTX*)sloth)->Fx))((CTX*)sloth);
			}
		}
	} else {
		sloth->flags |= ERR_UNDEFINED_WORD;
	}
}

// EXECUTE ( i*x xt -- j*x )
void _execute(SLOTH* sloth) {
	BYTE* NEXT = ((ENTRY*)POP(sloth))->code;
	while (NEXT != 0) {
		NEXT = CALL(NEXT, ((CTX*)sloth));
		if (NEXT != 0) {
			((void (*)(CTX*))(((CTX*)sloth)->Fx))((CTX*)sloth);
		}
	}
}

// TODO: Call C function _call to push address to return stack and the rest
void _compile(SLOTH* s) {
	_parse_name(s);
	_to_counted_string(s);
	_drop(s);
	ENTRY* w = find(s, s->TSB);
	BYTE* chere = unprotect((CTX*)s);
	if (chere) {
		CELL bytes = compile_cfunc((CTX*)s, (FUNC)&_call);
		bytes += compile_Ax((CTX*)s, (CELL)w->code);
		bytes += compile_next((CTX*)s);
		bytes += compile_ret((CTX*)s);
		if (protect((CTX*)s, chere)) {
			w->clen = bytes;
			w->code = chere;

			w->next = s->dict;
			s->dict = w;
		}
	}
}
