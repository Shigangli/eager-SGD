#include "ffop_default_progresser.h"
#include "utils/ffqman.h"
#include "ffop.h"

static ffqman_t ready_ops;

int ffop_default_progresser_init(){
    return ffqman_create(&ready_ops);
}

int ffop_default_progresser_finalize(){
    return ffqman_free(&ready_ops);
}

int ffop_default_progresser_track(ffop_t * op){
    return ffqman_push(&ready_ops, (void *) op);
}

int ffop_default_progresser_progress(ffqman_t * ready_list){
    ffop_t * readyop = NULL;
    ffqman_pop(&ready_ops, (void **) &readyop);
    while (readyop!=NULL){
        //FFLOG("Default progresser found op %lu completed\n", readyop->id);
        //FFOP_ENQUEUE(readyop, ready_list);      
        ffqman_push(ready_list, readyop);
        ffqman_pop(&ready_ops, (void **) &readyop);
    }
    return FFSUCCESS;
}

