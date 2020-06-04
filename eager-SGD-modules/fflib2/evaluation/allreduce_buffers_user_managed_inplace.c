#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 100
#define SEED 439634
#define INITIAL_COUNT 1024

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

    int32_t * reduced = (int32_t *) malloc(sizeof(int32_t)*INITIAL_COUNT);

    ffbuffer_h rcvbuff;

    ffbuffer_create(reduced, INITIAL_COUNT, FFINT32, 0, &rcvbuff);
    
    int failed=0;
    
    ffschedule_h allreduce;
    ffallreduce(FFINPLACE, &rcvbuff, INITIAL_COUNT, 0, FFSUM, FFINT32, FFCOLL_BUFFERS, &allreduce);
    
    srand(SEED);
    MPI_Barrier(MPI_COMM_WORLD); //not needed, just for having nice output
    for (int i=0; i<N; i++){
        
        int count = (rand() % max_count) + 1;
        reduced = (int32_t *) malloc(sizeof(int32_t)*count);
        ffbuffer_resize(rcvbuff, reduced, count, FFINT32);

        for (int j=0; j<count; j++){
            reduced[j] = i+j;
        }
        
        ffschedule_post(allreduce);
        ffschedule_wait(allreduce);

        for (int j=0; j<count; j++){
            if (reduced[j] != (i+j)*size){
                printf("[rank %i] FAILED! (i: %i; j: %i) (expected: %u; got: %u; csize: %u)\n", rank, i, j, (i+j)*size, reduced[j], size);
                failed=1;
            }
        }

    }

    ffschedule_delete(allreduce);
    ffbuffer_delete(rcvbuff);

    fffinalize();
    
    free(reduced);

    if (!failed){
        printf("PASSED!\n");
    }
    
    return 0;
}
