#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

int main(int argc, char * argv[]){

    int rank;
    int size;
    int count;

    if (argc!=2){
        printf("Usage: %s <count>\n", argv[0]);
        exit(1);
    } 

    count = atoi(argv[1]);

    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    printf("ID: %i; size: %i\n", rank, size);

    ffop_h op;

    int * buffer = (int *) malloc(sizeof(int)*count);

    if (rank==0){
        for (int i=0; i<count; i++) buffer[i] = i;
        FFCALL(ffsend(buffer, count, FFINT32, 1, 0, 0, &op));
    }else{
        FFCALL(ffrecv(buffer, count, FFINT32, 0, 0, 0, &op));        
    }
    
    ffop_post(op);   
    ffop_wait(op);
       
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank==1){
        int fail=0;
        for (int i=0; i<count; i++){
            if (buffer[i]!=i){
                printf("Fail!\n");
                fail=1;
            }
        }

        if (!fail) printf("Success!\n");
    }


    ffop_free(op);

    fffinalize();   
    free(buffer);
    
    return 0;
}
