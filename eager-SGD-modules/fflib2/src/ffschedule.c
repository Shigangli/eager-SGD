#include "ffschedule.h"
#include "ffstorage.h"
#include "ffop.h"

#define INITIAL_FFSCHEDULE_POOL_COUNT 1024

static pool_h schedule_pool;

static uint64_t schedid = 0;

int ffschedule_init(){
    schedule_pool = ffstorage_pool_create(sizeof(ffschedule_t), INITIAL_FFSCHEDULE_POOL_COUNT, NULL);
    return FFSUCCESS;
}


int ffschedule_create(ffschedule_h * handle){
    ffschedule_t ** sched       = (ffschedule_t **) handle; 
    ffstorage_pool_get(schedule_pool, (void **) sched);

    (*sched)->oplist            = NULL;
    (*sched)->id                = schedid++;   
    (*sched)->state             = NULL;

    //start and post are the same unless otherwise defined (this distinction makes sense only for solo)
    (*sched)->start_fun         = ffschedule_default_post;
    (*sched)->post_fun          = ffschedule_default_post;
    (*sched)->delete_fun        = ffschedule_default_delete;
    (*sched)->wait_fun          = ffschedule_default_wait;
    (*sched)->test_fun          = ffschedule_default_test;
    (*sched)->print_fun         = ffschedule_default_print;
    
    ffnop(0, (ffop_h *) &((*sched)->begin_op));
    ffnop(0, (ffop_h *) &((*sched)->end_op));

    return FFSUCCESS;
}


/* schedule actions */

int ffschedule_delete(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    return sched->delete_fun(handle);
}   

int ffschedule_start(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    return sched->start_fun(handle);
}

int ffschedule_post(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    return sched->post_fun(handle);
}

int ffschedule_wait(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    return sched->wait_fun(handle);
}

int ffschedule_test(ffschedule_h handle, int * flag){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    return sched->test_fun(handle, flag);
}

int ffschedule_print(ffschedule_h handle, FILE * fp, char * name){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    return sched->print_fun(handle, fp, name);
}

/* schedule default actions */

int ffschedule_default_delete(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    while (sched->oplist!=NULL){
        ffop_t * op = sched->oplist;
        sched->oplist = op->sched_next;
        ffop_free((ffop_h) op);
    }
    return ffstorage_pool_put(sched);
}

int ffschedule_default_post(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    FFLOG("Posting schedule %lu\n", sched->id);
    return ffop_post((ffop_h) sched->begin_op);
}

int ffschedule_default_wait(ffschedule_h handle){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    FFLOG("Waiting on schedule %lu (end_op: %lu)\n", sched->id, sched->end_op->id);
#ifndef FFPROGRESS_THREAD
    FFLOG_ERROR("Not implemented (FFPROGRESS_THREAD not defined)!\n");
    exit(1);
#endif

    FFLOG("sched->begin_op->version: %u; sched->end_op->version: %u; sched->end_op->completed: %u\n", sched->begin_op->version, sched->end_op->version, (uint32_t) sched->end_op->instance.completed);
    return ffop_wait((ffop_h) sched->end_op);
}

int ffschedule_default_test(ffschedule_h handle, int * flag){
    ffschedule_t * sched = (ffschedule_t *) handle; 
#ifndef FFPROGRESS_THREAD
    FFLOG_ERROR("Not implemented (FFPROGRESS_THREAD not defined)!\n");
    exit(1);
#endif
    return ffop_test((ffop_h) sched->end_op, flag);
}

