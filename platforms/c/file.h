#include"sloth.h"
#include<errno.h>

#if defined(WINDOWS)
#include <io.h>
#define F_OK 0
#define access _access
#else
#include<unistd.h>
#endif

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

/* -- File access methods ------------------------------ */

const char SLOTH_READ_ONLY[] = "r";
const char SLOTH_READ_ONLY_BIN[] = "rb";
const char SLOTH_READ_WRITE[] = "r+";
const char SLOTH_READ_WRITE_BIN[] = "rb+";
const char SLOTH_WRITE_ONLY[] = "w";
const char SLOTH_WRITE_ONLY_BIN[] = "wb";

void sloth_r_o_(X* x) {
	sloth_push(x, (CELL)SLOTH_READ_ONLY);	
}

void sloth_r_w_(X* x) {
	sloth_push(x, (CELL)SLOTH_READ_WRITE);
}

void sloth_w_o_(X* x) {
	sloth_push(x, (CELL)SLOTH_WRITE_ONLY);
}

void sloth_bin_(X* x) {
	char *fam = (char*)sloth_pop(x);
	if (fam == SLOTH_READ_ONLY) {
		sloth_push(x, (CELL)SLOTH_READ_ONLY_BIN);
	} else if (fam == SLOTH_READ_WRITE) {
		sloth_push(x, (CELL)SLOTH_READ_WRITE_BIN);
	} else if (fam == SLOTH_WRITE_ONLY) {
		sloth_push(x, (CELL)SLOTH_WRITE_ONLY_BIN);
	} else {
		sloth_throw(x, -12);
	}
}

/* -- File operations ---------------------------------- */

void sloth_create_file_(X* x) {
	FILE *fptr;
	char *fam = (char*)sloth_pop(x);
	int l = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	char buf[512];
	memcpy(buf, caddr, l);
	buf[l] = 0;
	/* Create/recreate file by opening it as write */
	fptr = fopen(buf, "w"); 
	if (fptr) {
		fclose(fptr);
		fptr = fopen(buf, fam);
		if (fptr) {
			sloth_push(x, (CELL)fptr);
			sloth_push(x, 0);
			return;
		}
	}
	sloth_push(x, 0);
	sloth_push(x, -37);
}

void sloth_open_file_(X* x) {
	FILE *fptr;
	char *fam = (char*)sloth_pop(x);
	int l = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	char buf[512];
	memcpy(buf, caddr, l);
	buf[l] = 0;
	fptr = fopen(buf, fam);
	if (fptr) {
		sloth_push(x, (CELL)fptr);
		sloth_push(x, 0);
	} else {
		sloth_push(x, 0);
		sloth_push(x, -37); /* file I/O exception */
	}
}

void sloth_close_file_(X* x) {
	sloth_push(x, fclose((FILE*)sloth_pop(x)));
}

void sloth_file_position_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	/* FIXME This will simulate a double number for now */
	sloth_push(x, ftell(fptr));
	sloth_push(x, 0);
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

void sloth_file_size_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	int pos, size;
	pos = ftell(fptr);	
	fseek(fptr, 0, SEEK_END); // seek to end of file
	size = ftell(fptr); // get current file pointer
	fseek(fptr, pos, SEEK_SET); // seek back to beginning of file
	/* FIXME This will simulate a double number for now */
	sloth_push(x, size);
	sloth_push(x, 0);
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

void sloth_reposition_file_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	uCELL udh = (uCELL)sloth_pop(x);
	uCELL udl = (uCELL)sloth_pop(x);
	fseek(fptr, udl, SEEK_SET);
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

void sloth_flush_file_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	sloth_push(x, fflush(fptr));
}

void sloth_resize_file_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	uCELL udh = (uCELL)sloth_pop(x);
	uCELL udl = (uCELL)sloth_pop(x);
	#if defined(WINDOWS)
	/* TODO Needs SetFilePointer and SetEndOfFile */
	#else
	if (ftruncate(fileno(fptr), udl)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
	#endif
}

void sloth_delete_file_(X* x) {
	int l = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	char buf[512];
	memcpy(buf, caddr, l);
	buf[l] = 0;
	sloth_push(x, !remove(buf) ? 0 : -37);
}

void sloth_rename_file_(X* x) {
	unsigned int u2 = (unsigned int)sloth_pop(x);
	char *caddr2 = (char*)sloth_pop(x);
	unsigned int u1 = (unsigned int)sloth_pop(x);
	char *caddr1 = (char*)sloth_pop(x);
	char buf2[512], buf1[512];
	memcpy(buf1, caddr1, u1);
	buf1[u1] = 0;
	memcpy(buf2, caddr2, u2);
	buf2[u2] = 0;
	sloth_push(x, !rename(buf1, buf2) ? 0 : -37);
}

