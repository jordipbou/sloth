#include"sloth.h"

#define SLOTH2PLIBSYS_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth2plibsys_##f##_));

/* -- Library initialization ---------------------------- */

void sloth2plibsys_p_libsys_init_(X* x);
void sloth2plibsys_p_libsys_shutdown_(X* x);

void sloth2plibsys_p_libsys_version_(X* x);

/* -- Multithreading ----------------------------------- */

void sloth2plibsys_p_uthread_create_(X* x);
void sloth2plibsys_p_uthread_exit_(X* x);
void sloth2plibsys_p_uthread_join_(X* x);
void sloth2plibsys_p_uthread_sleep_(X* x);

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_plibsys(X* x);
