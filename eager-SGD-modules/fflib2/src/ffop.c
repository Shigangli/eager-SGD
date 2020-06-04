#include "ffop.h"
#include "ffsend.h"
#include "ffrecv.h"
#include "ffstorage.h"
#include "ffop_default_progresser.h"
#include "ffop_scheduler.h"
#include <sched.h>
#include <assert.h>

#define INITIAL_FFOP_POOL_COUNT 8192
#define INITIAL_FFDEP_OP_POOL_COUNT 8192

static pool_h op_pool;
static pool_h dep_op_pool;

static uint64_t opid = 0;

int ffop_init(){

    op_pool = ffstorage_pool_create(sizeof(ffop_t), INITIAL_FFOP_POOL_COUNT, NULL);
    dep_op_pool = ffstorage_pool_create(sizeof(ffdep_op_t), INITIAL_FFDEP_OP_POOL_COUNT, NULL);

    return FFSUCCESS;
}

int ffop_finalize(){   
    ffstorage_pool_destroy(op_pool);
    ffstorage_pool_destroy(dep_op_pool);
    return FFSUCCESS;
}

int ffop_free(ffop_h _op){
    ffop_t * op = (ffop_t *) _op;
    //FIXME one should create/destroy the mutex/cond only when
    //the op is created for the first time and freed for the last time
    //(i.e., by the pool).
#ifdef WAIT_COND
    FFMUTEX_DESTROY(op->mutex);
    FFCOND_DESTROY(op->cond);
#endif
        
    ffdep_op_t *dep = op->dep_first;
    while (dep!=NULL){
        ffdep_op_t *dep_next = dep->next;
        ffstorage_pool_put(dep);
        if (dep==op->dep_last) dep = NULL;
        else dep = dep_next;
    }

    return ffstorage_pool_put(op);
}

int ffop_tostring(ffop_h _op, char * str, int len){
    ffop_t * op = (ffop_t *) _op;
    ff.impl.ops[op->type].tostring(op, str, len);
    return FFSUCCESS;
}

int ffop_post(ffop_h _op){
    ffop_t * op = (ffop_t *) _op;

    FFLOG("Posting op %lu\n", op->id);

    if (op->version>0 && IS_OPT_SET(op, FFOP_NON_PERSISTENT)){
        FFLOG("Re-posting a non-persistent operation is not allowed!\n");
        return FFINVALID_ARG;
    }

    op->posted_version++;

    return ffop_scheduler_schedule(op);
}

int ffop_execute(ffop_t * op){
    int res;

    op->version++;
    op->instance.dep_left = op->in_dep_count; 

    uint32_t op_version = op->version;

    assert(op->last_executed_version < op_version);

#ifdef FFDEBUG
    if (op->instance.dep_left>0) FFLOG("Posting an op with dependencies left!\n");
#endif

#ifdef ARGS_CHECK
    if (op->type<0 || op->type>FFMAX_IDX) {
        FFLOG("Invalid arg: op->type: %u\n", op->type);
        return FFINVALID_ARG;
    }
#endif
    uint8_t already_progressing = 0;// op->in_flight;
    
    if (op->in_flight && 0){
        if (IS_OPT_SET(op, FFOP_COMPLETE_BEFORE_CANCELLING)) {
            // post(op), post(op), exec(op); here we handle the case
            // in which the same op has been posted multiple time before 
            // executing it.
            FFLOG("Completing op %u before cancelling it (last_executed_version: %u; version: %u)!\n", op->id, op->last_executed_version, op_version);
            for (int i=op->last_completed_version; i < op_version - 1; i++) { ffop_complete(op); }
        } else {
            //fast forward
            op->last_completed_version = op_version - 1;
        }

        //ffop_cancel((ffop_h) op);         
        already_progressing = 1;
    }
    
    op->last_executed_version++;
    op->in_flight = 1;
    op->instance.completed = 0;

    FFLOG("Executing op %lu\n", op->id);
    //__sync_fetch_and_add(&(op->instance.posted_version), 1);
    res = ff.impl.ops[op->type].exec(op, NULL);

    /* check if the operation has been immediately completed */
    if (res==FFCOMPLETED && !already_progressing){ ffop_default_progresser_track(op); }
    
    /* check for futures */
    ffdep_op_t *cur_master = op->master_first;
    uint32_t futures_taken = 0;
    while (cur_master!=NULL){
        if (cur_master->futures>0){
            FFLOG("Found future for op %u from op %u (dep_left: %u)\n", op->id, cur_master->master->id, op->instance.dep_left);
            cur_master->futures--;
            op->instance.dep_left--;
            futures_taken++;
        }
        cur_master = cur_master->next_master;
    }

    if (op->in_dep_count > 0 && (op->instance.dep_left==0 || (futures_taken>0 && IS_OPT_SET(op, FFOP_DEP_OR)))) {
        ffop_post(op);
    }

    return res; 
}

