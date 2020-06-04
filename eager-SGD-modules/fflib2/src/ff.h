#ifndef _FF_H_
#define _FF_H_

#define FFCOMPLETED 1
#define FFSUCCESS 0
#define FFERROR -1
#define FFINVALID_ARG -2
#define FFTOO_MANY_DEPS -3
#define FFENOMEM -4
#define FFVERSION -5

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

#define FFOP_MPI

/* datatypes */
#define FFINT32             0
#define FFINT32_CTYPE       int32
#define FFINT64             1
#define FFINT64_CTYPE       int64
#define FFDOUBLE            2
#define FFDOUBLE_CTYPE      double
#define FFFLOAT             3
#define FFFLOAT_CTYPE       float
#define FFCHAR              4
#define FFCHAR_CTYPE        char
#define FFDATATYPE_NONE     5
#define FFDATATYPE_SENTINEL 6

/* operators */
#define FFSUM               0
#define FFPROD              1
#define FFMAX               2
#define FFMIN               3
#define FFIDENTITY          4
#define FFOPERATOR_SENTINEL 5
#define FFCUSTOM            6

/* options */
#define FFOP_DEP_AND            (1 << 1)
#define FFOP_DEP_OR             (1 << 2)
#define FFOP_NON_PERSISTENT     (1 << 3)
#define FFCOMP_DEST_ATOMIC      (1 << 4)
#define FFBUFFER_PTR            (1 << 5)
#define FFBUFFER_IDX            (1 << 6)
#define FFCOLL_BUFFERS          (1 << 7)
#define FFOP_DEP_FIRST          (1 << 8)
#define FFDEP_NO_AUTOPOST       (1 << 9)
#define FFDEP_IGNORE_VERSION    (1 << 10)
#define FFSHADOW_TAG            (1 << 11)
#define FFDEP_SKIP_OLD_VERSIONS (1 << 12)
#define FFOP_COMPLETE_BEFORE_CANCELLING (1 << 13)
#define FFDEP_CONSTRAINED       (1 << 14)

/* Our NULL */
#define FFNONE              -1
#define FFBUFF_NONE         0x0
#define FFINPLACE           ((void *) 0x1)

#define FFOP_STR_LEN        2048

#ifdef FFDEBUG
extern int dbg_myrank;
#define FFLOG(MSG, ...) printf("[%i - %u - %u][%s:%i] "MSG, dbg_myrank, getpid(), (unsigned int) pthread_self(), __FILE__, __LINE__,  ##__VA_ARGS__)

#define DOTLEN 256
#define FFGRAPH(A, B) \
{ \
char dot_a[DOTLEN]; \
char dot_b[DOTLEN]; \
ffop_tostring(A, dot_a, DOTLEN); \
ffop_tostring(B, dot_b, DOTLEN); \
FFLOG("DOT#\"%s.%i\" -> \"%s.%i\"\n", dot_a, dbg_myrank, dot_b, dbg_myrank); \
}
#else
#define FFLOG(MSG, ...) 
#define FFGRAPH(A, B) 
#endif

typedef int ffdatatype_h;
typedef int ffoperator_h;
typedef uint64_t ffop_h;
typedef uint64_t ffschedule_h;
typedef uint64_t ffbuffer_h;
typedef uint64_t ffdep_op_h;

typedef int (*ffschedule_start_fun_t)(ffschedule_h sched);
typedef int (*ffschedule_post_fun_t)(ffschedule_h sched);
typedef int (*ffschedule_delete_fun_t)(ffschedule_h sched);
typedef int (*ffschedule_wait_fun_t)(ffschedule_h sched);
typedef int (*ffschedule_test_fun_t)(ffschedule_h sched, int * flag);
typedef int (*ffschedule_print_fun_t)(ffschedule_h handle, FILE * fp, char * name);

typedef void (*ffcb_fun_t)(ffop_h op, void * arg);

int ffinit(int * argc, char *** argv);
int fffinalize();

int ffrank(int * rank);
int ffsize(int * size);

int ffop_post(ffop_h op);
int ffop_wait(ffop_h op);
int ffop_test(ffop_h op, int * flag);
int ffop_hb(ffop_h first, ffop_h second, int options);
int ffop_hb_fallback(ffop_h _first, ffop_h _second, ffop_h _fall_back, int options, ffdep_op_h *dep);
int ffdep_set_parent(ffdep_op_h _constrained_dep, ffdep_op_h _constraining_dep);
int ffop_free(ffop_h _op);
int ffop_tostring(ffop_h op, char * str, int len);
int ffop_cancel(ffop_h op);

