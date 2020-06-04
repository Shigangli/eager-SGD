#ifndef _FFOP_SCHEDULER_
#define _FFOP_SCHEDULER_

#include "ffop.h"

int ffop_scheduler_init();
int ffop_scheduler_finalize();
int ffop_scheduler_schedule(ffop_t * op);
int ffop_scheduler_execute_all();

#endif /* _FFOP_SCHEDULER_ */

