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
/* get current state */
tstate t_getstate();
/* set state */
int t_setstate(tstate state);
/* send clear code */
int t_clear();

/** Font operations, change colour/style of text following it **/
int f_default();
int f_bright();
int f_red();
int f_green();
int f_yellow();
int f_blue();
int f_magenta();
int f_white();
int f_black();
int f_cyan();
int f_bold(); /* synonym for bright */
int f_underline();

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
int c_scrlu();
int c_scrld();

#endif
