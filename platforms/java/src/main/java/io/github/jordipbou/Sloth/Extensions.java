package io.github.jordipbou.Sloth;

public class Extensions {
	//  // Loop helpers
	//  void ipush() {
	//  	rpush(ua_get(KX));
	//  	ua_set(KX, ua_get(JX));
	//  	ua_set(JX, ua_get(IX));
	//  	ua_set(LX, 0);
	//  }
	//  void ipop() {
	//  	ua_set(LX, 0);
	//  	ua_set(IX, ua_get(JX));
	//  	ua_set(JX, ua_get(KX));
	//  	ua_set(KX, rpop());
	//  }
	//  void _unloop_() {
	//  	ua_set(LX, ua_get(LX) - 1);
	//  	ua_set(IX, ua_get(JX));
	//  	ua_set(JX, ua_get(KX));
	//  	if (ua_get(LX) == -1) {
	//  		ua_set(KX, rpick(1));
	//  	} else {
	//  		ua_set(KX, rpick(3));
	//  	}
	//  }
	//  // Algorithm for doloop taken from pForth
	//  // (pf_inner.c case ID_PLUS_LOOP)
	//  void _doloop_() {
	//  	ipush();
	//  	int q = pop();
	//  	int do_first_loop = pop();
	//  	ua_set(IX, pop());
	//  	int l = pop();

	//  	int o = ua_get(IX) - l;
	//  	int d = 0;

	//  	// First iteration is executed always on a DO word
	//  	if (do_first_loop == 1) {
	//  		eval(q);
	//  		if (ua_get(LX) == 0) {
	//  			d = pop();
	//  			o = ua_get(IX) - l;
	//  			ua_set(IX, ua_get(IX) + d);
	//  		}
	//  	}

	//  	if (!(do_first_loop == 0 && o == 0)) {
	//  		while (((o ^(o + d)) & (o ^ d)) >= 0 && ua_get(LX) == 0) {
	//  			// TODO All this block is exact as in the previous if
	//  			eval(q);
	//  			if (ua_get(LX) == 0) { // Avoid popping if we are leaving
	//  				d = pop();
	//  				o = ua_get(IX) - l;
	//  				ua_set(IX, ua_get(IX) + d);
	//  			}
	//  		}
	//  	}

	//  	if (ua_get(LX) == 0 || ua_get(LX) == 1) {
	//  		// Leave case
	//  		ipop(); 
	//  	} else if (ua_get(LX) < 0) {
	//  		// Unloop case
	//  		ua_set(LX, ua_get(LX) + 1);
	//  		rpop();
	//  		_exit_();
	//  	}
	//  }

	// void _m_star_() { long d = lpop() * lpop(); dpush(d); }

	// void _d_plus_() { long n = dpop(); dpush(dpop() + n); }
}
