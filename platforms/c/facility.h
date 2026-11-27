#include "sloth.h"

/* == Facility words =================================== */

void sloth_key_question_(X* x);

/* == Facility extension words ========================= */

/* -- Keyboard input ----------------------------------- */

void sloth_e_key_(X* x);
void sloth_e_key_to_char_(X* x);
void sloth_e_key_to_f_key_(X* x);
void sloth_e_key_question_(X* x);
void sloth_emit_question_(X* x);

void sloth_k_alt_mask_(X* x);
void sloth_k_ctrl_mask_(X* x);
void sloth_k_shift_mask_(X* x);
void sloth_k_insert_(X* x);
void sloth_k_delete_(X* x);
void sloth_k_home_(X* x);
void sloth_k_prior_(X* x);
void sloth_k_next_(X* x);
void sloth_k_end_(X* x);
void sloth_k_up_(X* x);
void sloth_k_down_(X* x);
void sloth_k_left_(X* x);
void sloth_k_right_(X* x);
void sloth_k_f1_(X* x);
void sloth_k_f2_(X* x);
void sloth_k_f3_(X* x);
void sloth_k_f4_(X* x);
void sloth_k_f5_(X* x);
void sloth_k_f6_(X* x);
void sloth_k_f7_(X* x);
void sloth_k_f8_(X* x);
void sloth_k_f9_(X* x);
void sloth_k_f10_(X* x);
void sloth_k_f11_(X* x);
void sloth_k_f12_(X* x);

/* -- Time and date ------------------------------------ */

void sloth_time_and_date_(X* x);
void sloth_ms_(X* x);

/* == Bootstrapping ==================================== */

void sloth_bootstrap_facility_word_set(X* x);
