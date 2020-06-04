#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 100

int main(int argc, char * argv[]){
    
    int rank, size, count;

    if (argc!=2){
        printf("Usage: %s <count>\n", argv[0]);
        exit(1);
    } 

    count = atoi(argv[1]);

    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    //we keep it to check the result
    int32_t * to_reduce = malloc(sizeof(int32_t)*count);
    int32_t * reduced = malloc(sizeof(int32_t)*count);
    
    int failed=0;
    
    ffschedule_h allreduce;
    ffallreduce(FFINPLACE, reduced, count, 0, FFSUM, FFINT32, 0, &allreduce);

    MPI_Barrier(MPI_COMM_WORLD); //not needed, just for having nice output
    for (int i=0; i<N; i++){
            
        for (int j=0; j<count; j++){
            to_reduce[j] = i+j;
            reduced[j] = i+j;
        }
        
        ffschedule_post(allreduce);
        ffschedule_wait(allreduce);

        for (int j=0; j<count; j++){
            if (reduced[j] != (i+j)*size){
                printf("[rank %i] FAILED! (i: %i; j: %i) (expected: %u; got: %u; toreduce: %u; csize: %u)\n", rank, i, j, (i+j)*size, reduced[j], to_reduce[j], size);
                failed=1;
            }
        }

        /* this is ugly... TODO: have internal tags for collectives or allow to change the tag of an operation without changing it */
        /* let's rely on the MPI non-overtaking rule for now */
        //MPI_Barrier(MPI_COMM_WORLD);
    }

    ffschedule_delete(allreduce);

    fffinalize();
    
    free(reduced);
    free(to_reduce);

    if (!failed){
        printf("PASSED!\n");
    }
    
    return 0;
}
