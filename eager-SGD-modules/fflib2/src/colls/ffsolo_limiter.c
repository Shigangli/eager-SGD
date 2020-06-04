#include "ff.h"
#include "ffcollectives.h"

int ffsolo_limiter(int num_async, ffop_h * async_op, ffop_h * sync_op, ffschedule_h * _sched){

    ffschedule_h sched;
    FFCALL(ffschedule_create(&sched));

    ffop_h start_op;
    ffnop(FFOP_DEP_FIRST, &start_op);

    ffnop(FFOP_DEP_OR, async_op);
    ffnop(0, sync_op);

    for (int i=0; i<num_async; i++){
        ffop_h async_activator;
        ffnop(0, &async_activator);
        ffop_hb(start_op, async_activator, FFDEP_IGNORE_VERSION);
        ffop_hb(async_activator, *async_op, FFDEP_IGNORE_VERSION);
        ffschedule_add_op(sched, async_activator);
    }

    ffop_h sync_activator;
    ffnop(0, &sync_activator);
    ffop_hb(sync_activator, *sync_op, FFDEP_IGNORE_VERSION);
    ffop_hb(start_op, sync_activator, FFDEP_IGNORE_VERSION);

    ffschedule_add_op(sched, sync_activator);
    ffschedule_add_op(sched, start_op);
    ffschedule_add_op(sched, *async_op);
    ffschedule_add_op(sched, *sync_op);

    *_sched = sched;

    return FFSUCCESS;
}