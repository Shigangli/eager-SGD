#include "ffcallback.h"
#include "ffop.h"

int ffcallback(ffcb_fun_t cb, void * arg, int options, ffop_h * _op){

    ffop_t * op;
    ffop_create(&op);
    *_op = (ffop_h) op;

    op->type = FFCALLBACK;
    op->options ^= options;    
    
    op->callback.cb = cb;
    op->callback.arg =  arg;

    return FFSUCCESS;
}

int ffcallback_execute(ffop_t * op, ffbuffer_set_t * mem){
    FFLOG("FFCALLBACK ID: %lu\n", op->id);
    op->callback.cb((ffop_h) op, op->callback.arg);
    return FFCOMPLETED;
}

int ffcallback_tostring(ffop_t * op, char * str, int len){
    snprintf(str, len, "F.%lu", op->id);
    return FFSUCCESS;
}

int ffcallback_finalize(ffop_t * op){
    return FFSUCCESS;
}
