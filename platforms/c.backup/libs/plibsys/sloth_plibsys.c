#include"sloth_plibsys.h"
#include"plibsys.h"

/* -- Library initialization ---------------------------- */

void sloth2plibsys_p_libsys_init_(X* x) {
	p_libsys_init();
}

void sloth2plibsys_p_libsys_shutdown_(X* x) {
	p_libsys_shutdown();
}

void sloth2plibsys_p_libsys_version_(X* x) {
	const pchar* version_str = p_libsys_version();
	sloth_push(x, (CELL)version_str);
	sloth_push(x, strlen(version_str));
}

/* -- Multithreading ----------------------------------- */

ppointer sloth2plibsys__generic_thread_function(ppointer arg) {
	X* x = (X*)(*((CELL*)(arg)));
	CELL xt = *((CELL*)(((char*)arg) + sCELL));
	sloth_eval(x, xt);
	return 0;
}

void sloth2plibsys_p_uthread_create_(X* x) {
	/* TODO Should some pointer to data be accepted? */
	CELL xt = sloth_pop(x);
	CELL data = sloth_here(x);
	sloth_comma(x, (CELL)x);
	sloth_comma(x, xt);
	sloth_push(x, 
		(CELL)p_uthread_create(
			&sloth2plibsys__generic_thread_function,
			(ppointer)data,
			1,
			0));
}

void sloth2plibsys_p_uthread_exit_(X* x) {
	p_uthread_exit(sloth_pop(x));
}

void sloth2plibsys_p_uthread_join_(X* x) {
	PUThread *thread = (PUThread*)sloth_pop(x);
	p_uthread_join(thread);
}

void sloth2plibsys_p_uthread_sleep_(X* x) {
	sloth_push(x, p_uthread_sleep((uCELL)sloth_pop(x)));	
}

/* -- Bootstrapping ------------------------------------ */

void sloth_bootstrap_plibsys(X* x) {
	/* Library initialization */
	SLOTH2PLIBSYS_CODE("P-LIBSYS-INIT", p_libsys_init);
	SLOTH2PLIBSYS_CODE("P-LIBSYS-SHUTDOWN", p_libsys_shutdown);
	SLOTH2PLIBSYS_CODE("P-LIBSYS-VERSION", p_libsys_version);
	/* Multithreading */
	SLOTH2PLIBSYS_CODE("P-UTHREAD-CREATE", p_uthread_create);
	SLOTH2PLIBSYS_CODE("P-UTHREAD-EXIT", p_uthread_exit);
	SLOTH2PLIBSYS_CODE("P-UTHREAD-JOIN", p_uthread_join);
	SLOTH2PLIBSYS_CODE("P-UTHREAD-SLEEP", p_uthread_sleep);
}
