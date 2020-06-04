#ifndef _FFCALLBACK_H_
#define _FFCALLBACK_H_

#include "ffbuffer.h"
#include "ffinternal.h"

typedef void (*ffcb_fun_t)(ffop_h op, void * arg);

typedef struct ffcallback{
    ffcb_fun_t cb;
    void * arg;
} ffcallback_t;

int ffcallback_execute(ffop_t * op, ffbuffer_set_t * mem);
int ffcallback_tostring(ffop_t * op, char * str, int len);
int ffcallback_finalize(ffop_t * op);

#endif /* _FFCALLBACK_H_ */
