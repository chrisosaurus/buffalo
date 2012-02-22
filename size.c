#include <stdio.h>
#include <stdlib.h>
#include "codes.h"

int main(){
    printf("%d : %d\n", t_getwidth(), t_getheight());
    getchar();
    
    tstate orig;
    t_getstate(&orig);
    tstate nstate = t_initstate(&orig);
    t_setstate(&nstate);

    t_clear();
    f_default();
    c_line0();

    printf("%d : %d\n", t_getwidth(), t_getheight());

    t_setstate(&orig);
}
