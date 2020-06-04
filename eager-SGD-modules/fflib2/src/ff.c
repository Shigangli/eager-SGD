
#include "ff.h"
#include "ffop.h"
#include "ffnop.h"
#include "ffstorage.h"
#include "ffprogress.h"
#include "ffinternal.h"
#include "ffschedule.h"
#include "ffbuffer.h"
#include "ffop_default_progresser.h"
#include "ffop_scheduler.h"
#include "utils/ffqman.h"

#ifdef FFDEBUG
#include <sys/types.h>
#include <unistd.h>
int dbg_myrank;
#endif

ffdescr_t ff;


int ffinit(int * argc, char *** argv){

    int ret; 

    
    ffbind(&ff);
        
    /* ffnop is internal */
    ff.impl.ops[FFNOP].init = NULL;
    ff.impl.ops[FFNOP].exec = ffnop_execute; 
    ff.impl.ops[FFNOP].cancel = ffnop_cancel;    

    /* ffcallback is internal */
    ff.impl.ops[FFCALLBACK].init = NULL;
    ff.impl.ops[FFCALLBACK].exec = ffcallback_execute;
    
    /* tostring is internal */
    ff.impl.ops[FFSEND].tostring = ffsend_tostring;
    ff.impl.ops[FFRECV].tostring = ffrecv_tostring;
    ff.impl.ops[FFNOP].tostring  = ffnop_tostring;
    ff.impl.ops[FFCOMP].tostring  = ffcomp_tostring;
    ff.impl.ops[FFCALLBACK].tostring = ffcallback_tostring;

    /* finalize is internal */
    ff.impl.ops[FFSEND].finalize = ffsend_finalize;
    ff.impl.ops[FFRECV].finalize = ffrecv_finalize;
    ff.impl.ops[FFNOP].finalize = ffnop_finalize;
    ff.impl.ops[FFCOMP].finalize = ffcomp_finalize;
    ff.impl.ops[FFCALLBACK].finalize = ffcallback_finalize;

    ffstorage_init();
    ffbuffer_init();
    ffqman_init();
    ffop_init();
    ffschedule_init();
    ffop_scheduler_init();
    
    /* register default progresser (before progress thread!)*/
    ffprogresser_t default_progresser;
    default_progresser.init = ffop_default_progresser_init;
    default_progresser.finalize = ffop_default_progresser_finalize;
    default_progresser.progress = ffop_default_progresser_progress;
    ffprogresser_register(default_progresser);

    /* Initialize the backend */
    ff.impl.init(argc, argv);

    ff.terminate = 0;
#ifdef FFPROGRESS_THREAD
    ret = pthread_create(&(ff.progress_thread), NULL, progress_thread, &ff);
    if (ret){ return FFERROR; }
    while (!progresser_ready());
#else
    FFLOG("Progress thread disabled!\n");
#endif

#ifdef FFDEBUG
    ffrank(&dbg_myrank);
#endif

    FFLOG("Init: %i ==> PID %u\n", dbg_myrank, getpid());

    return FFSUCCESS;
}


int fffinalize(){

#ifdef FFPROGRESS_THREAD
    ff.terminate=1;
    if (pthread_join(ff.progress_thread, NULL)){
        return FFERROR;
    }
#endif

    ff.impl.finalize(); 
    ffop_finalize();
    ffbuffer_finalize();
    ffqman_finalize(); 
    ffstorage_finalize();
    ffop_scheduler_finalize(); 

    return FFSUCCESS;
}

int ffrank(int * rank){
    return ff.impl.get_rank(rank);
}

int ffsize(int * size){
    return ff.impl.get_size(size);
}

