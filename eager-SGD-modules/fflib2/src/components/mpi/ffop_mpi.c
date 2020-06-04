
#include "ffop_mpi.h"

MPI_Datatype datatype_translation_table[FFDATATYPE_SENTINEL] = { 
    MPI_INT, /* FFINT32 */ 
    MPI_LONG_LONG, /* FFINT64 */
    MPI_DOUBLE, /* FFDOUBLE */
    MPI_FLOAT, /* FFFLOAT */
    MPI_CHAR /* FFCHAR */
};
