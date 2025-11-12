#include "sloth.h"

/* -- File access words -------------------------------- */

void sloth_paren_(X* x);
void sloth_bin_(X* x);
void sloth_close_file_(X* x);
void sloth_create_file_(X* x);
void sloth_delete_file_(X* x);
void sloth_file_position_(X* x);
void sloth_file_size_(X* x);
void sloth_include_file_(X* x);
void sloth_included_(X* x);
void sloth_open_file_(X* x);
void sloth_r_slash_o_(X* x);
void sloth_r_slash_w_(X* x);
void sloth_read_file_(X* x);
void sloth_read_line_(X* x);
void sloth_reposition_file_(X* x);
void sloth_resize_file_(X* x);
void sloth_s_quote_(X* x);
void sloth_source_id_(X* x);
void sloth_w_slash_o_(X* x);
void sloth_write_file_(X* x);
void sloth_write_line_(X* x);

/* -- File access extension words ---------------------- */

void sloth_file_status_(X* x);
void sloth_flush_file_(X* x);
void sloth_include_(X* x);
void sloth_refill_(X* x);
void sloth_rename_file_(X* x);
void sloth_require_(X* x);
void sloth_required_(X* x);
void sloth_s_backslash_quote_(X* x);

/* == Bootstrapping ==================================== */

void sloth_bootstrap_file_access_wordset(X* x);
