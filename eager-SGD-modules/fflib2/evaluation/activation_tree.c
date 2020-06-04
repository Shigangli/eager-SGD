#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 100

#define SEED 103343521

int main(int argc, char * argv[]){
    
    int rank, comm_size;

    volatile int acc=0;
    int inc=1;

    ffschedule_h sched;
    ffop_h activation_op, activated_op, activator, activator_test, activation_join, activator_auto;      

    // we need to use the same seed in this test!
    srand(SEED);

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&comm_size);

    // create the activation tree
    ffactivation(0, 0, &activator, &activator_auto, &activator_test, &activation_join, &sched);
    
    // get the hook to activate the op
    ffschedule_get_end_op(sched, &activation_op);
 
    // create the activate op
    ffcomp((void *) &acc, &inc, 1, FFINT32, FFSUM, 0, (void *) &acc, &activated_op);

    // set the dependency
    ffop_hb(activation_op, activated_op, 0);
    
    // post the activation schedule
    ffschedule_post(sched);

   
    int activator_rank = rand() % comm_size;
        
    if (activator_rank == rank){
        printf("[%i] I'm the activator this time\n", rank);
        ffop_post(activator);
        ffop_wait(activator_test);
    }
    
    printf("[%i] WAITING FOR acc to be 1 (acc=%i)\n", rank, acc);
    while (acc!=1){;}

    printf("[%i] PASSED (acc=%i)!\n", rank, acc);

    fffinalize();
    return 0;   
}
