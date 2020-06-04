
#include "ffop_gcomp_operator.h"
#include "ffop.h"
#include "ffinternal.h"
#include "fflocks.h"

int ffop_gcomp_init(ffop_t * op){
    FFLOG("init op: %p; op->comp: %p\n", op, &(op->comp));
    return ffop_gcomp_operator_get(op->comp.operator_type, &(op->comp.operator));
}


int ffop_gcomp_execute(ffop_t * op, ffbuffer_set_t * mem){
    
    ffcomp_t * comp = &(op->comp);

    FFLOG("Posting COMP %lu options: %u\n", op->id, op->options);
#ifdef CHECK_ARGS
    if (op==NULL || op->type!=FFCOMP) {
        FFLOG_ERROR("Invalid argument!");
        return FFINVALID_ARG;
    }

    CHECKBUFFER(comp->buffer1, mem);
    CHECKBUFFER(comp->buffer2, mem);
    CHECKBUFFER(comp->buffer3, mem);
#endif

    void *buffer1, *buffer2, *buffer3;
    uint32_t buff1_count, buff2_count, buff3_count;

    GETBUFFER(comp->buffer1, mem, buffer1);
    buff1_count = comp->buffer1->count;

    //buffer2 is optional for some operators
    if (comp->buffer2!=FFBUFF_NONE) {
        GETBUFFER(comp->buffer2, mem, buffer2);
        buff2_count = comp->buffer2->count;
    }else {
        buffer2 = NULL;
        buff2_count = buff1_count; //to fool the min below
    }

    GETBUFFER(comp->buffer3, mem, buffer3);
    buff3_count = comp->buffer3->count;

    if (IS_OPT_SET(op, FFCOMP_DEST_ATOMIC)){
        FFLOG("locking %p\n", &(comp->buffer3->lock));
        FFLOCK_LOCK(&(comp->buffer3->lock));
    }

    uint32_t size = MIN(buff1_count, buff2_count, buff3_count);
    ffdatatype_h datatype = comp->buffer1->datatype; // they are the same

    FFLOG("comp: %p; comp->operator.op_fun: %p;\n", comp, comp->operator.op_fun);
    int res = comp->operator.op_fun(buffer1, buffer2, buffer3, size, datatype);

    if (IS_OPT_SET(op, FFCOMP_DEST_ATOMIC)){
        FFLOG("unlocking %p\n", &(comp->buffer3->lock));
        FFLOCK_UNLOCK(&(comp->buffer3->lock));
    }


    if (res==FFSUCCESS){ return FFCOMPLETED; }   

    return res;
}


int ffop_gcomp_wait(ffop_t * op){
    return FFSUCCESS;
}

int ffop_gcomp_test(ffop_t * op, int * flag){
    *flag = 1;
    return FFSUCCESS;
}

int ffop_gcomp_cancel(ffop_t * op){
    return FFSUCCESS;
}

