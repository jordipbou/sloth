/* ---------------------------------------------------- */
/* ------------------ SLOTH Forth --------------------- */
/* ---------------------------------------------------- */

#include<stdint.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>

/* ---------------------------------------------------- */
/* ---------------- Virtual machine ------------------- */
/* ---------------------------------------------------- */
/* This is the reference implementation of the SLOTH    */
/* Virtual Machine.                                     */
/* ---------------------------------------------------- */
/* This API defines how the virtual machine works and   */
/* allow access to its internals from the host.         */
/* ---------------------------------------------------- */
/* It uses a table of primitives (C functions that can  */
/* be called from Forth) that the bootstrapped          */
/* programming language can use to interact with the    */
/* virtual machine.                                     */
/* ---------------------------------------------------- */

typedef int8_t CHAR;
typedef intptr_t CELL;

#define sCELL sizeof(CELL)
#define sCHAR sizeof(CHAR)

/* Predefined sizes */
/* TODO: they should be modifiable before context creation */
/* or before including this file */
#define STACK_SIZE 64
#define RETURN_STACK_SIZE 64
#define DSIZE 65536
#define PSIZE 256

struct VM;
typedef void (*F)(struct VM*);

typedef struct PRIMITIVES {
	CELL sz;
	CELL last;
	F *p;
} P;

typedef struct VM { 
	CELL s[STACK_SIZE], sp;
	CELL r[RETURN_STACK_SIZE], rp;
	CELL ip;
	CELL d, sz;	/* Dict base address, dict size */

	/* Jump buffers used for exceptions */
	jmp_buf jmpbuf[8];
	int jmpbuf_idx;

	/* Pointer to array of primitives */
	P *p;
} X;

void init(X* x, CELL d, CELL sz) { 
	x->sp = 0; 
	x->rp = 0; 
	x->ip = -1; 
	x->d = d;
	x->sz = sz;

	x->jmpbuf_idx = -1;
}

/* Data stack */

void push(X* x, CELL v) { x->s[x->sp] = v; x->sp++; }
CELL pop(X* x) { x->sp--; return x->s[x->sp]; }
CELL pick(X* x, CELL a) { return x->s[x->sp - a - 1]; }

/* Return stack */

void rpush(X* x, CELL v) { x->r[x->rp] = v; x->rp++; }
CELL rpop(X* x) { x->rp--; return x->r[x->rp]; }
CELL rpick(X* x, CELL a) { return x->r[x->rp - a - 1]; }

/* Memory */

/* 
STORE/FETCH/CSTORE/cfetch work on absolute address units,
not just inside SLOTH dictionary (memory block).
*/
void store(X* x, CELL a, CELL v) { *((CELL*)a) = v; }
CELL fetch(X* x, CELL a) { return *((CELL*)a); }
void cstore(X* x, CELL a, CHAR v) { *((CHAR*)a) = v; }
CHAR cfetch(X* x, CELL a) { return *((CHAR*)a); }

/*
The next two macros allow transforming from relative to
absolute addresses.
*/
CELL to_abs(X* x, CELL a) { return (CELL)(x->d + a); }
CELL to_rel(X* x, CELL a) { return a - x->d; }

/* Inner interpreter */

CELL op(X* x) { 
	CELL o = fetch(x, to_abs(x, x->ip));
	x->ip += sCELL;
	return o; 
}

void do_prim(X* x, CELL p) { 
	(x->p->p[-1 - p])(x); 
}

void call(X* x, CELL q) { 
	if (x->ip >= 0) rpush(x, x->ip); 
	x->ip = q; 
}

void execute(X* x, CELL q) { 
	if (q < 0) do_prim(x, q); 
	else call(x, q); 
}

void inner(X* x) { 
	CELL t = x->rp;
	while (t <= x->rp && x->ip >= 0) { 
		execute(x, op(x));
	} 
}

void eval(X* x, CELL q) { 
	execute(x, q); 
	if (q > 0) inner(x); 
}

/* Exceptions */

void catch(X* x, CELL q) {
	int tsp = x->sp;
	int trp = x->rp;
	int tip = x->ip;
	int e;

	if (!(e = setjmp(x->jmpbuf[x->jmpbuf_idx++]))) {
		eval(x, q);
		push(x, 0);
	} else {
		x->sp = tsp;
		x->rp = trp;
		x->ip = tip;
		push(x, (CELL)e);
	}

	x->jmpbuf_idx--;
}

void throw(X* x, CELL e) {
	longjmp(x->jmpbuf[x->jmpbuf_idx], (int)e);
}

/* Loading and saving already bootstrapped images */

