
#include "ffop_mpi.h"
#include "ffop_mpi_progresser.h"
#include "ffsend.h"

int ffop_mpi_send_execute(ffop_t * op, ffbuffer_set_t * mem){
    int res;

    ffsend_t * send = &(op->send);

#ifdef CHECK_ARGS
    if (op==NULL || op->type!=FFSEND) {
        FFLOG_ERROR("Invalid argument!");
        return FFINVALID_ARG;
    }

    CHECKBUFFER(send->buffer, mem);

#endif

    void * buffer;
    GETBUFFER(send->buffer, mem, buffer)

    //uint32_t tag =  (send->tag << 16) | ((op->version & TAG_VERSION_MASK) << 1) | (send->tag_type & TAG_TYPE_MASK);

    //Cray MPI has 21bits tags
    // user tag : 8bits;
    // version  : 12bits;
    // shadow   : 1bit.
    uint32_t tag =  (send->tag << 13)  | ((op->version & TAG_VERSION_MASK) << 1) | (send->tag_type & TAG_TYPE_MASK);

#ifdef DEBUG
    if (op->version != (op->version & TAG_VERSION_MASK)){
        FFLOG("BIG WARNING: OP version tag is wrapping around!!!\n");
    }
#endif

    FFLOG("MPI_Isend count: %u; datatype: %u; dest: %u; user tag: %hu; op->version: %u (masked: %u); real tag: %u buffer: %p\n", send->buffer->count, send->buffer->datatype, send->peer, send->tag, op->version, (uint32_t) ((op->version & TAG_VERSION_MASK) << 1), tag, buffer);
    res = MPI_Isend(buffer, send->buffer->count, 
            datatype_translation_table[send->buffer->datatype], send->peer, 
            tag, MPI_COMM_WORLD, &(send->transport.mpireq));

    if (res!=MPI_SUCCESS) {
        FFLOG_ERROR("Error while creating the MPI_Isend!");
        return FFERROR;
    }

    return ffop_mpi_progresser_track(op, send->transport.mpireq, &(send->transport.pidx));
}

int ffop_mpi_send_wait(ffop_t * op){
    MPI_Wait(&(op->send.transport.mpireq), MPI_STATUS_IGNORE);
    return FFSUCCESS;
}

int ffop_mpi_send_test(ffop_t * op, int * flag){
    MPI_Test(&(op->send.transport.mpireq), flag, MPI_STATUS_IGNORE);
    return FFSUCCESS;
}

int ffop_mpi_send_cancel(ffop_t * op){
    ffsend_t * send = &(op->send);

    FFLOG("Cancelling MPI_Isend (op: %lu)!\n", op->id);

    ffop_mpi_progresser_release(send->transport.pidx);
    
    MPI_Cancel(&(send->transport.mpireq));

    //a cancelled op needs to be waited for
    MPI_Wait(&(send->transport.mpireq), MPI_STATUS_IGNORE); 

    return FFSUCCESS;
}
