#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 3

#define SEED 103343521

int main(int argc, char * argv[]){
    
    int rank, comm_size;

    volatile int acc=0;
    int inc=1;

    ffschedule_h sched;
    ffop_h activation_op, activated_op, activator, activator_test, sched_begin_op, activation_join, activator_auto;      

    // we need to use the same seed in this test!
    srand(SEED);

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&comm_size);

    // create the activation tree
    ffactivation(0, 0, &activator, &activator_auto, &activator_test, &activation_join, &sched);
    
    // get the hook to activate the op
    ffschedule_get_end_op(sched, &activation_op);

    // get the schedule start op 
    ffschedule_get_begin_op(sched, &sched_begin_op);

    // create the activate op
    ffcomp((void *) &acc, &inc, 1, FFINT32, FFSUM, 0, (void *) &acc, &activated_op);

    // set the dependency
    ffop_hb(activation_op, activated_op, 0);

    // repost schedule once the activated_op completes
    ffop_hb(activated_op, sched_begin_op, FFDEP_IGNORE_VERSION);
    
    // this barriers are just to get a nice log
    MPI_Barrier(MPI_COMM_WORLD);

    // post the activation schedule
    ffschedule_post(sched);

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i=0; i<N; i++){  
        int activator_rank = rand() % comm_size;
        printf("[%i] Rank %i the activator this time (%i)\n", rank, activator_rank, i);
        if (activator_rank == rank){
            ffop_post(activator);   
            ffop_wait(activated_op);
            MPI_Barrier(MPI_COMM_WORLD);
        }else{
            MPI_Barrier(MPI_COMM_WORLD);
            ffop_post(activator);   
            ffop_wait(activated_op);
        }
        ffop_wait(activator_test);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    

    if (acc==N){
        printf("[%i] PASSED (acc=%i)!\n", rank, acc);
    }else{
        printf("[%i] FAIL! (acc=%i instead of %i)\n", rank, acc, N);
    }

    fffinalize();
    return 0;   
}
