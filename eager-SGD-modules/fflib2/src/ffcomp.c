
#include "ff.h"
#include "ffop.h"
#include "ffinternal.h"
#include "ffcomp.h"

int ffcomp(void * addr1, void * addr2, int count, ffdatatype_h datatype, ffoperator_h ffoperator, int options, void * addr3, ffop_h * ophandle){

    ffbuffer_h b1, b2, b3;

    FFCALL(ffbuffer_create(addr1, count, datatype, options, &b1));
    FFCALL(ffbuffer_create(addr2, count, datatype, options, &b2));
    FFCALL(ffbuffer_create(addr3, count, datatype, options, &b3));

    return ffcomp_b(b1, b2, ffoperator, options, b3, ophandle);
}

int ffcomp_b(ffbuffer_h buffer1, ffbuffer_h buffer2, ffoperator_h ffoperator, int options, ffbuffer_h buffer3, ffop_h * ophandle){

    int res;
    ffop_t * op;
    ffop_create(&op);

    *ophandle = (ffop_h) op;    

    op->type = FFCOMP;
    op->options ^= options; 

    op->comp.buffer1 = (ffbuffer_t *) buffer1;
    op->comp.buffer2 = (ffbuffer_t *) buffer2;
    op->comp.buffer3 = (ffbuffer_t *) buffer3;

    FFLOG("FFCOMP ID: %lu (%p); buffer1: %p; count: %i; datatype: %i; operator: %i; options: %i\n", op->id, &(op->comp), op->comp.buffer1, op->comp.buffer1->count, op->comp.buffer1->datatype, ffoperator, options);

    op->comp.operator_type = ffoperator;
    return ff.impl.ops[FFCOMP].init(op);
    
}

int ffcomp_operator_create(ffoperator_fun_t fun, int commutative, ffoperator_h * handle){
    return ff.impl.operator_create(fun, commutative, handle);
}

int ffcomp_operator_delete(ffoperator_h handle){
    return ff.impl.operator_delete(handle);
}

int ffcomp_tostring(ffop_t * op, char * str, int len){
    snprintf(str, len, "%lu.C", op->id); 
    return FFSUCCESS;
}


int ffcomp_finalize(ffop_t * op){
    ffbuffer_finalize(&(op->comp.buffer1));
    ffbuffer_finalize(&(op->comp.buffer2));
    ffbuffer_finalize(&(op->comp.buffer3));

    return FFSUCCESS;
}

