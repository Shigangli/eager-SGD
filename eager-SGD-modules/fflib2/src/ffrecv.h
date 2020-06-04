#ifndef _FFRECV_H_
#define _FFRECV_H_

#include "ff.h"
#include "ffinternal.h"
#include "ffbuffer.h"
#include "ffbind.h"

typedef struct ffrecv{    
    ffpeer_t peer;
    ffbuffer_t * buffer;
    uint8_t tag;
    uint8_t flags;
    uint8_t tag_type;

    /* transport */
    ffimpl_recv_data_t transport;

} ffrecv_t;


int ffrecv_post(ffop_t * op, ffbuffer_set_t * mem);

int ffrecv_tostring(ffop_t * op, char * str, int len);

int ffrecv_finalize(ffop_t * op);

#endif /* _FFRECV_H_ */

