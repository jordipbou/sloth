#include "file.h"

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

/* -- File access methods ------------------------------ */

const char SLOTH_READ_ONLY[] = "r";
const char SLOTH_READ_ONLY_BIN[] = "rb";
const char SLOTH_READ_WRITE[] = "r+";
const char SLOTH_READ_WRITE_BIN[] = "rb+";
const char SLOTH_WRITE_ONLY[] = "w";
const char SLOTH_WRITE_ONLY_BIN[] = "wb";

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

void sloth_r_slash_o_(X* x) {
	sloth_push(x, (CELL)SLOTH_READ_ONLY);	
}

void sloth_r_slash_w_(X* x) {
	sloth_push(x, (CELL)SLOTH_READ_WRITE);
}

void sloth_w_slash_o_(X* x) {
	sloth_push(x, (CELL)SLOTH_WRITE_ONLY);
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

void sloth_file_size_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	int pos, size;
	pos = ftell(fptr);	
	fseek(fptr, 0, SEEK_END); /* seek to end of file */
	size = ftell(fptr); /* get current file pointer */
	fseek(fptr, pos, SEEK_SET); /* seek back to beginning of file */
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
	/* If the size of a long allows using two cells as a
		 double number, combine them to represent the offset */
#if ULONG_MAX / UINTPTR_MAX >= UINTPTR_MAX
	uCELL udh = (uCELL)sloth_pop(x);
	uCELL udl = (uCELL)sloth_pop(x);
	long offset = (long)(((unsigned long)udh << CELL_BITS)
	                   | (unsigned long)udl);
#else
	/* in any other case, just ignore the high part */
	long offset;
	(void)sloth_pop(x);
	offset = (long)sloth_pop(x);
#endif
	fseek(fptr, offset, SEEK_SET);
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
	int fd = fileno(fptr);
	/* If the size of a long allows using two cells as a
		 double number, combine them to represent the offset */
#if ULONG_MAX / UINTPTR_MAX >= UINTPTR_MAX
	uCELL udh = (uCELL)sloth_pop(x);
	uCELL udl = (uCELL)sloth_pop(x);
	long size = (long)(((unsigned long)udh << CELL_BITS)
	                 | (unsigned long)udl);
#else
	/* in any other case, just ignore the high part */
	long size;
	(void)sloth_pop(x);
	size = (long)sloth_pop(x);
#endif
	if (fd < 0) {
		sloth_push(x, -37);
	} else {
#if defined(WINDOWS)
		sloth_push(x, _chsize(fd, size) ? -37 : 0);
#else
		sloth_push(x, ftruncate(fd, size) ? -37 : 0);
#endif
	}
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
	(void)fwrite(caddr, suCHAR, l, fptr);
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
	(void)fwrite(caddr, suCHAR, l, fptr);
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

/* -- Loading scripts ---------------------------------- */

int sloth__is_file_included(X* x, char *a1, uCELL u1) {
	/* TODO iterate from last included file to first */
	CELL name = sloth_user_get(x, SLOTH_INCLUDED_FILES);
	while (name != 0) {
		CELL u2 = sloth_fetch(x, name + sCELL);
		CELL a2 = name + 2*sCELL;
		if (sloth__compare(x, (CELL)a1, u1, a2, u2)) {
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
	CELL addr;
	sloth_push(x, 32); sloth_word_(x);
	addr = sloth_pop(x);
	sloth_push(x, addr + suCHAR);
	sloth_push(x, sloth_c_fetch(x, addr));
	sloth_required_(x);
}

/* == Bootstrapping ==================================== */

void sloth_bootstrap_file_word_set(X* x) {
	
	/* -- File access methods ---------------------------- */

	SLOTH_CODE("BIN", bin);
	SLOTH_CODE("R/O", r_slash_o);
	SLOTH_CODE("R/W", r_slash_w);
	SLOTH_CODE("W/O", w_slash_o);

	/* -- File operations -------------------------------- */

	SLOTH_CODE("CREATE-FILE", create_file);
	SLOTH_CODE("OPEN-FILE", open_file);
	SLOTH_CODE("CLOSE-FILE", close_file);
	SLOTH_CODE("FILE-SIZE", file_size);
	SLOTH_CODE("REPOSITION-FILE", reposition_file);
	SLOTH_CODE("FLUSH-FILE", flush_file);
	SLOTH_CODE("RESIZE-FILE", resize_file);
	SLOTH_CODE("DELETE-FILE", delete_file);
	SLOTH_CODE("RENAME-FILE", rename_file);
	SLOTH_CODE("FILE-STATUS", file_status);

	/* -- Read operations -------------------------------- */

	SLOTH_CODE("READ-FILE", read_file);

	/* -- Write operations ------------------------------- */

	SLOTH_CODE("WRITE-LINE", write_line);
	SLOTH_CODE("WRITE-FILE", write_file);

	/* -- Loading scripts -------------------------------- */

	SLOTH_CODE("REQUIRED", required);
	SLOTH_CODE("REQUIRE", require);
}
