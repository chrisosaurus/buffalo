#include <stdio.h>
#include "codes.h"

static tstate orig, nstate; /* states of the terminal */

void
i_setup(void){
    t_getstate(&orig);
    nstate = t_initstate(&orig);
    t_setstate(&nstate);
}

void
i_tidyup(void){
    t_setstate(&orig);
    fflush(stdout);
}

int main(){
    char ch[7]; /* the returns from the terminal are up to 6 chars long, 7th is for \0 */
    i_setup();

    while( 1 ){
        t_read(ch, 7);
        printf("I got (%s)\n", ch);
        if( ch[0] == 27 && ch[1] == 0 ){ /* either escape or stray alt */
        }

        if( ch[0] == 27 && ch[1] == 'q' && ch[2] == 0 ){ /* alt+q */
            break;
        }
    }

    i_tidyup();
};
