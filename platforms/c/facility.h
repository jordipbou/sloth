#include "sloth.h"

/* == Facility words =================================== */

void sloth_key_question_(X* x);
void sloth_page_(X* x);

/* == Facility extension words ========================= */

/* -- Keyboard input ----------------------------------- */

void sloth_e_key_(X* x);
void sloth_e_key_to_char_(X* x);
void sloth_e_key_to_f_key_(X* x);
void sloth_e_key_question_(X* x);
void sloth_emit_question_(X* x);

/* -- Time and date ------------------------------------ */

void sloth_time_and_date_(X* x);
void sloth_ms_(X* x);

/* == Bootstrapping ==================================== */

void sloth_bootstrap_facility_word_set(X* x);
