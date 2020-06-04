
#include "ffop_mpi_progresser.h"
#include "utils/ffarman.h"
#include <assert.h>
//#include "../utils/fflock.h"

static ffop_t *    running_ops[FFMPI_MAX_REQ];
static MPI_Request requests[FFMPI_MAX_REQ];
static ffarman_t   index_manager;

//static fflock_t progress_lock;

int ffop_mpi_progresser_init(){

    //FFLOCK_INIT(&progress_lock);
    if (ffarman_create(FFMPI_MAX_REQ, &index_manager)!=FFSUCCESS){
        return FFERROR;
    }    

    for (int i=0; i<FFMPI_MAX_REQ; i++){
        requests[i] = MPI_REQUEST_NULL;
        running_ops[i] = NULL;
    }

    return FFSUCCESS;
}

int ffop_mpi_progresser_finalize(){
    ffarman_free(&index_manager);
    //FFLOCK_FREE(&progress_lock);
    return FFSUCCESS;
}

int ffop_mpi_progresser_track(ffop_t * op, MPI_Request req, uint32_t * save_idx){

#ifdef FFPROGRESS_THREAD
    uint32_t idx = ffarman_get(&index_manager);
    *save_idx = idx;
    FFLOG("progresser is now tracking op %lu (ver: %u) with idx %u\n", op->id, op->version, idx);
    //printf("getting idx: %i\n", idx);
    if (idx==(uint32_t) -1){
        FFLOG_ERROR("Too many in-flight MPI operations! (check FFMPI_MAX_REQ)");

#ifdef FFDEBUG
        int inflight=0;
        for (int i=0; i<FFMPI_MAX_REQ; i++){
            if (requests[i] != NULL){
                MPI_Status status;
                int flag;
                MPI_Request_get_status(requests[i], &flag, &status);
                uint32_t opid = (running_ops[i]!=NULL) ? running_ops[i]->id : -1;
                FFLOG("MPI DUMP: %i op %u (%i/%i); count: %lu; cancelled: %i; source: %i; tag: %i; error: %i\n", inflight, opid, i, FFMPI_MAX_REQ, status._ucount, status._cancelled, status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR);
                inflight++;
            }
        }
#endif

        assert(0);
        return FFENOMEM;
    }

    running_ops[idx] = op;
    requests[idx] = req;
#endif
    return FFSUCCESS;
}

int ffop_mpi_progresser_release(uint32_t idx){
#ifdef FFPROGRESS_THREAD
    FFLOG("progresser is now UNtracking op %lu (ver: %u) with idx %u\n", running_ops[idx]->id, running_ops[idx]->version, idx);

    requests[idx] = MPI_REQUEST_NULL;
    running_ops[idx] = NULL;
    
    //now that we finished with idx we can release it
    ffarman_put(&index_manager, idx);
#endif 
    return FFSUCCESS;
}

int ffop_mpi_progresser_progress(ffqman_t *ready_list){

    int outcount, res;
    int ready_indices[FFMPI_MAX_REQ];
    res = MPI_Testsome(FFMPI_MAX_REQ, requests, &outcount, ready_indices, MPI_STATUS_IGNORE);
    //res = MPI_Waitsome(FFMPI_MAX_REQ, requests, &outcount, ready_indices, MPI_STATUS_IGNORE);

    if (res!=MPI_SUCCESS) {
        FFLOG_ERROR("MPI_Testsome returned with error!");
        return FFERROR;
    }
    
    for (int i=0; i<outcount; i++){
        ffop_t * readyop = running_ops[ready_indices[i]];

        /* mark the operation as complete */ 
        //FFOP_ENQUEUE(readyop, ready_list);
        ffqman_push(ready_list, readyop);

        ffop_mpi_progresser_release(ready_indices[i]);
    }
    
    return FFSUCCESS;
}


