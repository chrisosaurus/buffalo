#include <unistd.h> /* tcgetattr and tcsetattr */
#include <stdio.h>
#include <sys/ioctl.h> /* TCIOCGWINSZ */
#include <string.h> /* strlen */
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

int t_clear(){ return fputs("[2J", stdout ); }
int t_getwidth(){
	/* do I need to take TCIOCGSIZE into account? if so http://www.linuxquestions.org/questions/programming-9/get-width-height-of-a-terminal-window-in-c-810739/ */
	struct winsize ts;
	if( ioctl(0, TIOCGWINSZ, &ts) )
		return -1;
	return ts.ws_col;
}
int t_getheight(){
	/* do I need to take TCIOCGSIZE into account? if so http://www.linuxquestions.org/questions/programming-9/get-width-height-of-a-terminal-window-in-c-810739/ */
	struct winsize ts;
	if( ioctl(0, TIOCGWINSZ, &ts) )
		return -1;
	return ts.ws_row;
}
int t_read(char *c, size_t len){
	memset(c, 0, len*sizeof(char));
	return read(0, c, len);
}


/** Font operations **/
int f_normal(){ return fputs("[0m", stdout ); }
int f_bright(){ return fputs("[1m", stdout); }
int f_black(){ return fputs("[30m", stdout ); }
int f_red(){ return fputs("[31m", stdout ); }
int f_green(){ return fputs("[32m", stdout ); }
int f_yellow(){ return fputs("[33m", stdout ); }
int f_blue(){ return fputs("[34m", stdout ); }
int f_magenta(){ return fputs("[35m", stdout ); }
int f_cyan(){ return fputs("[36m", stdout ); }
int f_white(){ return fputs("[37m", stdout ); }
int f_bold(){ return fputs("[1m", stdout); }
int f_underline(){ return fputs("[4m", stdout); }

/** Background colours **/
int b_default(){ return fputs("[49m", stdout); }
int b_black(){ return fputs("[40m", stdout); }
int b_red(){ return fputs("[41m", stdout); }
int b_green(){ return fputs("[42m", stdout); }
int b_yellow(){ return fputs("[43m", stdout); }
int b_blue(){ return fputs("[44m", stdout); }
int b_magenta(){ return fputs("[45m", stdout); }
int b_cyan(){ return fputs("[46m", stdout); }
int b_white(){ return fputs("[47m", stdout); }

/** Cursor operations **/
int c_up(){ return fputs("[A", stdout); }
int c_down(){ return fputs("[B", stdout); }
int c_right(){ return fputs("[C", stdout); }
int c_left(){ return fputs("[D", stdout); }
int c_nline(){ return fputs("[E", stdout); }
int c_pline(){ return fputs("[F", stdout); }
int c_line0(){ return fputs("[0;0H", stdout); }
int c_save(){ return fputs("[s", stdout); }
int c_enscrl(){ return fputs("", stdout); }
int c_restore(){ return fputs("[u", stdout); }
/*int c_scrle(){ return fputs("[r", stdout); } */
int c_moveu(){ return fputs("M", stdout); }
int c_moved(){ return fputs("D", stdout); }
/* scrlu will move the curs up a line, scrld down. if at edge of screen will keep going, inserting blank lines if needed
 * up and down will not insert blank lines
 * CSI n S and CSI n T will scroll the page about the cursor, up or down, and will also insert blank lines if needed */
int c_scrlu(int n){ char c[6]; snprintf(c, 6, "%c[%dS", 0x1b, n); return fputs(c, stdout); }
int c_scrld(int n){ char c[6]; snprintf(c, 6, "%c[%dT", 0x1b, n); return fputs(c, stdout); }
int c_goto(int line, int off){ char c[10]; snprintf(c, 10, "%c[%d;%dH", 0x1b, line, off); return fputs(c, stdout); }
