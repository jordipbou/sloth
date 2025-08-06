#include"sloth.h"

/* == Facility words =================================== */

void sloth_at_x_y_(X* x);
void sloth_key_question_(X* x);
void sloth_page_(X* x);

/* == Facility extension words ========================= */

/* -- Structures --------------------------------------- */

/* Not really in facility wordset, but needed by */
/* +FIELD so I just add it to the wordset. */
void sloth_fetch_plus_(X* x);
void sloth_plus_field_(X* x);
void sloth_begin_structure_(X* x);
void sloth_c_field_colon_(X* x);
void sloth_field_colon_(X* x);

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

void sloth_bootstrap_facility_wordset(X* x);
