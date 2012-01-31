#include <unistd.h>
#include <termios.h>
#include "codes.h"

/** Terminal operations **/
tstate t_getstate(){ return ft_getstate(1); }
tstate ft_getstate(){ /* FIXME */ }

int t_setstate(tstate state){ return ft_setstate(1, state); }
int ft_setstate(int fd, tstate state){ /* FIXME */ }

int t_clear(){ return ft_clear(1); }
int ft_clear(int fd){ return write(fd, "[2J", 4 ); }


/** Font operations **/
int f_default(){ return ff_default(1); }
int f_bright(){ return ff_bright(1); }
int f_red(){ return ff_red(1); }
int f_blue(){ return ff_blue(1); }
int f_magenta(){ return ff_magenta(1); }
int f_green(){ return ff_green(1); }
int f_yellow(){ return ff_yellow(1); }
int f_cyan(){ return ff_cyan(1); }
int f_white(){ return ff_white(1); }
int f_black(){ return ff_black(1); }
int f_bold(){ return ff_bold(1); }
int f_underline(){ return ff_underline(1); }

int ff_default(int fd){ return write(fd, "[0m", 4 ); }
int ff_bright(int fd){ return write(fd, "[1m", 4); }
int ff_red(int fd){ return write(fd, "[31m", 5 ); }
int ff_blue(int fd){ return write(fd, "[34m", 5 ); }
int ff_magenta(int fd){ return write(fd, "[35m", 5 ); }
int ff_green(int fd){ return write(fd, "[32m", 5 ); }
int ff_yellow(int fd){ return write(fd, "[33m", 5 ); }
int ff_cyan(int fd){ return write(fd, "[36m", 5 ); }
int ff_white(int fd){ return write(fd, "[37m", 5 ); }
int ff_black(int fd){ return write(fd, "[30m", 5 ); }
int ff_bold(int fd){ return write(fd, "[1m", 4); }
int ff_underline(int fd){ return write(fd, "[4m", 4); }


/** Cursor operations **/
int c_up(){ return fc_up(1); }
int c_down(){ return fc_down(1); }
int c_right(){ return fc_right(1); }
int c_left(){ return fc_left(1); }
int c_nline(){ return fc_nline(1); }
int c_pline(){ return fc_pline(1); }
int c_line0(){ return fc_line0(1); }
int c_save(){ return fc_save(1); }
int c_restore(){ return fc_restore(1); }

int fc_up(int fd){ return write(fd, "[A", 3); }
int fc_down(int fd){ return write(fd, "[B", 3); }
int fc_right(int fd){ return write(fd, "[C", 3); }
int fc_left(int fd){ return write(fd, "[D", 3); }
int fc_nline(int fd){ return write(fd, "[E", 3); }
int fc_pline(int fd){ return write(fd, "[F", 3); }
int fc_line0(int fd){ return write(fd, "[0;0H", 6); }
int fc_save(int fd){ return write(fd, "[s", 3); }
int fc_restore(int fd){ return write(fd, "[u", 3); }
