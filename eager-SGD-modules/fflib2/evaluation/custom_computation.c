#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X) { int ret; if (ret=(X)!=FFSUCCESS) { printf("Error: %i\n", ret); exit(-1); } }


int myoperator(void * a, void * b, void * c, uint32_t count, ffdatatype_h type){

    int * ia = (int *) a;
    int * ib = (int *) b;
    int * ic = (int *) c;

    for (int i=0; i<count; i++){
        ic[i] = ia[i] + ib[i] + 1;
    } 

    return FFSUCCESS;

}

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

    ffoperator_h operator;
    FFCALL(ffcomp_operator_create(myoperator, 1, &operator));

    FFCALL(ffcomp(buffer_a, buffer_b, count, FFINT32, operator, 0, buffer_c, &comp));
    ffop_post(comp);
    ffop_wait(comp);

    FFCALL(ffcomp_operator_delete(operator));
    
    int fail = 0;
    for (int i=0; i<count; i++){
        int32_t tmp = buffer_a[i] + buffer_b[i] + 1;
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
