#include"sloth.h"
#include<errno.h>

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

/* -- File access methods ------------------------------ */

const char SLOTH_READ_ONLY[] = "r";
const char SLOTH_READ_ONLY_BIN[] = "rb";
const char SLOTH_READ_WRITE[] = "w+";
const char SLOTH_READ_WRITE_BIN[] = "wb+";
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

/* TODO All positions and sizes are in double numbers!!! */

void sloth_file_position_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	sloth_push(x, ftell(fptr));
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
	sloth_push(x, size);
	if (ferror(fptr)) {
		sloth_push(x, -37);
	} else {
		sloth_push(x, 0);
	}
}

void sloth_reposition_file_(X* x) {
	FILE *fptr = (FILE*)sloth_pop(x);
	unsigned int ud = (unsigned int)sloth_pop(x);
	fseek(fptr, ud, SEEK_SET);
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

/* -- Read operations ---------------------------------- */

void sloth_read_line_(X* x) {
	char buf[1024];
	FILE *fptr = (FILE*)sloth_pop(x);
	int u1 = (int)sloth_pop(x);
	char *caddr = (char*)sloth_pop(x);
	int u2;
	char *res;
	/* Reads u1 characters + (maybe)two */
	/* implementation-defined line-terminating */
	/* characters + null terminator */
	if (fgets(buf, u1 + 3, fptr)) {
		/* TODO Remove the line terminator characters */
		/* from the count */
		u2 = strlen(buf);
		/* Copy without the null terminator character */
		memcpy(caddr, buf, u2);
		sloth_push(x, u2);
		sloth_push(x, -1);
		sloth_push(x, 0);
	} else {
		sloth_push(x, 0);
		sloth_push(x, 0);
		if (feof(fptr)) sloth_push(x, 0);
		else sloth_push(x, -37);
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
	/* TODO Add the implementation dependent line terminator */
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

	/* -- Read operations -------------------------------- */

	SLOTH_CODE("READ-LINE", read_line);
	SLOTH_CODE("READ-FILE", read_file);

	/* -- Write operations ------------------------------- */

	SLOTH_CODE("WRITE-LINE", write_line);
	SLOTH_CODE("WRITE-FILE", write_file);
}
