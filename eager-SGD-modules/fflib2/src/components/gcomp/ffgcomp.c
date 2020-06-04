#include "ffgcomp.h"
#include "ffop_gcomp_operator.h"

int ffgcomp_init(){
    return ffop_gcomp_operator_init();
}

int ffgcomp_finalize(){
    return ffop_gcomp_operator_finalize();
}
