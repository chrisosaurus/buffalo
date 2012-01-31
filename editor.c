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
  term_clear();
  text_default();
  curs_line0();

  //write(1, "hello\n", 6);
  char chs[2] = {0, 0};
  int alive = 1;
  int interactive = 0;
  
  // testing
  text_default();
  
  write(1, "d", 1);
  text_red();
  write(1, "r", 1);
  text_green();
  write(1, "g", 1);
  text_yellow();
  write(1, "y", 1);
  text_blue();
  write(1, "b", 1);
  text_magenta();
  write(1, "m", 1);
  text_cyan();
  write(1, "c", 1);

  text_default();
  text_bold();

  write(1, "b", 1);
  text_red();
  write(1, "r", 1);
  text_green();
  write(1, "g", 1);
  text_yellow();
  write(1, "y", 1);
  text_blue();
  write(1, "b", 1);
  text_magenta();
  write(1, "m", 1);
  text_cyan();
  write(1, "c", 1);

  text_default();
  

  // main loop
  while( alive ){
    while( interactive ){
      read(0, &chs, 2);
      /*write(1, "got:", 4);
        write(1, chs, 1);
        printf("%d\n", chs[0]); */
      if( chs[0] == 127 ) // backspace
        write(1, "\b", 1);
      else if( chs[0] == 10 ) // enter
        curs_nline();
      else if( chs[0] == 27 ){ // esc, NB alt + a will mean that chs[0] is ESC and chs[1] is a
        if( ! chs[1])
          interactive = 0; // if only esc is pressed, then this means swap modes
        printf("%d\n", chs[1]); // otherwise something more complex may be going on
        chs[1] = 0;
      }
      else if( chs[0] == 1 ){ // Ctrl + a
        write(1, ":)", 2);
        interactive = 0;
      }
      else{
        if( chs[0] < 30 || chs[0] > 126 ) // dont print out of range characters
          printf("%d ", chs[0]);
        else{
          write(1, chs, 1);
        }
      }

    }

    if( read(0, &chs, 1) ){
      //printf("read something: %s\n", chs);
      switch(chs[0]){
        case 'w':
          curs_up();
          break;
        case 'a':
          curs_left();
          break;
        case 's':
          curs_down();
          break;
        case 'd':
          curs_right();
          break;
        case 'q':
          curs_nline();
          break;
        case 'e':
          curs_pline();
          break;

        case '-':
          curs_save();
          break;
        case '=':
          curs_restore();
          break;

        case '!':
          alive = 0;
          break;
        case '`':
          interactive = 1;
          break;
        case '0':
          curs_line0();
          break;

        case '1':
          text_red();
          break;
        case '2':
          text_blue();
          break;
        case '3':
          text_green();
          break;

      }
    }else
      printf("read returning empty handed\n");
  }

  if( tcsetattr(1, TCSANOW, &bkp) ){
    perror("failed to reset termios setting back to the backup\n");
    exit(1);
  }
  // tidy up a little
  term_clear();
  text_default();
  curs_line0();
}
