#ifndef CODES_H
#define CODES_H

/** ASCII definitions **/
#define K_ENT 10
#define K_ESC 27
#define K_BKS 127
#define CONTROL(c) (c-96)
#define SHIFT(c) (c-32)


/* All functions come in two forms
 * non f forms write to stdout and will return the result of the equiv f form
 * f forms are the same as the non f forms except that you must specify a file descriptor to write to
 * in both cases, the return code is that returned by write(3)
*/

/** Terminal operations **/
typedef struct termios tstate;
/* get current state */
tstate t_getstate();
tstate ft_getstate();
/* set state */
int t_setstate(tstate state);
int ft_setsate(int fd, tstate state);
/* send clear code */
int t_clear();
int ft_clear(int fd);

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

int ff_default(int fd);
int ff_bright(int fd);
int ff_red(int fd);
int ff_green(int fd);
int ff_yellow(int fd);
int ff_blue(int fd);
int ff_magenta(int fd);
int ff_cyan(int fd);
int ff_white(int fd);
int ff_black(int fd);
int ff_bold(int fd); /* synonym for bright */
int ff_underline(int fd);

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

int fc_up(int fd);
int fc_down(int fd);
int fc_right(int fd);
int fc_left(int fd);
int fc_nline(int fd);
int fc_pline(int fd);
int fc_line0(int fd);
int fc_save(int fd);
int fc_restore(int fd);
int fc_scrlu(int fd);
int fc_scld(int fd);

#endif
