/* TODO Add the different column functions */
/* TODO Add bind function (and reset?) */

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

void sloth2sqlite_finalize_(X* x) {
	int err = sqlite3_finalize((sqlite3_stmt*)sloth_pop(x));
	if (err != SQLITE_OK) {
		sloth_throw(x, -10000 - err);
	}
}

void sloth2sqlite_close_(X* x) {
	int err = sqlite3_close((sqlite3*)sloth_pop(x));
	if (err != SQLITE_OK) {
		sloth_throw(x, -10000 - err);
	}
}

/* -- Execute ------------------------------------------ */

typedef struct {
	X* x;
	CELL xt;
	char *sql;
} sloth2sqlite_callback_data;

int sloth2sqlite__generic_callback(void *cdata, int count, char **data, char **columns) {
	X* x = ((sloth2sqlite_callback_data*)cdata)->x;
	CELL xt = ((sloth2sqlite_callback_data*)cdata)->xt;

	int i;
	for (i = count - 1; i >= 0; i--) {
		sloth_push(x, (CELL)data[i]);
		sloth_push(x, strlen(data[i]));
	}

	sloth_push(x, (CELL)count);
	sloth_eval(x, xt);

	return 0;
}

void sloth2sqlite_exec_(X* x) {
	CELL err;
	sloth2sqlite_callback_data cdata;

	CELL xt = sloth_pop(x);
	CELL l = sloth_pop(x);
	char *a = (char *)sloth_pop(x);
	sqlite3 *db = (sqlite3 *)sloth_pop(x);

	cdata.x = x;
	cdata.xt = xt;
	cdata.sql = malloc(l + 1);
	memcpy(cdata.sql, a, l);
	cdata.sql[l] = 0;

	err = sqlite3_exec(db, cdata.sql, &sloth2sqlite__generic_callback, &cdata, NULL);

	free(cdata.sql);

	if (err != SQLITE_OK) {
		sloth_throw(x, -10000 - err);
	}
}

/* -- Columns ------------------------------------------ */

void sloth2sqlite_column_text_(X* x) {
	const char *res;
	int icol = (int)sloth_pop(x);
	sqlite3_stmt *stmt = (sqlite3_stmt*)sloth_pop(x);
	res = sqlite3_column_text(stmt, icol);
	sloth_push(x, (CELL)res);
	sloth_push(x, strlen(res));
}

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_sqlite(X* x) {
	/* TODO Create SQLITE wordlist */

	SLOTH2SQLITE_CODE("LIBVERSION", libversion);
	SLOTH2SQLITE_CODE("OPEN", open);
	SLOTH2SQLITE_CODE("PREPARE", prepare_v2);
	SLOTH2SQLITE_CODE("STEP", step);
	SLOTH2SQLITE_CODE("FINALIZE", finalize);
	SLOTH2SQLITE_CODE("CLOSE", close);

	/* -- Execute ---------------------------------------- */

	SLOTH2SQLITE_CODE("EXEC", exec);

	/* -- Columns ---------------------------------------- */

	SLOTH2SQLITE_CODE("COLUMN-TEXT", column_text);
}
