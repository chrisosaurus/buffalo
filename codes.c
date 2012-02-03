#include <unistd.h>
#include <unistd.h> /* tcgetattr and tcsetattr */
#include "codes.h"

/** Terminal operations **/
int t_getstate(tstate *state){
	if( tcgetattr(1, state) )
		return -1;
	return 0;
}
tstate t_initstate(const tstate *state){
	tstate nstate = *state;
	nstate.c_lflag = 0; /* set non-canonical */
	nstate.c_cc[VTIME] = 0; /* set timeout */
	nstate.c_cc[VMIN] = 1; /* set read to return after it has one char available */
	return nstate;
}
int t_setstate(const tstate *state){
	if( tcsetattr(1, TCSANOW, state) )
		return -1;
	return 0;
}

int t_clear(){ return write(1, "[2J", 4 ); }

/** Font operations **/
int f_default(){ return write(1, "[0m", 4 ); }
int f_bright(){ return write(1, "[1m", 4); }
int f_red(){ return write(1, "[31m", 5 ); }
int f_blue(){ return write(1, "[34m", 5 ); }
int f_magenta(){ return write(1, "[35m", 5 ); }
int f_green(){ return write(1, "[32m", 5 ); }
int f_yellow(){ return write(1, "[33m", 5 ); }
int f_cyan(){ return write(1, "[36m", 5 ); }
int f_white(){ return write(1, "[37m", 5 ); }
int f_black(){ return write(1, "[30m", 5 ); }
int f_bold(){ return write(1, "[1m", 4); }
int f_underline(){ return write(1, "[4m", 4); }

/** Cursor operations **/
int c_up(){ return write(1, "[A", 3); }
int c_down(){ return write(1, "[B", 3); }
int c_right(){ return write(1, "[C", 3); }
int c_left(){ return write(1, "[D", 3); }
int c_nline(){ return write(1, "[E", 3); }
int c_pline(){ return write(1, "[F", 3); }
int c_line0(){ return write(1, "[0;0H", 6); }
int c_save(){ return write(1, "[s", 3); }
int c_restore(){ return write(1, "[u", 3); }
int c_scrlu(){ return write(1, "[S", 3); }
int c_scrld(){ return write(1, "[T", 3); }

