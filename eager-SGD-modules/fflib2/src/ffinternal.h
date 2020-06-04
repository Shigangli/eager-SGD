#ifndef _FFINTERNAL_
#define _FFINTERNAL_

#define FFPROGRESS_THREAD
//#define WAIT_COND

#include "ff.h"
#include "ffdatatype.h"
#include "fflocks.h"

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#include <unistd.h>
#define FFCALL(X) { int r; if (r=(X)!=FFSUCCESS) return r; }
#define FFCALLV(X, V) { int r; if (r=(X)!=FFSUCCESS) return V; }

//#define FFLOG_ERROR(MSG, ...) { printf("FFlib error: %s"MSG, ## __VA_ARGS__); }

#define FFLOG_ERROR(MSG, ...) { printf("[%u][%s:%i] "MSG, getpid(), __FILE__, __LINE__,  ##__VA_ARGS__); }

#define IS_OPT_SET(op, opt) (((op->options) & opt) == opt)

#define CAS(PTR, OLDVAL, NEWVAL) __sync_bool_compare_and_swap(PTR, OLDVAL, NEWVAL)
#define MIN(a, b, c) (((a<b ? a : b) < c) ? (a<b ? a : b) : c)

#define FFSEND      0
#define FFRECV      1
#define FFCOMP      2
#define FFNOP       3
#define FFCALLBACK  4
#define FFMAX_IDX   5 /* not actual op, just boundary for ops indexes */

#define TAG_VERSION_MASK    0x00000FFF
#define TAG_TYPE_MASK       0x00000001

typedef int ffdatatype_t; /* for now the internal datatype type is an int as well */
typedef uint32_t ffpeer_t;
typedef struct ffop ffop_t;
typedef struct ffop_descriptor ffop_descriptor_t;
typedef struct ffbuffer_set ffbuffer_set_t;

typedef int (*ffimpl_init_t)(int*, char***);
typedef int (*ffimpl_finalize_t)();
typedef int (*ffimpl_get_rank_t)(int*);
typedef int (*ffimpl_get_size_t)(int*);
typedef int (*ffimpl_operator_create_t)(ffoperator_fun_t, int, ffoperator_h*);
typedef int (*ffimpl_operator_delete_t)(ffoperator_h);


/* Operation descriptor */
typedef int (*ffop_exec_t)(ffop_t*, ffbuffer_set_t*);
typedef int (*ffop_init_t)(ffop_t*);
typedef int (*ffop_test_t)(ffop_t*, int*);
typedef int (*ffop_wait_t)(ffop_t*);
typedef int (*ffop_tostring_t)(ffop_t*, char *, int);
typedef int (*ffop_finalize_t)(ffop_t*);
typedef int (*ffop_cancel_t)(ffop_t*);

typedef struct ffop_descriptor{
    ffop_init_t init;
    ffop_exec_t exec;
    ffop_tostring_t tostring;
    ffop_finalize_t finalize;
    ffop_wait_t wait;
    ffop_test_t test;
    ffop_cancel_t cancel;
} ffop_descriptor_t;


/* FF descriptor */
typedef struct ff_descr{
    volatile int terminate;
    volatile int progress_thread_ready;
    pthread_t progress_thread;

    struct impl{
        ffimpl_init_t init;
        ffimpl_finalize_t finalize;
        ffimpl_get_rank_t get_rank;
        ffimpl_get_size_t get_size;
        ffimpl_operator_create_t operator_create;
        ffimpl_operator_delete_t operator_delete;
        ffop_descriptor_t ops[FFMAX_IDX];
    } impl;
} ffdescr_t;


extern ffdescr_t ff;

#endif /* _FFINTERNAL_ */
