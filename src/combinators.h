#ifndef COMBINATORS_H
#define COMBINATORS_H

#include"vm.h"

V _binrec(M* m, C a, C b, C c, C d) {
	eval(m, d);
	if (POP(m)) {
		eval(m, c);
	} else {
		eval(m, b);
		_binrec(m, a, b, c, d);
		swap(m);
		_binrec(m, a, b, c, d);
		eval(m, a);
	}
}

V binrec(M* m) {
	C a = POP(m);
	C b = POP(m);
	C c = POP(m);
	C d = POP(m);
	_binrec(m, a, b, c, d);
}

V times(M* m) { L2(m, C, q, C, n); while (n-- > 0) { PUSH(m, q); execute(m); DO(m, inner); } }

V combinators_ext(M* m) {
	switch (TOKEN(m)) {
	case 'r':
		switch (TOKEN(m)) {
		case 'b': binrec(m); break;
		}
		break;
	case 't': times(m); break;
	}
}

#endif