void print_op(FILE * fp, ffop_t * op, char * name){
    

    if (op->in_dep_count==0 && op->dep_first==NULL) return;
#ifdef VIS
    char buffer[FFOP_STR_LEN];
    ffop_tostring((ffop_h) op, buffer, FFOP_STR_LEN);    
    fprintf(fp, "##VISNODE {id: %lu, label: '%s'}\n", op->id, buffer);

    ffdep_op_t * depop = op->dep_first;
    if (depop==NULL) return;
    do{
        fprintf(fp, "##VISEDGE {from: %lu, to: %lu, arrows:'to'}\n", op->id, depop->op->id);
        depop = depop->next;
    } while(depop!=op->dep_first);
#else
    char buffer_op[FFOP_STR_LEN];
    char buffer_depop[FFOP_STR_LEN];
    ffop_tostring((ffop_h) op, buffer_op, FFOP_STR_LEN);

    ffdep_op_t * depop = op->dep_first;
    if (depop==NULL) return;
    do{
        ffop_tostring((ffop_h) depop->op, buffer_depop, FFOP_STR_LEN);
        fprintf(fp, "\"%lu.%s\"->\"%lu.%s\";", op->id, buffer_op, depop->op->id, buffer_depop);
        depop = depop->next;
    } while(depop!=op->dep_first);

#endif
}

int ffschedule_default_print(ffschedule_h handle, FILE * fp, char * name){
    ffschedule_t * sched = (ffschedule_t *) handle;
    ffop_t * op = sched->oplist;
    
#ifndef VIS
    fprintf(fp, "subgraph cluster_%s{ label=\"%s\";", name, name);
#endif

    print_op(fp, sched->begin_op, name);
    print_op(fp, sched->end_op, name);

    while (op!=NULL){
        print_op(fp, op, name);
        op = op->sched_next;    
    }
#ifndef VIS
    fprintf(fp, "}");
#endif
    return FFSUCCESS;
}


/* function setters */

int ffschedule_set_post_fun(ffschedule_h handle, ffschedule_post_fun_t fun){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->post_fun = fun;
    return FFSUCCESS;
}

int ffschedule_set_start_fun(ffschedule_h handle, ffschedule_start_fun_t fun){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->start_fun = fun;
    return FFSUCCESS;
}

int ffschedule_set_delete_fun(ffschedule_h handle, ffschedule_delete_fun_t fun){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->delete_fun = fun;
    return FFSUCCESS;
}

int ffschedule_set_wait_fun(ffschedule_h handle, ffschedule_wait_fun_t fun){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->wait_fun = fun;
    return FFSUCCESS;
}

int ffschedule_set_test_fun(ffschedule_h handle, ffschedule_test_fun_t fun){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->test_fun = fun;
    return FFSUCCESS;
}

int ffschedule_set_print_fun(ffschedule_h handle, ffschedule_print_fun_t fun){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->print_fun = fun;
    return FFSUCCESS;
}

/* others FIXME: adjust nicely */

int ffschedule_set_state(ffschedule_h handle, void * state){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    sched->state = state;
    return FFSUCCESS;
}

int ffschedule_get_state(ffschedule_h handle, void ** state){
    ffschedule_t * sched = (ffschedule_t *) handle; 
    *state = sched->state;
    return FFSUCCESS;
}


int ffschedule_add_op(ffschedule_h schedh, ffop_h oph){
    ffschedule_t * sched = (ffschedule_t *) schedh;
    ffop_t * op = (ffop_t *) oph;
    
    op->sched_next = sched->oplist;
    sched->oplist = op;

    FFLOG("adding op %lu to schedule %lu\n", op->id, sched->id);
    
    if (op->in_dep_count==0){
        FFLOG("making op %lu dependent from begin; schedule %lu\n", op->id, sched->id);
        ffop_hb((ffop_h) sched->begin_op, (ffop_h) op, 0);
    }

    if (op->dep_next==NULL){
        FFLOG("making end_op dependent from op %lu; schedule: %lu\n", op->id, sched->id);
        ffop_hb((ffop_h) op, (ffop_h) sched->end_op, 0);
    }

    return FFSUCCESS;
}

int ffschedule_get_begin_op(ffschedule_h schedh, ffop_h *oph){
    ffschedule_t * sched = (ffschedule_t *) schedh;
    *oph = (ffop_h) sched->begin_op;   
    return FFSUCCESS;
}

int ffschedule_get_end_op(ffschedule_h schedh, ffop_h *oph){
    ffschedule_t * sched = (ffschedule_t *) schedh;
    *oph = (ffop_h) sched->end_op;   
    return FFSUCCESS;
}