void load_image(X* x, char* filename) {
	FILE *f = fopen(filename, "rb");
	CELL sz;
	if (f) {
		fseek(f, 0L, SEEK_END);
		sz = ftell(f);
		rewind(f);
		if (x->sz < sz) { return; /* TODO: Error */ }
		fread((void*)x->d, 1, sz, f);
		fclose(f);
	} else {
		return; /* TODO: Error */
	}
}

void save_image(X* x, char* filename) {
	FILE *f = fopen(filename, "wb");
	if (f) {
		fwrite((void*)x->d, 1, x->sz, f);
		fclose(f);
	} else {
		return; /* TODO: Error */
	}
}

/* ---------------------------------------------------- */
/* -- Forth Kernel ------------------------------------ */
/* ---------------------------------------------------- */

/* -- Constants --------------------------------------- */

/* Relative addresses of variables accessed both from C */
/* and Forth. */

#define HERE					0	
#define BASE					sCELL
#define LATEST				2*sCELL
#define STATE					3*sCELL
#define IBUF					4*sCELL
#define IPOS					5*sCELL
#define ILEN					6*sCELL
#define SOURCE_ID			7*sCELL
#define DOES					8*sCELL

/* XTs of primitives that need to be called from C and  */
/* Forth code. */

#define EXIT					-1
#define LIT						-2
#define RIP						-3
#define COMPILE				-4

/* -- Helpers ----------------------------------------- */

/* Setting and getting variables (cell and char sized) */

void set(X* x, CELL a, CELL v) { store(x, to_abs(x, a), v); }
CELL get(X* x, CELL a) { return fetch(x, to_abs(x, a)); }

void cset(X* x, CELL a, CHAR v) { cstore(x, to_abs(x, a), v); }
CHAR cget(X* x, CELL a) { return cfetch(x, to_abs(x, a)); }

/* Memory management */

CELL here(X* x) { return get(x, HERE); }
void allot(X* x, CELL v) { set(x, HERE, get(x, HERE) + v); }
void align(X* x) { 
	set(x, HERE, (get(x, HERE) + (sCELL - 1)) & ~(sCELL - 1)); 
}

/* Compilation */

void comma(X* x, CELL v) { set(x, here(x), v); allot(x, sCELL); }
void ccomma(X* x, CHAR v) { set(x, here(x), v); allot(x, sCHAR); }

void compile(X* x, CELL xt) { comma(x, xt); }
void literal(X* x, CELL n) { comma(x, LIT); comma(x, n); }

/* Headers */

/* Header structure: */
/* Link CELL					@ NT */
/* XT CELL						@ NT + sCELL */
/* Wordlist CELL			@ NT + 2*sCELL */
/* Flags CHAR					@ NT + 3*sCELL */
/* Namelen CHAR				@ NT + 3*sCELL + sCHAR */
/* Name CHAR*namelen	@ NT + 3*sCELL + 2*sCHAR */

CELL header(X* x, CELL n, CELL l) {
	CELL w, i;
	align(x);
	w = here(x); /* NT address */
	comma(x, get(x, LATEST)); /* Store link to latest */
	set(x, LATEST, w); /* Set NT as latest */
	comma(x, 0); /* Reserve space for XT */
	comma(x, 1); /* Store wordlist (default wordlist id: 1) */
	ccomma(x, 0); /* Flags (default flags: 0) */
	ccomma(x, l); /* Name length */
	for (i = 0; i < l; i++) ccomma(x, fetch(x, n + i)); /* Name */
	align(x); /* Align XT address */
	set(x, w + sCELL, here(x));
	return w;
}

CELL get_link(X* x, CELL w) { return get(x, w); }

CELL get_xt(X* x, CELL w) { return get(x, w + sCELL); }
void set_xt(X* x, CELL w, CELL xt) { set(x, w + sCELL, xt); }

CELL get_wordlist(X* x, CELL w) { return get(x, w + 2*sCELL); }

CHAR get_flags(X* x, CELL w) { return cget(x, w + 3*sCELL); }
CELL has_flag(X* x, CELL w, CELL v) { return get_flags(x, w) & v; }

CHAR get_namelen(X* x, CELL w) { 
	return cget(x, w + 3*sCELL + sCHAR); 
}
CELL get_name_addr(X* x, CELL w) { 
	return to_abs(x, w + 3*sCELL + 2*sCHAR); 
}

/* Setting flags */

#define HIDDEN				1
#define IMMEDIATE			2
#define INSTANT				4

void set_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 3*sCELL, get_flags(x, w) | v); 
}
void unset_flag(X* x, CELL w, CHAR v) { 
	cset(x, w + 3*sCELL, get_flags(x, w) & ~v); 
}

