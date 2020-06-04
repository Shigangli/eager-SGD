#ifndef _FFSEND_H_
#define _FFSEND_H_

#include "ff.h"
#include "ffinternal.h"
#include "ffbuffer.h"
#include "ffbind.h"

typedef struct ffsend{    
    ffpeer_t peer;
    ffbuffer_t * buffer;
    uint8_t tag;
    uint8_t tag_type;
    
    uint8_t flags;

    /* transport */
    ffimpl_send_data_t transport;

} ffsend_t;

int ffsend_post(ffop_t * op, ffbuffer_set_t * mem);

int ffsend_tostring(ffop_t * op, char * str, int len);

int ffsend_finalize(ffop_t * op);


#endif /* _FFSEND_H_ */
