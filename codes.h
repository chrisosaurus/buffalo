#ifndef CODES_H
#define CODES_H

#include <termios.h> /* struct termios */

/** ASCII definitions **/
#define K_ENT 10
#define K_ESC 27
#define K_BKS 127
#define CONTROL(c) (c-96)
#define SHIFT(c) (c-32)


/** Terminal operations **/
typedef struct termios tstate;
int t_getstate(tstate *state); /* get current state */
tstate t_initstate(const tstate *state); /* take state, set needed flags and return */
int t_setstate(const tstate *state); /* set state */
int t_clear(); /* send clear code */
int t_getwidth(); /* get terminal width */
int t_getheight(); /* get terminal height */
int t_read(char *c, size_t len); /* request a read of up to len chars into c, will memset c to 0 first */

/** Font operations, change colour/style of text following it **/
int f_normal();
int f_bright();
int f_black();
int f_red();
int f_green();
int f_yellow();
int f_blue();
int f_magenta();
int f_cyan();
int f_white();
int f_bold(); /* synonym for bright */
int f_underline();

/** Background operations, change colour of background following it **/
int b_default();
int b_black();
int b_red();
int b_green();
int b_yellow();
int b_blue();
int b_magenta();
int b_cyan();
int b_white();

/** Cursor operations **/
int c_up();
int c_down();
int c_right();
int c_left();
int c_nline();
int c_pline();
int c_line0();
int c_save();
int c_restore();
/*int c_scrle(); enable scrolling - needed? */
int c_moveu(); /* move cursor up a line, will add a line if at start of screen */
int c_moved(); /* move cursor down a line, will add a line if at end of screen */
/* scroll screen up or down by n lines */
int c_scrlu(int n);
int c_scrld(int n);
int c_goto(int line, int pos);

#endif
