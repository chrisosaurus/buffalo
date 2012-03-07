#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "codes.h"

int main(){
	struct termios term, bkp;

	if( tcgetattr(1, &term) ){
		perror("failed to get termios\n");
		exit(1);
	}
	bkp = term;
	term.c_lflag = 0; // set non-canonical
	term.c_cc[VTIME] = 0; // set timeout
	term.c_cc[VMIN] = 1; // set read to return after it has one char available
	//tcflush(1, TCIFLUSH);
	if( tcsetattr(1, TCSANOW, &term) ){
		perror("Failed to set termios\n");
		exit(1);
	}

	// prepare terminal
	t_clear();
	f_normal();
	c_line0();

	//write(1, "hello\n", 6);
	char chs[2] = {0, 0};
	int alive = 1;
	int interactive = 0;

	// testing
	f_normal();

    fputs("d", stdout);
	f_red();
    fputs("r", stdout);
	f_green();
    fputs("g", stdout);
	f_yellow();
    fputs("y", stdout);
	f_blue();
    fputs("b", stdout);
	f_magenta();
    fputs("m", stdout);
	f_cyan();
    fputs("c", stdout);

	f_normal();
	f_bold();

    fputs("b", stdout);
	f_red();
    fputs("r", stdout);
	f_green();
    fputs("g", stdout);
	f_yellow();
    fputs("y", stdout);
	f_blue();
    fputs("b", stdout);
	f_magenta();
    fputs("m", stdout);
	f_cyan();
    fputs("c", stdout);

	f_normal();
    fputs("\n", stdout);
    fflush(stdout);


    b_default();
    fputs("default\n", stdout);
    b_black();
    fputs("black\n", stdout);
    b_red();
    fputs("red\n", stdout);
    b_green();
    fputs("green\n", stdout);
    b_yellow();
    fputs("yellow\n", stdout);
    b_blue();
    fputs("blue\n", stdout);
    b_magenta();
    fputs("magenta\n", stdout);
    b_cyan();
    fputs("cyan\n", stdout);
    b_white();
    fputs("white\n", stdout);
    b_default();
    fflush(stdout);

	if( tcsetattr(1, TCSANOW, &bkp) ){
		perror("failed to reset termios setting back to the backup\n");
		exit(1);
	}
	// tidy up a little
	/*t_clear();*/
	f_normal();
	/*c_line0();*/
}
