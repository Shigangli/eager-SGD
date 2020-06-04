#include "ffop_scheduler.h"
#include "utils/ffqman.h"
#include "ffop.h"

static ffqman_t to_schedule;

int ffop_scheduler_init(){
    return ffqman_create(&to_schedule);
}

int ffop_scheduler_finalize(){
    return ffqman_free(&to_schedule);
}

int ffop_scheduler_schedule(ffop_t * op){
    return ffqman_push(&to_schedule, (void *) op);
}

int ffop_scheduler_execute_all(){
    ffop_t * to_execute = NULL;
    ffqman_pop(&to_schedule, (void **) &to_execute);
    while(to_execute!=NULL){
        ffop_execute(to_execute);
        ffqman_pop(&to_schedule, (void **) &to_execute);
    }
    return FFSUCCESS;
}

