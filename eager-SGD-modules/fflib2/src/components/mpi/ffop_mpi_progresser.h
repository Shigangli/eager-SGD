#ifndef _FFOP_MPI_PROGRESSER_H_
#define _FFOP_MPI_PROGRESSER_H_

#include "ffop_mpi.h"
#include "ffop.h"
#include "utils/ffqman.h"

int ffop_mpi_progresser_init();
int ffop_mpi_progresser_finalize();

int ffop_mpi_progresser_track(ffop_t * op, MPI_Request req, uint32_t * idx);
int ffop_mpi_progresser_release(uint32_t idx);
int ffop_mpi_progresser_progress(ffqman_t *ready_list);


#endif /* _FFOP_MPI_PROGRESSER_H_ */


