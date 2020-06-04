#ifndef _FFMPI_TYPES_H_
#define _FFMPI_TYPES_H_

#include <mpi.h>

struct ffimpl_data {
    MPI_Request mpireq;
    uint32_t pidx; // index in MPI progresser
};

typedef struct ffimpl_data ffimpl_send_data_t;
typedef struct ffimpl_data ffimpl_recv_data_t;

#endif /* _FFMPI_TYPES_H_ */
