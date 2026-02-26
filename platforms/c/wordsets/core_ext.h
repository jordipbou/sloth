#include<sloth.h>

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

void sloth_pick_(X* x) {
	sloth_push(x, sloth_pick(x, sloth_pop(x))); 
}

void bootstrap_core_ext_wordset(X* x) {
	SLOTH_CODE("PICK", pick);	
}
