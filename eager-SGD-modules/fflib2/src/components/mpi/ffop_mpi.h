#ifndef _FFOP_MPI_
#define _FFOP_MPI_

#include <mpi.h>

#include "ff.h"
#include "ffbuffer.h"

typedef struct ffop ffop_t;

/* Max outstanding MPI operations */
//#define FFMPI_MAX_REQ 4096
#define FFMPI_MAX_REQ 16384

int ffop_mpi_init(ffop_t * op);

int ffop_mpi_send_execute(ffop_t * op, ffbuffer_set_t * mem);
int ffop_mpi_send_wait(ffop_t * op);
int ffop_mpi_send_test(ffop_t * op, int * flag);
int ffop_mpi_send_cancel(ffop_t * op);

int ffop_mpi_recv_execute(ffop_t * op, ffbuffer_set_t * mem);
int ffop_mpi_recv_wait(ffop_t * op);
int ffop_mpi_recv_test(ffop_t * op, int * flag);
int ffop_mpi_recv_cancel(ffop_t * op);

extern MPI_Datatype datatype_translation_table[FFDATATYPE_SENTINEL];

#endif /* _FFOP_MPI_ */
