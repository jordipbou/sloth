#include<sloth.h>

#define SLOTH_CODE(w, f) sloth_code(x, w, sloth_primitive(x, &sloth_##f##_));

void bootstrap_core_wordset(X* x) {
}
