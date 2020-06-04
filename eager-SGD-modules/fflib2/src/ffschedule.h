#ifndef _FFSCHEDULE_H_
#define _FFSCHEDULE_H_

#include "ff.h"
#include "ffinternal.h"
#include "ffop.h"

typedef struct ffschedule{
    ffop_t * oplist;
    ffop_t * begin_op;
    ffop_t * end_op;
    uint64_t id;
    void * state;

    ffschedule_post_fun_t start_fun;
    ffschedule_post_fun_t post_fun;
    ffschedule_delete_fun_t delete_fun;
    ffschedule_wait_fun_t wait_fun;
    ffschedule_test_fun_t test_fun;
    ffschedule_print_fun_t print_fun;
} ffschedule_t;


int ffschedule_init();


#endif /* _FFSCHEDULE_H_ */