void sloth_file_status_(X* x) {
	unsigned int u = (unsigned int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	char buf[512];
	memcpy(buf, caddr, u);
	buf[u] = 0;
	/* A value with implementation-defined information */
	/* about the file has to be returned. As I don't have */
	/* any clue about what time of information is expected */
	/* I just return 0. */
	sloth_push(x, 0); 
	sloth_push(x, !access(buf, F_OK) ? 0 : -37);
}

/* -- Read operations ---------------------------------- */

void sloth_read_line_(X* x) {
	char buf[1024];
	FILE *fptr = (FILE*)sloth_pop(x);
	int u1 = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	int u2, l;
	char *res;
	/* Read at most u1 characters */
	if (u1 == 0) {
		sloth_push(x, 0);
		sloth_push(x, -1);
		sloth_push(x, 0);
	} else {
		/* We need to read u1 + 1 because fgets counts the */
		/* zero at the end. */
		if (fgets(buf, u1 + 1, fptr)) {
			l = u2 = strlen(buf);
			/* Detect CR/LF to substract it from counted chars */
			if (buf[u2 - 1] == 10 || buf[u2 - 1] == 13) u2--;
			/* In case of CRLF, substract one again */
			if (buf[u2 - 1] == 10) u2--;
			memcpy(caddr, buf, l);
			sloth_push(x, u2);
			sloth_push(x, -1);
			sloth_push(x, 0);
		} else {
			sloth_push(x, 0);
			sloth_push(x, 0);
			if (feof(fptr)) {
				sloth_push(x, 0);
			} else {
				sloth_push(x, -37);
			}
		}
	}
}

void sloth_read_file_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	int u1 = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	int count = fread(caddr, suCHAR, u1, fptr);
	if (count == u1 || feof(fptr)) {
		/* File contents read succesfully */
		sloth_push(x, count);
		sloth_push(x, 0);
	} else if (ferror(fptr)) {
		sloth_push(x, count);
		sloth_push(x, -37);
	}
}

/* -- Write operations --------------------------------- */

void sloth_write_line_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	int l = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	int count = fwrite(caddr, suCHAR, l, fptr);
	fprintf(fptr, "\n");
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

void sloth_write_file_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	int l = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	int count = fwrite(caddr, suCHAR, l, fptr);
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

/* -- Loading scripts ---------------------------------- */

int sloth__is_file_included(X* x, char *a1, uCELL u1) {
	/* TODO iterate from last included file to first */
	CELL name = sloth_user_area_get(x, SLOTH_INCLUDED_FILES);
	while (name != 0) {
		CELL u2 = sloth_fetch(x, name + sCELL);
		CELL a2 = name + 2*sCELL;
		if (sloth__compare_without_case(x, (CELL)a1, u1, a2, u2)) {
			return 1;
		}
		name = sloth_fetch(x, name);
	}
	return 0;
}

void sloth_required_(X* x) {
	uCELL u = (uCELL)sloth_pop(x);
	char* caddr = (char*)sloth_pop(x);
	if (!sloth__is_file_included(x, caddr, u)) {
		sloth_push(x, (CELL)caddr);
		sloth_push(x, u);
		sloth_included_(x);
	}
}

void sloth_require_(X* x) {
	CELL name, namelen;
	sloth_push(x, 32); sloth_word_(x);
	name = sloth_pick(x, 0) + suCHAR;
	namelen = sloth_cfetch(x, sloth_pop(x));
	sloth_push(x, name);
	sloth_push(x, namelen);
	sloth_required_(x);
}

void sloth_bootstrap_file_wordset(X* x) {
	/* -- File access methods ---------------------------- */

	SLOTH_CODE("R/O", r_o);
	SLOTH_CODE("R/W", r_w);
	SLOTH_CODE("W/O", w_o);
	SLOTH_CODE("BIN", bin);

	/* -- File operations -------------------------------- */

	SLOTH_CODE("CREATE-FILE", create_file);
	SLOTH_CODE("OPEN-FILE", open_file);
	SLOTH_CODE("CLOSE-FILE", close_file);
	SLOTH_CODE("FILE-POSITION", file_position);
	SLOTH_CODE("FILE-SIZE", file_size);
	SLOTH_CODE("REPOSITION-FILE", reposition_file);
	SLOTH_CODE("FLUSH-FILE", flush_file);
	SLOTH_CODE("RESIZE-FILE", resize_file);
	SLOTH_CODE("DELETE-FILE", delete_file);
	SLOTH_CODE("RENAME-FILE", rename_file);
	SLOTH_CODE("FILE-STATUS", file_status);

	/* -- Read operations -------------------------------- */

	SLOTH_CODE("READ-LINE", read_line);
	SLOTH_CODE("READ-FILE", read_file);

	/* -- Write operations ------------------------------- */

	SLOTH_CODE("WRITE-LINE", write_line);
	SLOTH_CODE("WRITE-FILE", write_file);

	/* -- Loading scripts -------------------------------- */

	SLOTH_CODE("REQUIRED", required);
	SLOTH_CODE("REQUIRE", require);
}