int ffbuffer_create(void * addr, uint32_t count, ffdatatype_h datatype, int options, ffbuffer_h * _ffbuff);
int ffbuffer_delete(ffbuffer_h ffbuff);
int ffbuffer_resize(ffbuffer_h handle, void * addr, uint32_t new_count, ffdatatype_h new_datatype);
int ffbuffer_get_size(ffbuffer_h handle, uint32_t * count, ffdatatype_h * datatype);
int ffbuffer_get_data(ffbuffer_h handle, void ** mem);

int ffsend(void * addr, int count, ffdatatype_h datatype, int dest, int8_t tag, int options, ffop_h * op);
int ffsend_b(ffbuffer_h buffer, int dest, int8_t tag, int options, ffop_h *_op);

int ffrecv(void * addr, int count, ffdatatype_h datatype, int source, int8_t tag, int options, ffop_h * op);
int ffrecv_b(ffbuffer_h, int source, int8_t tag, int options, ffop_h * _op);

int ffnop(int options, ffop_h * handle);
int ffcallback(ffcb_fun_t cb, void * arg, int options, ffop_h * _op);

typedef int (*ffoperator_fun_t)(void*, void*, void*, uint32_t, ffdatatype_h);
int ffcomp(void * addr1, void * addr2, int count, ffdatatype_h datatype, ffoperator_h ffoperator, int options, void * addr3, ffop_h * op);
int ffcomp_b(ffbuffer_h buffer1, ffbuffer_h buffer2, ffoperator_h ffoperator, int options, ffbuffer_h buffer3, ffop_h * ophandle);
int ffcomp_operator_create(ffoperator_fun_t fun, int commutative, ffoperator_h * handle);
int ffcomp_operator_delete(ffoperator_h handle);

int ffschedule_create(ffschedule_h *sched);
int ffschedule_add_op(ffschedule_h sched, ffop_h op); 
int ffschedule_start(ffschedule_h sched);
int ffschedule_post(ffschedule_h sched);
int ffschedule_wait(ffschedule_h handle);
int ffschedule_test(ffschedule_h handle, int * flag);
int ffschedule_print(ffschedule_h handle, FILE * fp, char * name);
int ffschedule_delete(ffschedule_h sched);
int ffschedule_default_post(ffschedule_h sched);
int ffschedule_default_wait(ffschedule_h handle);
int ffschedule_default_test(ffschedule_h handle, int * flag);
int ffschedule_default_delete(ffschedule_h sched);
int ffschedule_default_print(ffschedule_h handle, FILE * fp, char * name);
int ffschedule_set_state(ffschedule_h handle, void * state);
int ffschedule_get_state(ffschedule_h handle, void ** state);
int ffschedule_get_begin_op(ffschedule_h schedh, ffop_h *oph);
int ffschedule_get_end_op(ffschedule_h schedh, ffop_h *oph);
int ffschedule_set_start_fun(ffschedule_h handle, ffschedule_start_fun_t fun);
int ffschedule_set_post_fun(ffschedule_h handle, ffschedule_post_fun_t fun);
int ffschedule_set_delete_fun(ffschedule_h handle, ffschedule_delete_fun_t fun);
int ffschedule_set_wait_fun(ffschedule_h handle, ffschedule_wait_fun_t fun);
int ffschedule_set_test_fun(ffschedule_h handle, ffschedule_test_fun_t fun);
int ffschedule_set_print_fun(ffschedule_h handle, ffschedule_print_fun_t fun);

int ffallreduce(void * sndbuff, void * rcvbuff, int count, int16_t tag, ffoperator_h ffoperator, ffdatatype_h datatype, int options, ffschedule_h * _sched);
int ffactivation(int options, int tag, ffop_h *user_activator, ffop_h *auto_activator, ffop_h * user_activator_test, ffop_h *activation_join, ffschedule_h *_sched);
int ffsolo_allreduce(void * sndbuff, void * rcvbuff, int count, int16_t tag, ffoperator_h ffoperator, ffdatatype_h datatype, int options, int async, ffschedule_h * _sched);
int ffsolo_limiter(int num_async, ffop_h * async_op, ffop_h * sync_op, ffschedule_h * _sched);
int ffrand_allreduce(void * sndbuff, void * rcvbuff, int count, int16_t tag, ffoperator_h ffoperator, ffdatatype_h datatype, int options, int seed, int async, ffschedule_h * _sched);


#endif /* _FF_H_ */
