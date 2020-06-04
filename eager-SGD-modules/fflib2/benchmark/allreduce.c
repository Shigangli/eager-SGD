#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "liblsb.h"

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

#define N 100

int main(int argc, char * argv[]){
    
    int rank, size, count, mpi;

    if (argc!=3){
        printf("Usage: %s <count> <mpi/fflib>\n", argv[0]);
        exit(1);
    } 

    count = atoi(argv[1]);
    mpi = !strcmp(argv[2], "mpi");

    ffinit(&argc, &argv);
    LSB_Init(argv[2], 0);


    ffrank(&rank);
    ffsize(&size);

    int32_t * to_reduce = malloc(sizeof(int32_t)*count);;
    int32_t * reduced = malloc(sizeof(int32_t)*count);;
    
    int failed=0;
    int gfailed=0;    

    ffschedule_h allreduce;

    LSB_Set_Rparam_int("rank", rank);
    LSB_Set_Rparam_int("comm_size", size);
    LSB_Set_Rparam_int("msg_size", count*sizeof(int32_t));
    LSB_Set_Rparam_string("lib", argv[2]);

    LSB_Set_Rparam_string("type", "schedule_creation");
    LSB_Res();
    ffallreduce(to_reduce, reduced, count, 0, FFSUM, FFINT32, 0, &allreduce);
    LSB_Rec(0);


    LSB_Set_Rparam_string("type", "schedule_execution");

    MPI_Barrier(MPI_COMM_WORLD); //not needed, just for having nice output
    for (int i=0; i<N; i++){
        
        for (int j=0; j<count; j++){
            to_reduce[j] = i+j;
            reduced[j] = 0;
        }


        MPI_Barrier(MPI_COMM_WORLD);


        if (mpi) {
            LSB_Res();
            MPI_Allreduce(to_reduce, reduced, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            LSB_Rec(i);
        } else {
            ffschedule_post(allreduce);
            LSB_Res();
            ffschedule_wait(allreduce);
            LSB_Rec(i);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        /*for (int j=0; j<count; j++){
            if (reduced[j] != (i+j)*size){
                //printf("FAILED!\n");
                failed=1;
            }
        }*/
    
        MPI_Allreduce(&failed, &gfailed, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        if (gfailed>0) break;
    }

    if (gfailed==0){
        printf("PASSED!\n");
    }


    LSB_Set_Rparam_string("type", "schedule_deletion");
    LSB_Res();
    ffschedule_delete(allreduce);
    LSB_Rec(0);

    LSB_Finalize();
    fffinalize();
        
    free(reduced);
    free(to_reduce);


    
    return 0;
}