void set_hidden(X* x) { set_flag(x, get(x, LATEST), HIDDEN); }
void _immediate(X* x) { set_flag(x, get(x, LATEST), IMMEDIATE); }

void unset_hidden(X* x) { unset_flag(x, get(x, LATEST), HIDDEN); }


/* -- Primitives -------------------------------------- */

void _exit(X* x) { x->ip = (x->rp > 0) ? rpop(x) : -1; }
void _lit(X* x) { push(x, op(x)); }
void _rip(X* x) { push(x, to_abs(x, x->ip) + op(x) - sCELL); }

void _compile(X* x) { compile(x, pop(x)); }

/* Commands that can help you start or end work sessions */

void _environment_q(X* x) { 
	CELL u = pop(x);
	CHAR *s = (CHAR *)pop(x);
	/*
	if (compare_no_case(x, s, u, "/COUNTED-STRING")) {
		push(x, COUNTED_STRING_MAX_SIZE);
	} else if (compare_no_case(x, s, u, "/HOLD")) {
		push(x, NUMERIC_OUTPUT_STRING_BUFFER_SIZE);
	} else if (compare_no_case(x, s, u, "/PAD")) {
		push(x, SCRATCH_AREA_SIZE);
	} else if (compare_no_case(x, s, u, "ADDRESS-UNIT-BITS")) {
		push(x, BITS_PER_ADDRESS_UNIT);
	} else if (compare_no_case(x, s, u, "FLOORED")) {
		push(x, FLOORED_DIVISION_AS_DEFAULT);
	} else if (compare_no_case(x, s, u, "MAX-CHAR")) {
		push(x, MAX_VALUE_FOR_CHAR);
	} else if (compare_no_case(x, s, u, "MAX-D")) {
		push(x, LARGEST_USABLE_DOUBLE_NUMBER);
	} else if (compare_no_case(x, s, u, "MAX-N")) {
		push(x, LARGEST_USABLE_SIGNED_INTEGER);
	} else if (compare_no_case(x, s, u, "MAX-U")) {
		push(x, LARGEST_USABLE_UNSIGNED_INTEGER);
	} else if (compare_no_case(x, s, u, "MAX-UD")) {
		push(x, LARGEST_USABLE_UNSIGNED_DOUBLE_NUMBER);
	} else if (compare_no_case(x, s, u, "RETURN-STACK-CELLS")) {
		push(x, RETURN_STACK_SIZE);
	} else if (compare_no_case(x, s, u, "STACK-CELLS")) {
		push(x, STACK_SIZE);
	}
	*/
}
void _unused(X* x) { push(x, x->sz - get(x, HERE)); }
void _words(X* x) { /* TODO */ }
void _bye(X* x) { exit(0); }
void _time_and_date(X* x) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	push(x, tm->tm_sec);
	push(x, tm->tm_min);
	push(x, tm->tm_hour);
	push(x, tm->tm_mday);
	push(x, tm->tm_mon);
	push(x, tm->tm_year);
}

/* Commands to inspect memory, debug & view code */

void _depth(X* x) { push(x, x->sp); }
/* BLOCK EXT */ void _list(X* x) { /* Not implemented */ }
void _dump(X* x) { /* TODO */ }
void _question(X* x) { printf("%ld ", fetch(x, pop(x))); }
void _dot_s(X* x) { 
	CELL i;
	printf("<%ld> ", x->sp);
	for (i = 0; i < x->sp; i++) {
		printf("%ld ", x->s[i]);
	}
}
void _see(X* x) { /* TODO */ }

/* Commands that change compilation & interpretation settings */

void _base(X* x) { push(x, to_abs(x, BASE)); }
void _decimal(X* x) { set(x, BASE, 10); }
void _forget(X* x) { /* Obsolescent, don't implement */ }
void _hex(X* x) { set(x, BASE, 16); }
void _marker(X* x) { /* TODO */ }

void _also(X* x) { /* TODO */ }
void _definitions(X* x) { /* TODO */ }
void _forth(X* x) { /* TODO */ }
void _forth_wordlist(X* x) { /* TODO */ }
void _get_current(X* x) { /* TODO */ }
void _get_order(X* x) { /* TODO */ }
void _only(X* x) { /* TODO */ }
void _order(X* x) { /* TODO */ }
void _previous(X* x) { /* TODO */ }
void _set_current(X* x) { /* TODO */ }
void _set_order(X* x) { /* TODO */ }
void _wordlist(X* x) { /* TODO */ }

void _assembler(X* x) { /* TODO */ }
void _editor(X* x) { /* TODO */ }

