#include "ffcollectives.h"
#include "ffinternal.h"
#include <assert.h>

int ffactivation_delete(ffschedule_h sched){
    int * buff;
    ffschedule_get_state(sched, (void **) &buff);
    free(buff);
}

int ffactivation(int options, int tag, ffop_h *user_activator, ffop_h *auto_activator, ffop_h * user_activator_test, ffop_h *activation_join, ffschedule_h *_sched){
    ffschedule_h sched;
    FFCALL(ffschedule_create(&sched));

    int csize, rank;
    ffsize(&csize);
    ffrank(&rank);

    ffop_h completion, sched_activation;

    int * buff = malloc(sizeof(int));

    int maxr = (int)ceil((log2(csize)));
    ffop_h * recvs = (ffop_h *) malloc(sizeof(ffop_h)*maxr);
    ffop_h * completions = (ffop_h *) malloc(sizeof(ffop_h)*maxr);
    
    ffschedule_set_state(sched, (void *) buff);
    ffschedule_set_delete_fun(sched, ffactivation_delete);

    ffop_h sched_begin_op;
    ffschedule_get_begin_op(sched, &sched_begin_op);   

    ffnop(FFOP_DEP_OR, user_activator); 
    //ffop_hb(sched_begin_op, *user_activator, FFDEP_NO_AUTOPOST);

    ffnop(FFOP_DEP_OR, auto_activator);

    ffop_h prev_dep = *user_activator;
    int mask = 0x1, cnt=0;
    while (mask < csize) {
        uint32_t dst = rank^mask;
        if (dst < csize) {
            ffop_h send, recv, completion;

            //comp 
            ffnop(0, &completion);
            if (prev_dep!=FFNONE) ffop_hb(prev_dep, completion, 0);

            //send
            ffdep_op_h send_to_user_activation_test = FFNONE;
            ffsend(buff, 1, FFINT32, dst, tag, FFOP_DEP_OR | FFOP_COMPLETE_BEFORE_CANCELLING | options, &send);
            for (int i=0; i<cnt; i++){
                //ffop_hb(recvs[i], send, FFDEP_IGNORE_VERSION);
                ffdep_op_h send_to_completion;
                ffop_hb(recvs[i], send, 0);
                
                ffop_hb_fallback(send, completions[i], FFNONE, FFDEP_SKIP_OLD_VERSIONS, &send_to_completion);
                if (i==0) send_to_user_activation_test = send_to_completion;
            }            
            //ffop_hb(*user_activator, send, FFDEP_IGNORE_VERSION);
            ffop_hb(send, completion, FFDEP_SKIP_OLD_VERSIONS);

            //recv
            ffrecv(buff, 1, FFINT32, dst, tag, options, &recv);
            ffop_hb(*auto_activator, recv, 0);

            recvs[cnt] = recv;
            completions[cnt] = completion;
            cnt++;
            prev_dep = recv;
            ffdep_op_h user_activation_to_send;
            ffop_hb_fallback(*user_activator, send, completions[0], 0, &user_activation_to_send);
            if (send_to_user_activation_test!=FFNONE) ffdep_set_parent(send_to_user_activation_test, user_activation_to_send);

            ffschedule_add_op(sched, send);
        }
        mask <<= 1;
    }

    if (cnt>0) *user_activator_test = completions[0];

    ffschedule_add_op(sched, *user_activator);
    ffschedule_add_op(sched, *auto_activator);

    ffop_h sched_completion;
    ffnop(FFOP_DEP_OR, &sched_completion);

    *activation_join = sched_completion;
    ffop_hb(recvs[cnt-1], sched_completion, 0);

    for (int i=0; i<cnt; i++){     
        ffop_hb(completions[i], sched_completion, 0);
        //this thing that we need to add the ops to the sched once all the deps are satisfied sucks!!!
        //FIXME: ffop_hb should be able to update the schedule (if it exists)
        ffschedule_add_op(sched, recvs[i]);
        ffschedule_add_op(sched, completions[i]);
    }
    ffschedule_add_op(sched, sched_completion);

    *_sched = sched;

    free(recvs);
    free(completions);

    return FFSUCCESS;
}
