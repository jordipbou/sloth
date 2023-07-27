#ifndef S_TRACE_H
#define S_TRACE_H

C S_dump_S(B* s, X* x) {
	C i = 0, t, n = 0;
	while (i < x->sp) { s += t = sprintf(s, "%ld ", x->s[i]); n += t; i++; }
	return n;
}

C S_dump_CODE(B* s, B* a) {
	C i = 0, t = 1;
	if (a < 1000) {
    return sprintf(s, "%ld", a);
  }
	while (t && a[i] && a[i] != 10) {
		switch (a[i++]) {
		case '[': t++; break;
		case ']': t--; break;
		}
	}
	return sprintf(s, "%.*s", (unsigned int)i, a);
}

C S_dump_R(B* s, X* x) {
	C i, t = 1, n = 0;
	s += t = S_dump_CODE(s, x->ip); n += t;
	for (i = x->rp - 1; i >= 0; i--) {
		*s++ = ' '; *s++ = ':'; *s++ = ' '; n += 3;
		s += t = S_dump_CODE(s, x->r[i]); n += t;
	}
	return n;
}

C S_dump_X(B* s, X* x, unsigned int w) {
	C t, n = 0;
	s += t = S_dump_S(s, x); n += t;
	*s++ = ':'; *s++ = ' '; n += 2;
  s += t = S_dump_R(s, x); n += t;
	return n;
}

void S_trace(X* x) {
  B buf[1024];
  memset(buf, 0, 255);
	S_dump_X(buf, x, 30);
	printf("%.35s <%ld>\n", buf, x->rp);
  printf("R:%x %d W:%x %d\n", x->o, x->on, x->d, x->dn);
}

#endif