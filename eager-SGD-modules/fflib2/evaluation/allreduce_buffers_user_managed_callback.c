#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 100
#define SEED 439634
#define INITIAL_COUNT 1024

#define ALLREDUCE_STATE 12345678

void allreduce_callback(ffop_h op, void * arg){
    int state = *((int *) arg);
    printf("allreduce state: %i\n", state);
    if (state!=ALLREDUCE_STATE){
        printf("FAIL!\n");
        assert(0);
    }
}

int main(int argc, char * argv[]){
    
    int rank, size, max_count;

    if (argc!=2){
        printf("Usage: %s <max count>\n", argv[0]);
        exit(1);
    } 

    max_count = atoi(argv[1]);

    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    int32_t * to_reduce = (int32_t *) malloc(sizeof(int32_t)*INITIAL_COUNT);
    int32_t * reduced = (int32_t *) malloc(sizeof(int32_t)*INITIAL_COUNT);

    ffbuffer_h sndbuff, rcvbuff;

    ffbuffer_create(to_reduce, INITIAL_COUNT, FFINT32, 0, &sndbuff);
    ffbuffer_create(reduced, INITIAL_COUNT, FFINT32, 0, &rcvbuff);
    
    int failed=0;
    
    ffschedule_h allreduce;
    ffallreduce(&sndbuff, &rcvbuff, INITIAL_COUNT, 0, FFSUM, FFINT32, FFCOLL_BUFFERS, &allreduce);
 
    /* here we set the callback */
    ffop_h cb_op, allreduce_completion_op;

    // let's just have a state to check if the callback is correct
    int allreduce_state = ALLREDUCE_STATE;    

    // create the callback op
    ffcallback(allreduce_callback, (void *) &allreduce_state, 0, &cb_op);

    // set the callback op to be called once the last op of the allreduce schedule completes
    ffschedule_get_end_op(allreduce, &allreduce_completion_op);
    ffop_hb(allreduce_completion_op, cb_op, 0);   

    /* end callback setup */

    srand(SEED);
    MPI_Barrier(MPI_COMM_WORLD); //not needed, just for having nice output
    for (int i=0; i<N; i++){
        
        int count = (rand() % max_count) + 1;
        to_reduce = (int32_t *) malloc(sizeof(int32_t)*count);
        reduced = (int32_t *) malloc(sizeof(int32_t)*count);
        ffbuffer_resize(sndbuff, to_reduce, count, FFINT32);
        ffbuffer_resize(rcvbuff, reduced, count, FFINT32);

        for (int j=0; j<count; j++){
            to_reduce[j] = i+j;
            reduced[j] = 0;
        }
        
        ffschedule_post(allreduce);
        ffschedule_wait(allreduce);

        for (int j=0; j<count; j++){
            if (reduced[j] != (i+j)*size){
                printf("[rank %i] FAILED! (i: %i; j: %i) (expected: %u; got: %u; toreduce: %u; csize: %u)\n", rank, i, j, (i+j)*size, reduced[j], to_reduce[j], size);
                failed=1;
            }
        }

    }

    ffschedule_delete(allreduce);
    ffbuffer_delete(sndbuff);
    ffbuffer_delete(rcvbuff);

    fffinalize();
    
    free(reduced);
    free(to_reduce);

    if (!failed){
        printf("PASSED!\n");
    }
    
    return 0;
}
