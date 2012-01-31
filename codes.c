#include <unistd.h>
#include "codes.h"

int term_clear(){ return fterm_clear(1); }

int fterm_clear(int fd){ return write(fd, "[2J", 4 ); }


int text_default(){ return ftext_default(1); }
int text_bright(){ return ftext_bright(1); }
int text_red(){ return ftext_red(1); }
int text_blue(){ return ftext_blue(1); }
int text_magenta(){ return ftext_magenta(1); }
int text_green(){ return ftext_green(1); }
int text_yellow(){ return ftext_yellow(1); }
int text_cyan(){ return ftext_cyan(1); }
int text_white(){ return ftext_white(1); }
int text_black(){ return ftext_black(1); }
int text_bold(){ return ftext_bold(1); }
int text_underline(){ return ftext_underline(1); }

int ftext_default(int fd){ return write(fd, "[0m", 4 ); }
int ftext_bright(int fd){ return write(fd, "[1m", 4); }
int ftext_red(int fd){ return write(fd, "[31m", 5 ); }
int ftext_blue(int fd){ return write(fd, "[34m", 5 ); }
int ftext_magenta(int fd){ return write(fd, "[35m", 5 ); }
int ftext_green(int fd){ return write(fd, "[32m", 5 ); }
int ftext_yellow(int fd){ return write(fd, "[33m", 5 ); }
int ftext_cyan(int fd){ return write(fd, "[36m", 5 ); }
int ftext_white(int fd){ return write(fd, "[37m", 5 ); }
int ftext_black(int fd){ return write(fd, "[30m", 5 ); }
int ftext_bold(int fd){ return write(fd, "[1m", 4); }
int ftext_underline(int fd){ return write(fd, "[4m", 4); }


int curs_up(){ return fcurs_up(1); }
int curs_down(){ return fcurs_down(1); }
int curs_right(){ return fcurs_right(1); }
int curs_left(){ return fcurs_left(1); }
int curs_nline(){ return fcurs_nline(1); }
int curs_pline(){ return fcurs_pline(1); }
int curs_line0(){ return fcurs_line0(1); }
int curs_save(){ return fcurs_save(1); }
int curs_restore(){ return fcurs_restore(1); }

int fcurs_up(int fd){ return write(fd, "[A", 3); }
int fcurs_down(int fd){ return write(fd, "[B", 3); }
int fcurs_right(int fd){ return write(fd, "[C", 3); }
int fcurs_left(int fd){ return write(fd, "[D", 3); }
int fcurs_nline(int fd){ return write(fd, "[E", 3); }
int fcurs_pline(int fd){ return write(fd, "[F", 3); }
int fcurs_line0(int fd){ return write(fd, "[0;0H", 6); }
int fcurs_save(int fd){ return write(fd, "[s", 3); }
int fcurs_restore(int fd){ return write(fd, "[u", 3); }
