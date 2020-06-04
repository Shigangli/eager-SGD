#include "ff.h"
#include "ffinternal.h"
#include "ffop_gcomp_operator.h"
#include "utils/ffarman.h"

#include <string.h>

#define BLOCK 1024

#define SUM(TYPE, A, B, C, SIZE)                            \
{                                                           \
    for (uint32_t i = 0; i<SIZE; i++){                      \
        ((TYPE *)C)[i] = ((TYPE *)A)[i] + ((TYPE *)B)[i];   \
    }                                                       \
}

#define VSUM(TYPE, A, B, C, SIZE)                                                                                           \
{                                                                                                                           \
    uint32_t to_reduce = SIZE;                                                                                              \
    while (to_reduce >= BLOCK){                                                                                             \
        SUM(TYPE, &(((TYPE *) A)[SIZE-to_reduce]), &(((TYPE *)B)[SIZE-to_reduce]), &(((TYPE *) C)[SIZE-to_reduce]), BLOCK); \
        to_reduce -= BLOCK;                                                                                                 \
    }                                                                                                                       \
    SUM(TYPE, &(((TYPE *)A)[SIZE-to_reduce]), &(((TYPE *)B)[SIZE-to_reduce]), &(((TYPE *)C)[SIZE-to_reduce]), to_reduce);   \
}


static ffop_gcomp_operator_t operators[FFOPERATOR_SENTINEL];
static ffop_gcomp_operator_t custom_operators[FFMAX_CUSTOM_OPERATORS];
static ffarman_t custom_idx;


int ffop_gcomp_operator_sum(void * a, void * b, void* c, uint32_t size, ffdatatype_h type){
    switch (type){
        case FFINT32:{ 
            FFLOG("(a: %p; b: %p; c: %p; a[1]: %i; b[1]: %i; c[1]: %i; size: %u)!\n", a, b, c, ((int32_t *)a)[0], ((int32_t *)b)[0], ((int32_t *)c)[0], size);

            VSUM(int32_t, a, b, c, size);

            FFLOG("(a: %p; b: %p; c: %p; a[1]: %i; b[1]: %i; c[1]: %i; size: %u)!\n", a, b, c, ((int32_t *)a)[0], ((int32_t *)b)[0], ((int32_t *)c)[0], size);
            break;
        }
        case FFINT64:
            VSUM(int64_t, a, b, c, size);
            break;
        case FFDOUBLE:
            VSUM(double, a, b, c, size);
            break;
        case FFFLOAT:
            VSUM(float, a, b, c, size);
            break;
        default:
            FFLOG_ERROR("Operator not found!\n");
            return FFINVALID_ARG;         
    }

    return FFSUCCESS;
}


int ffop_gcomp_operator_copy(void * a, void * b, void* c, uint32_t size, ffdatatype_h type){
    size_t unitsize;
    ffdatatype_size(type, &unitsize);

    int rank;
    ffrank(&rank);
    //printf("[%i] [DELETE ME] COPYING %u x %lu from %p to %p\n", rank, size, unitsize, a, c);
    
    FFLOG("COPYING %u x %lu from %p to %p\n", size, unitsize, a, c);
    memcpy(c, a, size*unitsize);
    return FFSUCCESS;
}


int ffop_gcomp_operator_init(){

    operators[FFSUM].type = FFSUM;
    operators[FFSUM].commutative = 1;
    operators[FFSUM].idx = FFSUM;
    operators[FFSUM].op_fun = ffop_gcomp_operator_sum;

    operators[FFIDENTITY].type = FFIDENTITY;
    operators[FFIDENTITY].commutative = 1;
    operators[FFIDENTITY].idx = FFIDENTITY;
    operators[FFIDENTITY].op_fun = ffop_gcomp_operator_copy;

    FFLOG("identity fun ptr: %p\n", operators[FFIDENTITY].op_fun);

    if (ffarman_create(FFMAX_CUSTOM_OPERATORS, &custom_idx)!=FFSUCCESS){
        return FFERROR;
    }    

    return FFSUCCESS;
}

int ffop_gcomp_operator_finalize(){
    return FFSUCCESS;
}

int ffop_gcomp_operator_get(ffoperator_h opidx, ffop_gcomp_operator_t * opdescr){
    
    FFLOG("opidx: %u; opdescr: %p\n", opidx, opdescr);
    if (opidx<0) return FFINVALID_ARG;

    /* copy the descriptor */
    if (opidx < FFOPERATOR_SENTINEL){
        *opdescr = operators[opidx]; 
        FFLOG("op_fun: %p\n", opdescr->op_fun);
        return FFSUCCESS;
    }

    opidx = opidx - FFCUSTOM;
    if (opidx < FFMAX_CUSTOM_OPERATORS){
        *opdescr = custom_operators[opidx];
        FFLOG("op_fun: %p\n", opdescr->op_fun);
        return FFSUCCESS;
    }

    FFLOG("Error: no operator found!\n");
    return FFINVALID_ARG;
}


int ffop_gcomp_operator_custom_create(ffoperator_fun_t fun, int commutative, ffoperator_h * handle){


    uint32_t idx = ffarman_get(&custom_idx);

    if (idx<0){
        FFLOG_ERROR("Too many custom operators! (check FFMAX_CUSTOM_OPERATORS)");
        return FFENOMEM;
    }

    custom_operators[idx].type = FFCUSTOM;
    custom_operators[idx].idx = idx;
    custom_operators[idx].commutative = commutative;
    custom_operators[idx].op_fun = fun;

    *handle = idx + FFCUSTOM;

    return FFSUCCESS;
}

int ffop_gcomp_operator_custom_delete(ffoperator_h opidx){
    
    opidx = opidx - FFCUSTOM;
    if (opidx >= FFMAX_CUSTOM_OPERATORS || custom_operators[opidx].type == FFNONE) return FFERROR;
    custom_operators[opidx].type = FFNONE;   

    ffarman_put(&custom_idx, opidx);
    
    return FFSUCCESS;
}

