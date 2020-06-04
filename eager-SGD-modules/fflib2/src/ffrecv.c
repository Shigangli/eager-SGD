#include "ffrecv.h"
#include "ffop.h"

int ffrecv(void * addr, int count, ffdatatype_h datatype, int source, int8_t tag, 
    int options, ffop_h * op){

    ffbuffer_h buff;
    FFCALL(ffbuffer_create(addr, count, datatype, options, &buff));
 
    return ffrecv_b(buff, source, tag, options, op);   
}

int ffrecv_b(ffbuffer_h buffer, int source, int8_t tag, int options, ffop_h * _op){

    int res; 
    ffop_t * op;
    ffop_create(&op);
    *_op = (ffop_h) op;

    op->type = FFRECV;
    op->options ^= options;

    op->recv.peer = source;
    op->recv.tag = tag;
    op->recv.tag_type = IS_OPT_SET(op, FFSHADOW_TAG); 
 
    op->recv.buffer = (ffbuffer_t *) buffer;

    FFLOG("FFRECV ID: %lu; source: %i; count: %i; datatype: %i; tag: %hd; options: %i\n", op->id, source, op->recv.buffer->count, op->recv.buffer->datatype, tag, options);

    /* implementation specific */   
    res = ff.impl.ops[FFRECV].init(op);

    return res;
}

int ffrecv_tostring(ffop_t * op, char * str, int len){
    snprintf(str, len, "%lu.R(%i)", op->id, op->recv.peer); 
    return FFSUCCESS;
}

int ffrecv_finalize(ffop_t * op){
    ffbuffer_finalize(&(op->recv.buffer));
    return FFSUCCESS;
}
