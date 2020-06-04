
#ifndef _FFCOLLECTIVES_
#define _FFCOLLECTIVES_


#include "ff.h"
#include "ffop.h"
#include "ffinternal.h"

#include <math.h>

#define RANK2VRANK(rank, vrank, root) \
{ \
    vrank = rank; \
    if (rank == 0) vrank = root; \
    if (rank == root) vrank = 0; \
}
                                                                               
#define VRANK2RANK(rank, vrank, root) \
{ \
    rank = vrank; \
    if (vrank == 0) rank = root; \
    if (vrank == root) rank = 0; \
}

#endif /* _FFCOLLECTIVES_ */
