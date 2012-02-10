#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>

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

    puts("[2J");
    puts("[0m");
    puts("[0;0H");
    printf("hello world!!!!\n");
    puts("[32m");
    puts("GREEEEEEENNN");
    printf("%c[33m%s", 0x1b, "YELLOW");
    int i;
    for( i=0; i<10; ++i )
        puts("hehe");
    puts("enter to exit");
    fputs("[0;0H", 1);
    getchar();
}
