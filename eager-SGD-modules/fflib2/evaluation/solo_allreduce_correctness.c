#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

void print_array(char *lbl, int32_t * array, int len){
    int rank;
    ffrank(&rank);

    printf("[%i] %s: array [ ", rank, lbl);
    for (int i=0; i<len; i++){
        printf("%i ", array[i]);
    }
    printf("]\n");
}

int compare_buffers(int32_t *a, int32_t *b, int count){
    int rank;
    ffrank(&rank);

    for (int i=0; i<count; i++){
        int32_t aa, bb;
        aa=a[i];
        bb=b[i];
        if (aa!=bb) {
            printf("[%i] a[%i] = %i; b[%i] = %i;\n", rank, i, aa, i, bb);
            return aa - bb;
        }
    }
    return 0;
}

int main(int argc, char * argv[]){
    
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
    int32_t * mpi_reduced = (int32_t *) calloc(count, sizeof(int32_t));


    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&size);

    int to_slow_down = size * slowp;

    printf("Slowing down at most %i processes at each iteration\n", to_slow_down);

    srand(time(NULL)/(rank+1));
   
    ffschedule_h solo_allreduce_sched;
    ffsolo_allreduce(to_reduce, reduced, count, 0, FFSUM, FFINT32, 0, 20, &solo_allreduce_sched);

    if (rank==0){
        FILE * fp = fopen("test_vis", "w+");
        ffschedule_print(solo_allreduce_sched, fp, "solo_allreduce");
        fclose(fp);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    ffschedule_start(solo_allreduce_sched);

    for(int i=0; i<iters; i++){
        printf("[%i] Iteration %i\n", rank, i);
        for (int j=0; j<count; j++){
            to_reduce[j]++;
        }
    
        MPI_Barrier(MPI_COMM_WORLD);
        ffschedule_post(solo_allreduce_sched);
        ffschedule_wait(solo_allreduce_sched);
        MPI_Barrier(MPI_COMM_WORLD);
    
        MPI_Allreduce(to_reduce, mpi_reduced, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);


        if (compare_buffers(reduced, mpi_reduced, count)!=0){
            printf("[%i] Correctness check failed (it %i)!\n", rank, i);

            print_array("FFLIB", reduced, count);
            print_array("MPI", mpi_reduced, count);

            assert(0);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("Correcness check passed!\n");

    fffinalize();    
    free(reduced);
    free(to_reduce);
    
    return 0;
}
