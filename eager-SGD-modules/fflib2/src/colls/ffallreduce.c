#include "ffcollectives.h"
#include <assert.h>

#define FFALLREDUCE_REC_DOUBLING

typedef struct allreduce_state{
    ffbuffer_h * tmpbuffs;
    ffbuffer_h sndbuff;
    ffbuffer_h rcvbuff; 
    int count;
    ffdatatype_h datatype;
    int tmpbuffs_count;
    int free_sr_buff;
} allreduce_state_t;


#ifdef FFALLREDUCE_REC_DOUBLING
int counter = 0;

int ffallreduce_post(ffschedule_h sched){
    allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);
    assert(state!=NULL);    

    int sb_count, rb_count;
    ffdatatype_h sb_type, rb_type;
    int in_place = state->sndbuff == FFBUFF_NONE;

    if (!in_place) ffbuffer_get_size(state->sndbuff, &sb_count, &sb_type);
    ffbuffer_get_size(state->rcvbuff, &rb_count, &rb_type);

    if ((!in_place && sb_type!=rb_type) || rb_type!=state->datatype) {
        FFLOG("Datatype mismatch!\n");
        return FFINVALID_ARG;
    }

    if (!in_place && sb_count != rb_count){
        FFLOG("Size mismatch!\n");
        return FFINVALID_ARG;
    }

    if (rb_count > state->count){
        FFLOG("Resizing temp buffers!\n");
        for (int i=0; i<state->tmpbuffs_count; i++){
            ffbuffer_resize(state->tmpbuffs[i], NULL, rb_count, rb_type);
        }
        state->count = rb_count;
    }

    
    return ffschedule_default_post(sched);
}


int ffallreduce_delete(ffschedule_h sched){
    allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);
    assert(state!=NULL);    
    int in_place = state->sndbuff == FFBUFF_NONE;

    for (int i=0; i<state->tmpbuffs_count; i++){
        ffbuffer_delete(state->tmpbuffs[i]);
    }

    if (state->free_sr_buff){
        if (!in_place) ffbuffer_delete(state->sndbuff);
        ffbuffer_delete(state->rcvbuff);
    }
    
    return ffschedule_default_delete(sched);
}

//Recursive doubling
int ffallreduce(void * sndbuff, void * rcvbuff, int count, int16_t tag, ffoperator_h operator, ffdatatype_h datatype, int options, ffschedule_h * _sched){

    counter++;
    printf("Allreduce schedule is posted by %d times \n", counter);
    ffschedule_h sched;
    FFCALL(ffschedule_create(&sched));

    int csize, rank;
    ffsize(&csize);
    ffrank(&rank);

    int mask = 0x1;
    int maxr = (int)ceil((log2(csize)));
    int in_place = sndbuff == FFINPLACE;
    int buffers_provided = ((options & FFCOLL_BUFFERS) == FFCOLL_BUFFERS);

    size_t unitsize;
    ffdatatype_size(datatype, &unitsize);

    allreduce_state_t * state = (allreduce_state_t *) malloc(sizeof(allreduce_state_t));
   
    state->tmpbuffs = (ffbuffer_h *) malloc(sizeof(ffbuffer_h)*(maxr));
    for (int i=0; i<maxr; i++){
        FFLOG("Allocating tmp buff %i; count: %i; datatype: %u\n", i, count, datatype);
        ffbuffer_create(NULL, count, datatype, 0, &(state->tmpbuffs[i]));
    }
    state->tmpbuffs_count = maxr;
    state->datatype = datatype;
    state->count = count;

    if (buffers_provided){
        FFLOG("Buffers are provided by the user\n");
        if (!in_place) state->sndbuff = *((ffbuffer_h *) sndbuff);
        else state->sndbuff = FFBUFF_NONE;
        state->rcvbuff = *((ffbuffer_h *) rcvbuff);
        state->free_sr_buff = 0;
        ffschedule_set_post_fun(sched, ffallreduce_post);
    }else{
        FFLOG("Allocating buffers\n");
        if (!in_place) ffbuffer_create(sndbuff, count, datatype, 0, &(state->sndbuff));
        else state->sndbuff = FFBUFF_NONE;
        ffbuffer_create(rcvbuff, count, datatype, 0, &(state->rcvbuff));
        state->free_sr_buff = 1;
    }

    ffop_h move;

    ffbuffer_h sb = state->sndbuff;
    ffbuffer_h rb = state->rcvbuff;
    FFLOG("sndbuff: %p; rcvbuff: %p\n", sb, rb);

    //copy from sb to rb
    if (!in_place){
        ffcomp_b(sb, FFBUFF_NONE, FFIDENTITY, FFCOMP_DEST_ATOMIC, rb, &move);
    }else{
        ffnop(0, &move);      
    }

    ffschedule_set_state(sched, (void *) state);
    ffschedule_set_delete_fun(sched, ffallreduce_delete);

    ffop_h send=FFNONE, recv=FFNONE, prev_send=FFNONE, comp=FFNONE;
    uint32_t r=0;
    comp = move;
    while (mask < csize) {
        uint32_t dst = rank^mask;
        if (dst < csize) {

            assert(r<maxr);

            //send the data in recv buffer
            ffsend_b(rb, dst, tag, 0, &send);  

            //before sending we have to wait for the computation (or move)
            ffop_hb(comp, send, 0);


            //receive data to tmp buffer
            ffrecv_b(state->tmpbuffs[r], dst, tag, 0, &recv);            
 
            //accumulate data in tmp buffer and store in recv buffer
            ffcomp_b(state->tmpbuffs[r], rb, operator, FFCOMP_DEST_ATOMIC, rb, &comp);    

            //the next comp has to wait this send (they share the recv buffer)
            ffop_hb(send, comp, 0);
            prev_send = send;            
    
            //comp has to wait the receive to happen (they share the tmp buffer)
            ffop_hb(recv, comp, 0);    

            ffschedule_add_op(sched, send);
            ffschedule_add_op(sched, recv);
            ffschedule_add_op(sched, comp);   
            
            r++; 
        }
        mask <<= 1;
    }

    ffschedule_add_op(sched, move);

    *_sched = sched;
    return FFSUCCESS;
}

