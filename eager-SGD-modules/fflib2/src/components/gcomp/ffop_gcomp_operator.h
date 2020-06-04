#ifndef _FFOPERATOR_H_
#define _FFOPERATOR_H_

#include "ff.h"
#include "ffinternal.h"
#include "ffdatatype.h"

#define FFMAX_CUSTOM_OPERATORS 32

typedef struct ffop_gcomp_operator{
    uint32_t type;
    int commutative;
    int idx;
    ffoperator_fun_t op_fun;
} ffop_gcomp_operator_t;

int ffop_gcomp_operator_init();
int ffop_gcomp_operator_finalize();

int ffop_gcomp_operator_get(ffoperator_h opidx, ffop_gcomp_operator_t * opdescr);

int ffop_gcomp_operator_custom_create(ffoperator_fun_t fun, int commutative, ffoperator_h * handle);
int ffop_gcomp_operator_custom_delete(ffoperator_h opidx);


#endif /* _FFOPERATOR_H_ */
