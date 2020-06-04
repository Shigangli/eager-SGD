#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define DEPS 10

int main(int argc, char * argv[]){
    
    int rank, size;

    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    ffop_h master_op_first;
    ffop_h master_op_all;
    ffop_h dep_ops_first[DEPS];
    ffop_h dep_ops_all[DEPS];
    
    int acc1=0, acc2=0;
    int inc=1;

    ffnop(FFOP_DEP_FIRST, &master_op_first);
    ffnop(0, &master_op_all);
    
    for (int i=0; i<DEPS; i++){
        ffcomp(&acc1, &inc, 1, FFINT32, FFSUM, 0, &acc1, &(dep_ops_first[i]));
        ffop_hb(master_op_first, dep_ops_first[i], 0);

        ffcomp(&acc2, &inc, 1, FFINT32, FFSUM, 0, &acc2, &(dep_ops_all[i]));
        ffop_hb(master_op_all, dep_ops_all[i], 0);
    }

    for (int i=0; i<DEPS; i++){
        ffop_post(master_op_first);
        ffop_post(master_op_all);   
    }

    if (acc1==DEPS && acc2==DEPS*DEPS) printf("PASSED!\n");
    else printf("FAILED!\n");

    ffop_free(master_op_first);
    ffop_free(master_op_all);
    for (int i=0; i<DEPS; i++){
        ffop_free(dep_ops_first[i]);
        ffop_free(dep_ops_all[i]);
    }

    fffinalize();
    return 0;   
}