int ffop_wait(ffop_h _op){
    ffop_t * op = (ffop_t *) _op;

    FFLOG("Waiting op %lu (version: %u; wait_version: %u; waiting for: %u; completed: %u)\n", op->id, op->version, op->wait_version, op->wait_version+1, (unsigned int) op->instance.completed);

#ifdef FFPROGRESS_THREAD
    uint32_t polls=0;

#ifdef WAIT_COND
    while (!FFOP_TEST_VERSION_COMPLETION(op, op->wait_version+1)){
        FFCOND_WAIT(op->cond, op->mutex);
    }
#else
    while (!FFOP_TEST_VERSION_COMPLETION(op, op->wait_version+1)){
        if (polls >= FFPOLLS_BEFORE_YIELD){
            polls=0;
            sched_yield();
        }else {
            polls++;
        }
    }
#endif

    FFLOG("Waking op from waiting for %lu (current version: %u; wait_version: %u)\n", op->id, op->version, op->wait_version);

    if (op->wait_version+1 >= op->version) op->instance.completed = 0;
    op->wait_version++;

    //FFLOG("Wait on %p finished: version: %u; posted version: %u; completed version: %u\n", op, op->version, op->instance.posted_version, op->instance.completed_version);
    return FFSUCCESS;
#else
    ff.impl.ops[op->type].wait(op); //FIXME: check this return value
    return ffop_complete(op);
#endif
}

int ffop_test(ffop_h _op, int * flag){
    ffop_t * op = (ffop_t *) _op;
    
#ifdef FFPROGRESS_THREAD
    *flag = FFOP_TEST_VERSION_COMPLETION(op, op->wait_version+1);

    if (*flag){
        op->instance.completed=0;
        op->wait_version++;
    }
    return FFSUCCESS;
#else
    FFLOG_ERROR("This path is not tested!!!\n");
    ff.impl.ops[op->type].test(op, flag); //FIXME: check this return value
    if (*flag){
        op->instance.completed=0;
        op->wait_version++;
        return ffop_complete(op);

    }
    return FFSUCCESS;
#endif
}

int ffop_hb(ffop_h _first, ffop_h _second, int options){
    ffdep_op_h dep;
    return ffop_hb_fallback(_first, _second, FFNONE, options, &dep);
}

int ffdep_set_parent(ffdep_op_h _constrained_dep, ffdep_op_h _constraining_dep){

    ffdep_op_t *constrained_dep = _constrained_dep;
    ffdep_op_t *constraining_dep = _constraining_dep;
    assert(constraining_dep->children==NULL);
    constraining_dep->children = constrained_dep;
    constrained_dep->options |= FFDEP_CONSTRAINED;

}

