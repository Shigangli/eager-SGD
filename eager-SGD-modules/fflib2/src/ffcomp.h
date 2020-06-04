#ifndef _FFCOMP_H_
#define _FFCOMP_H_

#include "ff.h"
#include "ffinternal.h"
#include "ffbuffer.h"
#include "ffbind.h"

typedef struct ffcomp{
    ffbuffer_t * buffer1;
    ffbuffer_t * buffer2;
    ffbuffer_t * buffer3;

    ffoperator_h operator_type; 

    ffimpl_comp_data_t operator;

} ffcomp_t;

int ffcomp_tostring(ffop_t * op, char * str, int len);

int ffcomp_finalize(ffop_t * op);


#endif /* _FFCOMP_H_ */
