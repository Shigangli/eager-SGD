#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 100



int main(int argc, char * argv[]){
    
    int rank, size;

    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    int acc=0;
    int inc=1;
    ffop_h nop1, nop2, nop3;
    ffnop(0, &nop1);
    ffnop(0, &nop2);
    ffcomp(&acc, &inc, 1, FFINT32, FFSUM, FFOP_DEP_OR, &acc, &nop3);

    ffop_hb(nop1, nop3, 0);
    ffop_hb(nop2, nop3, 0);

    for (int i=0; i<N; i++){
        // this activates op3
        ffop_post(nop1);

        // this should not activate op3
        ffop_post(nop2);

        ffop_wait(nop1);
        ffop_wait(nop2);
        ffop_wait(nop3);
    }
 
    if (acc==N) printf("PASSED!\n");
    else printf("FAILED!\n");

    fffinalize();
    return 0;   
}