/* Source code preprocessing, interpreting & auditing commands */

void _dot_paren(X* x) { /* TODO */ }
void _include_file(X* x) { /* TODO */ }
void _included(X* x) { /* TODO */ }
void _load(X* x) { /* TODO */ }
void _thru(X* x) { /* TODO */ }
void _bracket_if(X* x) { /* TODO */ }
void _bracket_else(X* x) { /* TODO */ }
void _bracket_then(X* x) { /* TODO */ }

/* Comment-introducing operations */

void _backslash(X* x) { /* TODO */ }
void _paren(X* x) { /* TODO */ }

/* Dynamic memory operations */

void _allocate(X* x) { /* TODO */ }
void _free(X* x) { /* TODO */ }
void _resize(X* x) { /* TODO */ }

/* String operations */

void _convert(X* x) { /* TODO */ }
void _count(X* x) { /* TODO */ }
void _erase(X* x) { /* TODO */ }
void _fill(X* x) { /* TODO */ }
void _hold(X* x) { /* TODO */ }
void _move(X* x) { /* TODO */ }
void _to_number(X* x) { /* TODO */ }
void _less_number_sign(X* x) { /* TODO */ }
void _number_sign_greater(X* x) { /* TODO */ }
void _number_sign(X* x) { /* TODO */ }
void _number_sign_s(X* x) { /* TODO */ }
void _sign(X* x) { /* TODO */ }

void _blank(X* x) { /* TODO */ }
void _cmove(X* x) { /* TODO */ }
void _cmove_up(X* x) { /* TODO */ }
void _compare(X* x) { /* TODO */ }
void _search(X* x) { /* TODO */ }
void _slash_string(X* x) { /* TODO */ }
void _dash_trailing(X* x) { /* TODO */ }

void _to_float(X* x) { /* TODO */ }
void _represent(X* x) { /* TODO */ }

/* Disk input/output operations using files or block buffers */

void _block(X* x) { /* TODO */ }
void _buffer(X* x) { /* TODO */ }
void _empty_buffers(X* x) { /* TODO */ }
void _flush(X* x) { /* TODO */ }
void _save_buffers(X* x) { /* TODO */ }
void _scr(X* x) { /* TODO */ }
void _update(X* x) { /* TODO */ }

void _bin(X* x) { /* TODO */ }
void _close_file(X* x) { /* TODO */ }
void _create_file(X* x) { /* TODO */ }
void _delete_file(X* x) { /* TODO */ }
void _file_position(X* x) { /* TODO */ }
void _file_size(X* x) { /* TODO */ }
void _file_status(X* x) { /* TODO */ }
void _flush_file(X* x) { /* TODO */ }
void _open_file(X* x) { /* TODO */ }
void _r_o(X* x) { /* TODO */ }
void _r_w(X* x) { /* TODO */ }
void _read_file(X* x) { /* TODO */ }
void _read_line(X* x) { /* TODO */ }
void _rename_file(X* x) { /* TODO */ }
void _reposition_file(X* x) { /* TODO */ }
void _resize_file(X* x) { /* TODO */ }
void _w_o(X* x) { /* TODO */ }
void _write_file(X* x) { /* TODO */ }
void _write_line(X* x) { /* TODO */ }

/* Helpers to add primitives to the dictionary */

CELL primitive(X* x, F f) { 
	x->p->p[x->p->last++] = f; 
	return 0 - x->p->last; 
}

CELL code(X* x, char* name, CELL xt) {
	CELL w = header(x, (CELL)name, strlen(name));
	set_xt(x, w, xt);
	return xt; 
}

