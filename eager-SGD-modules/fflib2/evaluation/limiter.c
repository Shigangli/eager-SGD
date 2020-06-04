#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "ff.h"

void fun_sync(ffop_h op, void * args){
    printf("SYNC FULL CALLED!\n");
}

void fun_async(ffop_h op, void * args){
    printf("ASYNC FULL CALLED!\n");
}

int main(int argc, char * argv[]){

    int rank, size;
    ffinit(&argc, &argv);

    ffrank(&rank);
    ffsize(&size);

    ffop_h async_op, sync_op, end_op, async_callback, sync_callback;
    ffschedule_h sched;

    ffcallback(fun_async, NULL, FFOP_DEP_OR, &async_callback);
    ffcallback(fun_sync, NULL, FFOP_DEP_OR, &sync_callback);

    ffnop(FFOP_DEP_OR, &end_op);
    ffop_hb(async_op, end_op, FFDEP_IGNORE_VERSION);
    ffop_hb(sync_op, end_op, FFDEP_IGNORE_VERSION);

    ffsolo_limiter(3, &async_op, &sync_op, &sched);
    ffop_hb(async_op, async_callback, FFDEP_IGNORE_VERSION);
    ffop_hb(sync_op, sync_callback, FFDEP_IGNORE_VERSION);

    for (int i=0; i<15; i++){
        ffschedule_post(sched);
        ffop_wait(end_op);
    }

    ffschedule_delete(sched);
    ffop_free(async_op);
    ffop_free(sync_op);
    ffop_free(end_op);

    fffinalize();
    return 0;
}
