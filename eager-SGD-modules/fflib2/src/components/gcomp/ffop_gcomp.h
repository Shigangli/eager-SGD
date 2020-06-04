#ifndef _FFOP_GCOMP_H_
#define _FFOP_GCOMP_H_

int ffop_gcomp_init(ffop_t * op);
int ffop_gcomp_execute(ffop_t * op, ffbuffer_set_t * mem);
int ffop_gcomp_wait(ffop_t * op);
int ffop_gcomp_test(ffop_t * op, int * flag);
int ffop_gcomp_cancel(ffop_t * op);

#endif /* _FFOP_GCOMP_H_ */
