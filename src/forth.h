#ifndef SLOTH_FORTH
#define SLOTH_FORTH

#define SF_DEFAULT_DICT_SIZE 65536

void SF_init(X* x) {
  x->b = malloc(SF_DEFAULT_DICT_SIZE);
}

#endif
