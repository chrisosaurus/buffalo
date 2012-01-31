#ifndef CODES_H
#define CODES_H

/* terminal operations */
/* non f versions write to stdout */
/* return code is that returned by the f versions */
int term_clear();
/* same as above but the file descriptor to write to can be specified */
/* return code is that returned by write */
int fterm_clear(int fd);


/* text operations, change colour/style of text following it */
/* non f versions write to standard out */
/* return code is that returned by the f versions */
int text_default();
int text_bright();
int text_red();
int text_green();
int text_yellow();
int text_blue();
int text_magenta();
int text_white();
int text_black();
int text_cyan();
int text_bold(); /* synonym for bright */
int text_underline();
/* same as above but the file descriptor to write to can be specified */
/* return code is that returned by write */
int ftext_default(int fd);
int ftext_bright(int fd);
int ftext_red(int fd);
int ftext_green(int fd);
int ftext_yellow(int fd);
int ftext_blue(int fd);
int ftext_magenta(int fd);
int ftext_cyan(int fd);
int ftext_white(int fd);
int ftext_black(int fd);
int ftext_bold(int fd); /* synonym for bright */
int ftext_underline(int fd);

/* cursor operations */
/* non f versions write to stdout */
/* return code is that returned by the f versions */
int curs_up();
int curs_down();
int curs_right();
int curs_left();
int curs_nline();
int curs_pline();
int curs_line0();
int curs_save();
int curs_restore();
/* same as above but the file descriptor to write to can be specified */
/* return code is that returned by write */
int fcurs_up(int fd);
int fcurs_down(int fd);
int fcurs_right(int fd);
int fcurs_left(int fd);
int fcurs_nline(int fd);
int fcurs_pline(int fd);
int fcurs_line0(int fd);
int fcurs_save(int fd);
int fcurs_restore(int fd);

#endif
