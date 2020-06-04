#include "ffcollectives.h"

typedef struct rand_allreduce_state{
    ffschedule_h activation_schedule;
    ffschedule_h allreduce_schedule;
    ffschedule_h sync_allreduce_schedule;
    ffop_h allreduce_ends;
    ffop_h allreduce_activation_test;  //op that needs to be tested to check if a local activation completed (two activations should not overlap)
    ffop_h activation_op;
    ffop_h activation_auto;
    uint32_t current_activator;
    uint32_t passive_activations;
    uint32_t myrank;
    uint32_t seed;
    uint32_t count;
    uint32_t async;
    uint32_t comm_size;
} rand_allreduce_state_t;

int ffrand_allreduce_post(ffschedule_h sched);
int ffrand_allreduce_wait(ffschedule_h sched);
int ffrand_allreduce_test(ffschedule_h sched, int * flag);
int ffrand_allreduce_delete(ffschedule_h sched);
int ffrand_allreduce_start(ffschedule_h sched);
int ffrand_allreduce_print(ffschedule_h handle, FILE * fp, char * name);

int ffrand_allreduce(void * sndbuff, void * rcvbuff, int count, int16_t tag, ffoperator_h operator, ffdatatype_h datatype, int options, int seed, int async, ffschedule_h * _sched){

    rand_allreduce_state_t * state = (rand_allreduce_state_t *) malloc(sizeof(rand_allreduce_state_t));

    ffschedule_h sched;
    FFCALL(ffschedule_create(&sched));
    ffschedule_set_state(sched, state);
 
    ffschedule_set_start_fun(sched, ffrand_allreduce_start);
    ffschedule_set_post_fun(sched, ffrand_allreduce_post);
    ffschedule_set_wait_fun(sched, ffrand_allreduce_wait);
    ffschedule_set_test_fun(sched, ffrand_allreduce_test);
    ffschedule_set_delete_fun(sched, ffrand_allreduce_delete);
    ffschedule_set_print_fun(sched, ffrand_allreduce_print);

    //create the activation schedule
    //activation_schedule_op is the op to post to activate the schedule internally
    ffop_h activation_schedule_test, activation_schedule_link, activation_schedule_op, activation_schedule_root, activation_join, activation_auto;
    ffactivation(FFSHADOW_TAG, tag, &activation_schedule_op, &activation_auto, &activation_schedule_test, &activation_join, &(state->activation_schedule));
    ffschedule_get_begin_op(state->activation_schedule, &activation_schedule_root);
    ffschedule_get_end_op(state->activation_schedule, &activation_schedule_link);
    
    //create the async allreduce schedule
    ffop_h allreduce_begin, allreduce_end;  
    ffallreduce(sndbuff, rcvbuff, count, tag, operator, datatype, options, &(state->allreduce_schedule));
    ffschedule_get_begin_op(state->allreduce_schedule, &allreduce_begin);
    ffschedule_get_end_op(state->allreduce_schedule, &allreduce_end);

    ffop_hb(activation_join, allreduce_begin, 0);
    ffop_hb(allreduce_end, activation_auto, FFDEP_IGNORE_VERSION);

    state->allreduce_activation_test = activation_schedule_test;
    state->allreduce_ends = allreduce_end;
    state->activation_op = activation_schedule_op;
    state->activation_auto = activation_auto;

    state->passive_activations = 0;
    state->seed = seed;
    state->count = 0;
    state->async = async;
    ffrank(&(state->myrank));
    ffsize(&(state->comm_size));

    //FIXME: TODO
    // 1) add multiple buffers

    *_sched = sched;
    return FFSUCCESS;  
}

int ffrand_allreduce_start(ffschedule_h sched){
    rand_allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);
    return ffop_post(state->activation_auto);
}

int ffrand_allreduce_post(ffschedule_h sched){
    rand_allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);
    state->count++;

    state->current_activator = rand_r(&(state->seed)) % state->comm_size;
    FFLOG("Activator: %u\n", state->current_activator);
    //if (state->current_activator == state->myrank || state->count % state->async == 0){
    if (state->current_activator == state->myrank){
        FFLOG("Catching up %u passive activations\n", state->passive_activations);
        for (int i=0; i<state->passive_activations; i++){
            ffop_post(state->activation_op);
            ffop_wait(state->allreduce_activation_test);
        }
        FFLOG("Done, posting the actual one!\n");
        state->passive_activations = 0;
        return ffop_post(state->activation_op);
    } 
    state->passive_activations++;
    return FFSUCCESS;
}

int ffrand_allreduce_wait(ffschedule_h sched){
    rand_allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);

    if (state->current_activator == state->myrank) {
        FFCALLV(ffop_wait(state->allreduce_activation_test), FFERROR);
    }
    return ffop_wait(state->allreduce_ends);
}

int ffrand_allreduce_test(ffschedule_h sched, int * flag){
    rand_allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);

    *flag=1;
    if (state->current_activator == state->myrank){
        ffop_test(state->allreduce_activation_test, flag);
    }

    if (*flag){
        ffop_test(state->allreduce_ends, flag);
    }
    return FFSUCCESS;
}   

int ffrand_allreduce_print(ffschedule_h handle, FILE * fp, char * name){
    rand_allreduce_state_t * state;
    ffschedule_get_state(handle, (void **) &state);

    ffschedule_print(state->activation_schedule, fp, "activation");
    ffschedule_print(state->allreduce_schedule, fp, "allreduce");
    ffschedule_default_print(handle, fp, name);

    return FFSUCCESS;
}

int ffrand_allreduce_delete(ffschedule_h sched){
    rand_allreduce_state_t * state;
    ffschedule_get_state(sched, (void **) &state);
    ffschedule_delete(state->activation_schedule);
    ffschedule_delete(state->allreduce_schedule);
    ffschedule_default_delete(sched); //to free the ops inside the schedule and the schedule itself
    free(state);
    //no need to delete the ops in state since they are part of the activation schedule
    return FFSUCCESS;
}
