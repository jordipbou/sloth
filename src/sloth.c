#include<stdio.h>
#include "sloth.h"
/*
#include "combinators.h"
#include "dictionary.h"
#include "strings.h"
*/

#ifdef _WIN32
  #include <conio.h>
#else
	#include <unistd.h>
	#include <termios.h>
#endif

/*
 Source code for getch is taken from:
 Crossline readline (https://github.com/jcwangxp/Crossline).
 It's a fantastic readline cross-platform replacement, but only getch was
 needed and there's no need to include everything else.
*/
#ifdef _WIN32
int _getch (void) {	fflush (stdout); return _getch(); }
#else
int _getch ()
{
	char ch = 0;
	struct termios old_term, cur_term;
	fflush (stdout);
	if (tcgetattr(STDIN_FILENO, &old_term) < 0)	{ perror("tcsetattr"); }
	cur_term = old_term;
	cur_term.c_lflag &= ~(ICANON | ECHO | ISIG); /* echoing off, canonical off, no signal chars */
	cur_term.c_cc[VMIN] = 1;
	cur_term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &cur_term) < 0)	{ perror("tcsetattr"); }
	if (read(STDIN_FILENO, &ch, 1) < 0)	{ /* perror("read()"); */ } /* signal will interrupt */
	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old_term) < 0)	{ perror("tcsetattr"); }
	return ch;
}
#endif

void key(X* x) { S_lit(x, (C)_getch()); }
void emit(X* x) { printf("%c", (B)S_drop_C(x)); }

B* bootForth =
"\\here _[cb.+ . b.+]q"
"\\allot _[cb.+ . + cb.+ ,]q"
"\\, _[$here , c $allot ]q"
"\\c, _[$here ; 1 $allot ]q"
"\\latest _[cc+b.+ .]q"
"\\flags _[c+]q"
"\\nfa _[c1++ d : s 1+ s]q"
"\\cfa _[c1++ d : s 1++]q"
"\\set-immediate _[$flags d:1|s;]q"
"\\is-immediate? _[$flags :1&]q"
"\\immediate _[$latest $set-immediate ]q"
"\\in _0$, "
"\\tib _255$allot "
"\\tib-length _0$, "
"\\source _[\\tib \\tib-length .]q"
"\\compare _[ro=[1s[rroo:s:=(1+s1+sr)&]ts_s_][___0]?]q"
"\\2over _[((oo)rr)rr]q"
/*
"\\find _[$latest [d0=[0][(oourr) $nfa $compare [s_s_0][1]?]?][.]w]q"
*/
"\\refill _[0\\in ,\\tib 255a\\tib-length ,]q"
"\\bl _[' ]q"
"\\is-space? _\\bl g[1+<]q"
"\\is-not-space? _\\is-space? g[0=]q"
"\\parse-name _[\\tib \\in .[oo+d$source +<s:$is-space? &][1+]woo+rr[oo+d$source +<s:$is-not-space? &][1+]wd\\in ,r(+u-)s]q"
"\\state _0$, "
"\\[ _[0\\state ,]q"
"\\] _[1\\state ,]q"
"\\: _[$parse-name h_]g\\] g93$c, "
"\\; _\\[ g'9$c, '3$c, \\c, g']$c, $immediate "
"\\interpret _[$parse-name d0=![`d[d$cfa s\\state .[$is-immediate? [x0][g0]?][_x0]?][_nd0=[__\\state .['#$c, $, 0][0]?][___1~]?]?][__0~]?]q"
"\\quit _[[1][$refill [$interpret d0=][_]w0~=[\" OK\"p10e][10e\"Word not found\"p10e]?]w]q"
"\\dup _[d]q"
"\\+ _[+]q"
"\\.s _[y]q";

int main(int argc, char** argv) {
	FILE* fptr;
	B buf[1024];
	C i;
	B* j;
	X* x = S_init();
	
  x->key = &key;
  x->emit = &emit;
  /*
  EXT(x, 'D') = &SD_ext;
  EXT(x, 'Q') = &SC_ext;
  EXT(x, 'S') = &SS_ext;
  */

	if (argc == 2 || argc == 3) {
		fptr = fopen(argv[1], "r");
		while (fgets(buf, 255, fptr)) {
			S_eval(x, buf);
			if (x->err != 0) {
					printf("ERROR: %ld\n", x->err);
					return;
			}
			x->err = 0;
		}
		printf("Ok "); for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i]); } printf("\n");
	}

  S_eval(x, bootForth);
  
	if (argc == 1 || argc == 3) {
		do {
			fgets(buf, 255, stdin);
			x->ip = buf;	
			S_inner(x);
			/*
      memset(buf, 0, 255);
      S_dump_X(buf, x, 50);
      printf("%s\n", buf);
			*/
			if (x->err != 0) { printf("ERROR: %ld", x->err); return; }
			printf("Ok "); for (i = 0; i < x->sp; i++) { printf("%ld ", x->s[i].v.i); } printf("\n");
		} while(1);
	}
}
