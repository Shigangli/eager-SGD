#include "ffinternal.h"
#include "ffdatatype.h"

int ffdatatype_size(ffdatatype_h datatype, size_t * size){
    size_t unitsize;

    switch (datatype){
        case FFINT32: 
            unitsize = sizeof(int32_t);
            break;
        case FFINT64:
            unitsize = sizeof(int64_t);
            break;
        case FFDOUBLE:
            unitsize = sizeof(double);
            break;
        case FFFLOAT:
            unitsize = sizeof(float);
            break;
        case FFDATATYPE_NONE:
            unitsize = 0;
            break;
        default:
            FFLOG_ERROR("Operator not found!\n");
            return FFINVALID_ARG;         
    }
    
    *size = unitsize;

    return FFSUCCESS;
}

