#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }

int main(int argc, char * argv[]){

    int rank;
    int size;
    int count;
    int iters;

    if (argc!=3){
        printf("Usage: %s <count> <iters>\n", argv[0]);
        exit(1);
    } 

    count = atoi(argv[1]);
    iters = atoi(argv[2]);

    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    printf("ID: %i; size: %i\n", rank, size);

    ffop_h pingop, pongop;

    int * buffer_ping = (int *) malloc(sizeof(int)*count);
    int * buffer_pong = (int *) malloc(sizeof(int)*count);

    for (int iter=0; iter<iters; iter++) {
        if (rank==0){ /* sender */
            for (int i=0; i<count; i++) buffer_ping[i] = i;
            for (int i=0; i<count; i++) buffer_pong[i] = 0;

            FFCALL(ffsend(buffer_ping, count, FFINT32, 1, 0, 0, &pingop));
            FFCALL(ffrecv(buffer_pong, count, FFINT32, 1, 0, 0, &pongop));        

            ffop_post(pingop);   
            ffop_post(pongop);   

        }else{ /* receiver */

            FFCALL(ffrecv(buffer_ping, count, FFINT32, 0, 0, 0, &pingop));
            FFCALL(ffsend(buffer_ping, count, FFINT32, 0, 0, 0, &pongop));
        
            ffop_hb(pingop, pongop, 0);
    
            ffop_post(pingop);   
            //ffop_post(pongop); /* no need to post a dependent op! */  
        }
    
        /* wait the pong to be received/sent */
        ffop_wait(pongop);

        MPI_Barrier(MPI_COMM_WORLD);
   

        int fail=0;
        for (int i=0; i<count && rank==0; i++){
            if (buffer_pong[i]!=i){
                printf("Fail ping: %i (expected: %i); pong: %i (expected %i)!\n", buffer_ping[i], i, buffer_pong[i], count-i);
                fail=1;
            }
        }

        for (int i=0; i<count && rank==1; i++){
            if (buffer_ping[i]!=i){
                printf("Fail ping: %i (expected: %i); pong: %i (expected %i)!\n", buffer_ping[i], i, buffer_pong[i], count-i);
                fail=1;
            }
        }


        if (!fail) printf("Success!\n");


        ffop_free(pingop);
        ffop_free(pongop);
    }

    fffinalize();   
    free(buffer_ping);
    free(buffer_pong);    

    return 0;
}
