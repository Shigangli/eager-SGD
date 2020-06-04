#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdint.h>
#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

void print_array(int32_t * array, int len){
    int rank;
    ffrank(&rank);
    printf("[%i] array [ ", rank);
    for (int i=0; i<len; i++){
        printf("%i ", array[i]);
    }
    printf("]\n");
}

int main(int argc, char * argv[]){
    
    int rank, size, count, iters;

    if (argc!=3){
        printf("Usage: %s <count> <iters>\n", argv[0]);
        exit(1);
    } 

    count = atoi(argv[1]);
    iters = atoi(argv[2]);
    int32_t * to_reduce = (int32_t *) calloc(count, sizeof(int32_t));
    int32_t * reduced = (int32_t *) calloc(count, sizeof(int32_t));

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&size);

    ffschedule_h solo_allreduce_sched;
    ffsolo_allreduce(to_reduce, reduced, count, 0, FFSUM, FFINT32, 0, iters, &solo_allreduce_sched);
    ffschedule_start(solo_allreduce_sched);

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank==0){
        for(int i=0; i<iters; i++){
            printf(" #### Starting iteration %i ####\n", i);
            ffschedule_post(solo_allreduce_sched);
            ffschedule_wait(solo_allreduce_sched);
            //print_array(reduced, count);
        }   
        MPI_Request req;
        MPI_Ibarrier(MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }else{
        MPI_Request req;
        int completed=0;
        MPI_Ibarrier(MPI_COMM_WORLD, &req);
        do{
            usleep(1000*(rand()/RAND_MAX));
            for (int i=0; i<count; i++){
                to_reduce[i]++;
            }
            //print_array(reduced, count);
            MPI_Test(&req, &completed, MPI_STATUS_IGNORE);
        }while(!completed);
    }

    fffinalize();    
    free(reduced);
    free(to_reduce);
    
    return 0;
}
