#ifndef _FFBUFFER_H_
#define _FFBUFFER_H_

#include "ff.h"
#include "ffinternal.h"
#include "ffdatatype.h"


#define MAX_BUFFER_COUNT 3

#define CHECKBUFFER(BUFFDESCR, MEM) { \
    if (BUFFDESCR->type == FFBUFFER_IDX && (mem==NULL || \
            BUFFDESCR->idx > MEM->length)){ \
        FFLOG_ERROR("Invalid argument!"); \
        return FFINVALID_ARG; \
    } \
}

#define GETBUFFER(BUFFDESCR, MEM, BUFF) { \
    if (BUFFDESCR->type == FFBUFFER_PTR){ \
        BUFF = BUFFDESCR->ptr; \
    }else{ \
        BUFF = MEM->buffers[BUFFDESCR->idx]; \
    } \
}


typedef uint32_t ffbuffer_type_t;

typedef struct ffbuffer{
    ffbuffer_type_t type;
    FFLOCK_TYPE lock; 
 
    uint8_t selfalloc; 
 
    union{
        void * ptr;
        uint32_t idx;
    };

    uint32_t count;
    ffdatatype_t datatype;
    int options;
} ffbuffer_t;

typedef struct ffbuffer_set{
    /* ugly */
    void * buffers[MAX_BUFFER_COUNT];
    uint32_t length;

    struct ffmem_set * next;
} ffbuffer_set_t;


int ffbuffer_create(void * addr, uint32_t count, ffdatatype_t datatype, int options, ffbuffer_h * _ffbuff);
int ffbuffer_delete(ffbuffer_h ffbuff);

int ffbuffer_init();
int ffbuffer_finalize();



#endif /* _FFBUFFER_H_ */