#else

#define TMPMEM(MEM, TYPE, BSIZE, OFF) (void *) &(((uint8_t *) MEM)[((BSIZE)*(OFF))])
int ffallreduce(void * sndbuff, void * rcvbuff, int count, int tag, ffoperator_h operator, ffdatatype_h datatype, ffschedule_h * _sched){
 
    ffschedule_h sched;
    FFCALL(ffschedule_create(&sched));

    int p, rank;
    ffsize(&p);
    ffrank(&rank);
    int maxr = (int)ceil((log2(p)));
    int vpeer, vrank, peer, root=0;

    size_t unitsize;
    ffdatatype_size(datatype, &unitsize);

    RANK2VRANK(rank, vrank, root);

    int nchild = ceil(log(vrank==0 ? p : vrank)) + 1;
    
    void * tmpmem = malloc(nchild*count*unitsize);
    ffschedule_set_tmpmem(sched, tmpmem);

    //TODO: MAKE COLLECTIVE TAG

    // reduce part 
    ffop_h send_up;
    ffnop(0, &send_up);

    ffop_h move;
    ffcomp(sndbuff, NULL, count, datatype, FFIDENTITY, 0, rcvbuff, &move);

    ffop_h comp=FFNONE, recv=FFNONE;

    for(int r=1; r<=maxr; r++) {
        if((vrank % (1<<r)) == 0) {
            /* we have to receive this round */
            vpeer = vrank + (1<<(r-1));
            VRANK2RANK(peer, vpeer, root)

            if(peer<p) {

                FFLOG("nchild: %i; tmpmem: %p; r: %i; peer: %i; count: %i; unitsize: %lu\n", nchild, TMPMEM(tmpmem, datatype, count*unitsize, r-1), r, peer, count, unitsize);

                assert(nchild > r-1);

                //Receive from the peer
                ffrecv(TMPMEM(tmpmem, datatype, count*unitsize, r-1), count, datatype, peer, tag, 0, &recv); 
             
                //accumulate
                ffcomp(TMPMEM(tmpmem, datatype, count*unitsize, r-1), rcvbuff, count, datatype, operator, FFCOMP_DEST_ATOMIC, rcvbuff, &comp); 

                //we need to receive before start computing
                ffop_hb(recv, comp, 0);

                //wait for the rcvbuff (our accumulator) to be ready before sending
                ffop_hb(move, comp, 0);

                //we need to compute everything before sending
                ffop_hb(comp, send_up, 0);
           
                ffschedule_add_op(sched, recv); 
                ffschedule_add_op(sched, comp);   
            }
        }else{
            ffop_h send;
       
            /* we have to send this round */
            vpeer = vrank - (1<<(r-1));
            VRANK2RANK(peer, vpeer, root)

            //send
            ffsend(rcvbuff, count, datatype, peer, tag, 0, &send);
    
            //receive & reduce data from children before sending it up
            ffop_hb(send_up, send, 0);
            
            ffschedule_add_op(sched, send);

            break;
        }
    }
 
    if (recv==FFNONE) ffop_hb(move, send_up, 0); 
    ffschedule_add_op(sched, move);

    // broadcast
    RANK2VRANK(rank, vrank, root);

    recv = FFNONE;
    ffop_h recv_before_send;
    ffnop(0, &recv_before_send);

    /* receive from the right hosts  */
    if(vrank != 0) {
        for(int r=0; r<maxr; r++) {
            if((vrank >= (1<<r)) && (vrank < (1<<(r+1)))) {
                VRANK2RANK(peer, vrank-(1<<r), root);

                //recv
                ffrecv(rcvbuff, count, datatype, peer, tag, 0, &recv);               
    
                ffop_hb(recv, recv_before_send, 0);

                ffschedule_add_op(sched, recv);
            }
        }
    }

    // at the root we need to wait to receive before sending down
    if (recv == FFNONE) {
        ffop_hb(send_up, recv_before_send, 0);
    }

    // now send to the right hosts 
    for(int r=0; r<maxr; r++) {
        if(((vrank + (1<<r) < p) && (vrank < (1<<r))) || (vrank == 0)) {
            VRANK2RANK(peer, vrank+(1<<r), root);

            ffop_h send;
    
            //send
            ffsend(rcvbuff, count, datatype, peer, tag, 0, &send);

            ffop_hb(recv_before_send, send, 0);

            ffschedule_add_op(sched, send);
        }
    }

    ffschedule_add_op(sched, send_up);
    ffschedule_add_op(sched, recv_before_send);

    *_sched = sched;

    return FFSUCCESS;
}

#endif
