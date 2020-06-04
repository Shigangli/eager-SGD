#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
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
    srand(time(NULL));

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&size);

    printf("ID: %i; size: %i\n", rank, size);

    ffop_h comp;

    /* a = b + c */
    int32_t * buffer_a = (int32_t *) malloc(sizeof(int32_t)*count);
    int32_t * buffer_b = (int32_t *) malloc(sizeof(int32_t)*count);
    int32_t * buffer_c = (int32_t *) malloc(sizeof(int32_t)*count);

    for (int i=0; i<count; i++) {
        buffer_a[i] = rand();
        buffer_b[i] = rand();
        buffer_c[i] = 0;
    }

    FFCALL(ffcomp(buffer_a, buffer_b, count, FFINT32, FFSUM, 0, buffer_c, &comp));
    ffop_post(comp);
    ffop_wait(comp);
    
    int fail = 0;
    for (int i=0; i<count; i++){
        int32_t tmp = buffer_a[i] + buffer_b[i];
        if (tmp!=buffer_c[i]){
            printf("Failed!\n");
            fail=1;
        }      
    }

    ffop_free(comp);
    
    if (!fail) printf("Success!\n");
    
    fffinalize();   
    free(buffer_a);
    free(buffer_b);    
    free(buffer_c);

    return 0;
}