int ffop_hb_fallback(ffop_h _first, ffop_h _second, ffop_h _fall_back, int options, ffdep_op_h *_dep){
    ffop_t * first = (ffop_t *) _first;
    ffop_t * second = (ffop_t *) _second;
    ffop_t * fall_back = (_fall_back==FFNONE) ? NULL : (ffop_t *) _fall_back;
#ifdef ARGS_CHECK
    if (first==NULL || second==NULL) return FFINVALID_ARG;
#endif

    FFLOG("HB: %lu -> %lu\n", first->id, second->id);

    FFGRAPH(_first, _second);
    
    ffdep_op_t *dep;        
    ffstorage_pool_get(dep_op_pool, (void **) &dep);
    *((ffdep_op_t **) _dep) = dep;
    dep->op = second;
    dep->options = options;
    dep->fall_back = fall_back;
    dep->count = 0;
    dep->futures = 0;
    dep->children = NULL;
    dep->master = first;

    if (first->dep_first==NULL){
        first->dep_first        = dep;
        first->dep_last         = dep;
        first->dep_next         = dep;
        dep->next               = dep;
    }else{
        dep->next               = first->dep_first;
        first->dep_last->next   = dep;
        first->dep_last         = dep;
    }

    dep->next_master = second->master_first;
    second->master_first = dep;
    
    __sync_fetch_and_add(&(second->in_dep_count), 1);
    second->instance.dep_left = second->in_dep_count;

    return FFSUCCESS;
}


/* internal (called by ffsend, ffrecv, others) */
int ffop_create(ffop_t ** ptr){
    ffstorage_pool_get(op_pool, (void **) ptr);
    
    ffop_t * op                     = *ptr;

    op->id                          = opid++;
    op->in_dep_count                = 0;
    op->sched_next                  = NULL;
    op->options                     = FFOP_DEP_AND;
    op->version                     = 0;
    op->wait_version                = 0;
    op->posted_version              = 0;

    op->dep_next                    = NULL;
    op->dep_first                   = NULL;
    op->dep_last                    = NULL;
    op->in_flight                   = 0;

    op->master_first                = NULL;

    op->instance.next               = NULL;
    op->instance.dep_left           = 0;
    op->last_executed_version       = 0;
    op->last_completed_version      = 0;
    //op->instance.posted_version     = 0;
    //op->instance.completed_version  = 0;
    op->instance.completed          = 0;

#ifdef WAIT_COND
    FFMUTEX_INIT(op->mutex);
    FFCOND_INIT(op->cond);
#endif

    return FFSUCCESS;
}

