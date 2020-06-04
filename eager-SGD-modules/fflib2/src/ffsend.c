#include "ffsend.h"
#include "ffop.h"

int ffsend(void * addr, int count, ffdatatype_h datatype, int dest, int8_t tag, 
    int options, ffop_h * op){
 
    ffbuffer_h buff;
    FFCALL(ffbuffer_create(addr, count, datatype, options, &buff));
 
    return ffsend_b(buff, dest, tag, options, op);   
}

int ffsend_b(ffbuffer_h buffer, int dest, int8_t tag, int options, ffop_h *_op){

    int res;   
    ffop_t * op;
    ffop_create(&op);
    *_op = (ffop_h) op;

    op->type = FFSEND;
    op->options ^= options;

    op->send.peer = dest;
    op->send.tag = tag;
    op->send.tag_type = IS_OPT_SET(op, FFSHADOW_TAG); 

    op->send.buffer = (ffbuffer_t *) buffer;

    FFLOG("FFSEND ID: %lu; dest: %i; count: %i; datatype: %i; tag: %hd; options: %i\n", op->id, dest, op->send.buffer->count, op->send.buffer->datatype, tag, options);

    /* implementation specific */
    res = ff.impl.ops[FFSEND].init(op);    
    
    return res;
}

int ffsend_tostring(ffop_t * op, char * str, int len){
    snprintf(str, len, "%lu.S(%i)", op->id, op->send.peer); 
    return FFSUCCESS;
}


int ffsend_finalize(ffop_t * op){

    ffbuffer_finalize(&(op->send.buffer));
    return FFSUCCESS;
}
