#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

void print_array(int32_t * array, int len){
    printf("array [ ");
    for (int i=0; i<len; i++){
        printf("%i ", array[i]);
    }
    printf("]\n");
}

int main(int argc, char * argv[]){
    unsigned int seed = 6545343;

    int rank, size, count, iters;
    float slowp;
    if (argc!=4){
        printf("Usage: %s <count> <iters> <slow processes>\n", argv[0]);
        exit(1);
    } 

    count = atoi(argv[1]);
    iters = atoi(argv[2]);
    slowp = atof(argv[3]);
    int32_t * to_reduce = (int32_t *) calloc(count, sizeof(int32_t));
    int32_t * reduced = (int32_t *) calloc(count, sizeof(int32_t));
    

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&size);

    int to_slow_down = size * slowp;

    printf("Slowing down at most %i processes at each iteration\n", to_slow_down);

    srand(time(NULL)/(rank+1));
   
    ffschedule_h rand_allreduce_sched;
    ffrand_allreduce(to_reduce, reduced, count, 0, FFSUM, FFINT32, 0, seed, 20, &rand_allreduce_sched);

    if (rank==0){
        FILE * fp = fopen("test_vis", "w+");
        ffschedule_print(rand_allreduce_sched, fp, "solo_allreduce");
        fclose(fp);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    ffschedule_start(rand_allreduce_sched);

    for(int i=0; i<iters; i++){
        printf("[%i] Iteration %i\n", rank, i);
        for (int i=0; i<count; i++){
            to_reduce[i]++;
        }
    
        for (int j=0; j<to_slow_down; j++){
            if (rank == rand() % size){
                usleep(1000*(rand()/RAND_MAX));
            }
        }

        ffschedule_post(rand_allreduce_sched);
        ffschedule_wait(rand_allreduce_sched);

        //print_array(reduced, count);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("[%i] Completed\n", rank);

    fffinalize();    
    free(reduced);
    free(to_reduce);
    
    return 0;
}