int ffop_complete(ffop_t * op){

#ifdef FFDEBUG
    if (op->version <= op->last_completed_version){
        FFLOG("ERROR: op %u; version: %u greater than last_completed_version: %u\n", op->id, op->version, op->last_completed_version);
        assert(0);
    }
#endif

    assert(op->version > op->last_completed_version);
    uint32_t op_version = ++op->last_completed_version;

    FFLOG("completing op %lu (version: %u)\n", op->id, op_version);

    op->in_flight = 0;
    // restore dep_left, so the op can be reused
    //op->instance.dep_left = op->in_dep_count; 
    __sync_add_and_fetch(&(op->instance.completed), 1);

#ifdef WAIT_COND
    FFCOND_SIGNAL(op->cond, op->mutex);
#endif

    if (op->dep_next==NULL) return FFSUCCESS;
    ffdep_op_t * first_dep = op->dep_next;
    
    uint8_t satisfy_all = !IS_OPT_SET(op, FFOP_DEP_FIRST);

    do{
        ffdep_op_t * dep = op->dep_next;
        ffop_t * dep_op = dep->op;
        op->dep_next = op->dep_next->next;
        uint32_t dep_op_version = dep_op->posted_version;

        if (op_version <= dep_op_version && !IS_OPT_SET(dep, FFDEP_IGNORE_VERSION)) {
            FFLOG("ffop version mismatch -> dependency not satisfied (%lu.version (dep_op) = %u; %lu.version (op) = %u); \n", dep_op->id, dep_op_version, op->id, op_version);
            if (dep->fall_back!=NULL) {
                dep_op = dep->fall_back;
                dep_op_version = dep_op->posted_version;  
                FFLOG("ffop version mismatch FOUND FALLBACK! (%lu.version (dep_op) = %u; %lu.version (op) = %u);\n", dep_op->id, dep_op_version, op->id, op_version);

                if (op_version <= dep_op_version && !IS_OPT_SET(dep, FFDEP_IGNORE_VERSION)) {
                    FFLOG("ffop version mismatch on FALLBACK DEP OP! -> dependency not satisfied (%lu.version (dep_op) = %u; %lu.version (op) = %u);\n", dep_op->id, dep_op_version, op->id, op_version);
                    continue;
                }
            } else {
                continue;
            }
        }

        if (op_version > dep_op_version + 1 && !IS_OPT_SET(dep, FFDEP_IGNORE_VERSION)){
            /*
            if (IS_OPT_SET(dep, FFDEP_SKIP_OLD_VERSIONS)) {
                FFLOG("OLD OP VERSION: skipping (%lu.version (dep_op) = %u; %lu.version (op) = %u); (cur deps left: %u/%u)\n", dep_op->id, dep_op_version, op->id, op_version, dep_op->instance.dep_left, dep_op->in_dep_count);
                continue;
            }
            //we are updating an operation that is old: it may have partially satisfied dependencies
            FFLOG("OLD OP VERSION: resetting deps to %u (before was %u)\n", dep_op->in_dep_count, dep_op->instance.dep_left);
            dep_op->instance.dep_left = dep_op->in_dep_count; 
            */

           dep->futures++;
           FFLOG("OP %u is too old (op: %u; op_version: %u; dep_op: %u; dep_op_version: %u), increasing futures of this dep (futures: %u)\n", dep_op->id, op->id, op_version, dep_op->id, dep_op_version, dep->futures);
           continue;
        }

        if (IS_OPT_SET(dep, FFDEP_NO_AUTOPOST)){
            FFLOG("op %lu is set with FFOP_NO_AUTOPOST --> skipping it!\n", dep_op->id);
            continue;
        }

        if (IS_OPT_SET(dep, FFDEP_CONSTRAINED)){
            if (dep->count==0) continue;
            else dep->count--;
        }

        int32_t deps = __sync_add_and_fetch(&(dep_op->instance.dep_left), -1);
        FFLOG("Decreasing %lu dependencies by one: now %i (is OR dep: %u; non-persistent: %u; ignore-version: %u); %lu.version (dep_op) = %u; %lu.version (op) = %u\n", dep_op->id, dep_op->instance.dep_left, (unsigned int) IS_OPT_SET(dep_op, FFOP_DEP_OR), (unsigned int) IS_OPT_SET(dep_op, FFOP_NON_PERSISTENT), (unsigned int) IS_OPT_SET(dep, FFDEP_IGNORE_VERSION), dep_op->id, dep_op_version, op->id, op_version);

        if (dep->children!=NULL){
            dep->children->count++;
        }

        int trigger;
        // triggering conditions:
        // no dependencies left if is an AND dependency *or* at least one is satisfied if it is an OR dep
        trigger  = deps <= 0 || IS_OPT_SET(dep_op, FFOP_DEP_OR); 
        // the op has not been already posted *or* the op is persistent
        trigger &= (dep_op_version==0 || !IS_OPT_SET(dep_op, FFOP_NON_PERSISTENT));
    
        if (trigger){
            FFLOG("All dependencies of %lu are satisfied: posting it!\n", dep_op->id);
            ffop_post((ffop_h) dep_op);
        }else{
            FFLOG("Op %lu is not going to be posted: deps: %i; FFOP_DEP_OR: %u; FFOP_NON_PERSISTENT: %u; dep_op_version: %u\n", dep_op->id, deps, IS_OPT_SET(dep_op, FFOP_DEP_OR), IS_OPT_SET(dep_op, FFOP_NON_PERSISTENT), dep_op_version);    
        }
        
    } while (op->dep_next != first_dep && satisfy_all);

    FFLOG("statify dependency op loop completed\n");

    return FFSUCCESS;
} 

int ffop_cancel(ffop_h _op){
    ffop_t * op = (ffop_t *) _op;
    FFLOG("Cancelling op %lu (version: %u)\n", op->id, op->version);
    op->instance.dep_left = op->in_dep_count; 
    return ff.impl.ops[op->type].cancel(op); 
}
