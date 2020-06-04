#ifndef _FFPROGRESS_H_
#define _FFPROGRESS_H_

#define MAX_PROGRESSERS 16

#include "ffop.h"
#include "ffinternal.h"
#include "utils/ffqman.h"

typedef int (*ffprogresser_init_t)();
typedef int (*ffprogresser_finalize_t)();
typedef int (*ffprogresser_progress_t)(ffqman_t *);


typedef struct ffprogresser {
    ffprogresser_init_t init;
    ffprogresser_finalize_t finalize;
    ffprogresser_progress_t progress;
} ffprogresser_t;

int progresser_ready();
void * progress_thread(void * args);

int ffprogresser_register(ffprogresser_t progresser);

#endif /* _FFPROGRESS_H_ */