void bootstrap(X* x) {
	comma(x, 0); /* HERE */
	comma(x, 0); /* LATEST */
	comma(x, 0); /* STATE */
	comma(x, 0); /* IBUF */
	comma(x, 0); /* IPOS */
	comma(x, 0); /* ILEN */
	comma(x, 0); /* SOURCE_ID */
	comma(x, 0); /* DOES */

	/* Required order primitives */

	code(x, "EXIT", primitive(x, &_exit));
	code(x, "LIT", primitive(x, &_lit));
	code(x, "RIP", primitive(x, &_rip));
	code(x, "COMPILE,", primitive(x, &_compile));

	/* Commands that can help you start or end work sessions */

	code(x, "ENVIRONMENT?", primitive(x, &_environment_q));
	code(x, "UNUSED", primitive(x, &_unused));
	code(x, "WORDS", primitive(x, &_words));
	code(x, "BYE", primitive(x, &_bye));
	code(x, "TIME&DATE", primitive(x, &_time_and_date));

	/* Commands to inspect memory, debug & view code */

	code(x, "DEPTH", primitive(x, &_depth));
	code(x, "LIST", primitive(x, &_list));
	code(x, "DUMP", primitive(x, &_dump));
	code(x, "?", primitive(x, &_question));
	code(x, ".S", primitive(x, &_dot_s));
	code(x, "SEE", primitive(x, &_see));

	/* Commands that change compilation & interpretation settings */

	code(x, "BASE", primitive(x, &_base));
	code(x, "DECIMAL", primitive(x, &_decimal));
	code(x, "FORGET", primitive(x, &_forget));
	code(x, "HEX", primitive(x, &_hex));
	code(x, "MARKER", primitive(x, &_marker));

	code(x, "ALSO", primitive(x, &_also));
	code(x, "DEFINITIONS", primitive(x, &_definitions));
	code(x, "FORTH", primitive(x, &_forth));
	code(x, "FORTH-WORDLIST", primitive(x, &_forth_wordlist));
	code(x, "GET-CURRENT", primitive(x, &_get_current));
	code(x, "GET-ORDER", primitive(x, &_get_order));
	code(x, "ONLY", primitive(x, &_only));
	code(x, "ORDER", primitive(x, &_order));
	code(x, "PREVIOUS", primitive(x, &_previous));
	code(x, "SET-CURRENT", primitive(x, &_set_current));
	code(x, "SET-ORDER", primitive(x, &_set_order));
	code(x, "WORDLIST", primitive(x, &_wordlist));
	
	code(x, "ASSEMBLER", primitive(x, &_assembler));
	code(x, "EDITOR", primitive(x, &_editor));

	/* Source code preprocessing, interpreting & auditing commands */

	code(x, ".(", primitive(x, &_dot_paren));
	code(x, "INCLUDE-FILE", primitive(x, &_include_file));
	code(x, "INCLUDED", primitive(x, &_included));
	code(x, "LOAD", primitive(x, &_load));
	code(x, "THRU", primitive(x, &_thru));
	code(x, "[IF]", primitive(x, &_bracket_if));
	code(x, "[ELSE]", primitive(x, &_bracket_else));
	code(x, "[THEN]", primitive(x, &_bracket_then));

	/* Comment-introducing operations */

	code(x, "\\", primitive(x, &_backslash));
	code(x, "(", primitive(x, &_paren));

	/* Dynamic memory operations */

	code(x, "ALLOCATE", primitive(x, &_allocate));
	code(x, "FREE", primitive(x, &_free));
	code(x, "RESIZE", primitive(x, &_resize));

	/* String operations */

	code(x, "CONVERT", primitive(x, &_convert));
	code(x, "COUNT", primitive(x, &_count));
	code(x, "ERASE", primitive(x, &_erase));
	code(x, "FILL", primitive(x, &_fill));
	code(x, "HOLD", primitive(x, &_hold));
	code(x, "MOVE", primitive(x, &_move));
	code(x, ">NUMBER", primitive(x, &_to_number));
	code(x, "<#", primitive(x, &_less_number_sign));
	code(x, "#>", primitive(x, &_number_sign_greater));
	code(x, "#", primitive(x, &_number_sign));
	code(x, "#S", primitive(x, &_number_sign_s));
	code(x, "SIGN", primitive(x, &_sign));
	code(x, "BLANK", primitive(x, &_blank));
	code(x, "CMOVE", primitive(x, &_cmove));
	code(x, "CMOVE>", primitive(x, &_cmove_up));
	code(x, "COMPARE", primitive(x, &_compare));
	code(x, "SEARCH", primitive(x, &_search));
	code(x, "/STRING", primitive(x, &_slash_string));
	code(x, "-TRAILING", primitive(x, &_dash_trailing));
	code(x, ">FLOAT", primitive(x, &_to_float));
	code(x, "REPRESENT", primitive(x, &_represent));

}

/* ---------------------------------------------------- */
/* -- main -------------------------------------------- */
/* ---------------------------------------------------- */

int main() {
	X* x = malloc(sizeof(X));
	x->p = malloc(sizeof(P));
	x->p->p = malloc(sizeof(F) * PSIZE);
	x->p->last = 0;
	x->p->sz = PSIZE;
	x->d = (CELL)malloc(DSIZE);
	init(x, x->d, DSIZE);

	set(x, 0, 17);
	push(x, to_abs(x, 0));
	_question(x);
	push(x, to_abs(x, 0));
	_question(x);
	push(x, 11);
	push(x, 13);
	_dot_s(x);
	_dot_s(x);

	free((void*)x->d);
	free(x->p);
	free(x);
}
