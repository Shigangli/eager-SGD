#include "ffbuffer.h"
#include "ffstorage.h"

#include <assert.h>

#define INITIAL_FFBUFFER_POOL_COUNT 1024

static pool_h buff_pool;

int ffbuffer_create(void * addr, uint32_t count, ffdatatype_h datatype, int options, ffbuffer_h * _ffbuff){

    uint8_t selfalloc = addr==NULL;

    ffstorage_pool_get(buff_pool, (void **) _ffbuff);
    ffbuffer_t * ffbuff = *((ffbuffer_t **) _ffbuff); 

    FFLOG("Creating buffer: addr: %p; count: %u; datatype: %u, ptr: %p\n", addr, count, datatype, ffbuff);
    ffbuff->options = options;
    ffbuff->selfalloc = selfalloc;
    ffbuff->count = count;
    ffbuff->ptr = addr;
    ffbuff->datatype = datatype;

    if ((options & FFBUFFER_IDX) == FFBUFFER_IDX){  
        assert(!selfalloc);
        FFLOG("Creating IDX buffer!\n");
        ffbuff->type = FFBUFFER_IDX;
        ffbuff->idx  = *((uint32_t *) addr);
    }else{ 
        FFLOG("Creating PTR buffer!\n");
        ffbuff->type = FFBUFFER_PTR;
        ffbuff->datatype = FFDATATYPE_NONE;

        if (ffbuffer_resize(*_ffbuff, addr, count, datatype) != FFSUCCESS){
            ffstorage_pool_put(_ffbuff);
            return FFENOMEM;
        }
    }

    FFLOCK_INIT(&(ffbuff->lock));   
 
    return FFSUCCESS;
}

int ffbuffer_get_data(ffbuffer_h handle, void ** mem){
    ffbuffer_t * ffbuff = (ffbuffer_t *) handle;    
    assert((ffbuff->options && FFBUFFER_IDX) != FFBUFFER_IDX);
    assert(ffbuff->ptr!=NULL);
    *mem = ffbuff->ptr;
    return FFSUCCESS;
}

int ffbuffer_delete(ffbuffer_h _ffbuff){
    ffbuffer_t * ffbuff = (ffbuffer_t *) _ffbuff;    
    FFLOCK_DESTROY(&(ffbuff->lock));
    if (ffbuff->selfalloc) free(ffbuff->ptr);
    return ffstorage_pool_put(ffbuff);
}

int ffbuffer_get_size(ffbuffer_h handle, uint32_t * count, ffdatatype_h * datatype){
    ffbuffer_t * ffbuff = (ffbuffer_t *) handle;    
    *count = ffbuff->count;
    *datatype = ffbuff->datatype;
    return FFSUCCESS;
}

int ffbuffer_resize(ffbuffer_h handle, void * addr, uint32_t new_count, ffdatatype_h new_datatype){
    ffbuffer_t * ffbuff = (ffbuffer_t *) handle;    
    assert((ffbuff->options && FFBUFFER_IDX) != FFBUFFER_IDX);

    size_t new_unitsize, old_unitsize;
    ffdatatype_size(new_datatype, &new_unitsize);
    ffdatatype_size(ffbuff->datatype, &old_unitsize);
    FFLOG("Resizing buffers (addr: %p; count: %u; datatype: %u; datatype size: %lu; size: %lu)\n", ffbuff->ptr, new_count, new_datatype, new_unitsize, new_unitsize*new_count);

    if (addr==NULL){
        if (ffbuff->ptr==NULL || new_count * new_unitsize > ffbuff->count*old_unitsize){
            FFLOG("Realloc from %lu to %lu\n", old_unitsize * ffbuff->count, new_unitsize * new_count);
            ffbuff->ptr = realloc(ffbuff->ptr, new_unitsize*new_count);
            if (ffbuff->ptr==NULL) { return FFENOMEM; }
        }
    }else{
        FFLOG("Address has been provided: %p\n", addr);
        if (ffbuff->selfalloc) {
            FFLOG("Freeing old buffer address (selfalloc)");
            free(ffbuff->ptr);
        }
        ffbuff->ptr = addr;
    }

    ffbuff->count = new_count;
    ffbuff->datatype = new_datatype;

    return FFSUCCESS;
}

int ffbuffer_init(){
    buff_pool = ffstorage_pool_create(sizeof(ffbuffer_t), INITIAL_FFBUFFER_POOL_COUNT, NULL);
    return FFSUCCESS;
}

int ffbuffer_finalize(){
    return ffstorage_pool_destroy(buff_pool);
}
