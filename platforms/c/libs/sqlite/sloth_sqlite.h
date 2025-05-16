/* TODO Use exceptions for errors received from sqlite3? */

#include"sloth.h"
#include"sqlite3.h"

#define SLOTH2SQLITE_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2sqlite_##f##_));

void sloth2sqlite_libversion_(X* x) {
	sloth_push(x, (CELL)SQLITE_VERSION);
	sloth_push(x, strlen(SQLITE_VERSION));
}

void sloth2sqlite_open_(X* x) {
	char buf[255];
	int err;
	sqlite3 *db;
	CELL l = sloth_pop(x);
	char *filename = (char *)sloth_pop(x);
	if (filename[l] != 0) {
		memcpy(buf, filename, l);
		buf[l] = 0;
		filename = buf;
	}
	err = sqlite3_open(filename, &db);
	if (err == SQLITE_OK) {
		sloth_push(x, (CELL)db);
	} else {
		sloth_throw(x, -10000 - err);
	}
}

void sloth2sqlite_prepare_v2_(X* x) {
	int err;
	sqlite3_stmt *stmt;
	sqlite3 *db = (sqlite3*)sloth_pop(x);
	int nByte = (int)sloth_pop(x);
	char *zSql = (char*)sloth_pop(x);
	err = sqlite3_prepare_v2(db, zSql, nByte, &stmt, 0);
	if (err == SQLITE_OK) {
		sloth_push(x, (CELL)stmt);
	} else {
		sloth_throw(x, -10000 - err);
	}
}

void sloth2sqlite_step_(X* x) {
	sqlite3_stmt *stmt = (sqlite3_stmt*)sloth_pop(x);
	sloth_push(x, sqlite3_step(stmt));
}

void sloth2sqlite_column_text_(X* x) {
	const char *res;
	int icol = (int)sloth_pop(x);
	sqlite3_stmt *stmt = (sqlite3_stmt*)sloth_pop(x);
	res = sqlite3_column_text(stmt, icol);
	sloth_push(x, (CELL)res);
	sloth_push(x, strlen(res));
}

void sloth_bootstrap_sqlite(X* x) {
	/* Create SQLITE wordlist */

	SLOTH2SQLITE_CODE("LIBVERSION", libversion);
	SLOTH2SQLITE_CODE("OPEN", open);
	SLOTH2SQLITE_CODE("PREPARE", prepare_v2);
	SLOTH2SQLITE_CODE("STEP", step);
	SLOTH2SQLITE_CODE("COLUMN-TEXT", column_text);

	/* Add constants here or in a 4th file */
}
