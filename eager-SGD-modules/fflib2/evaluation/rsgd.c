#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "ff.h"

#define FFCALL(X)                       \
    {                                   \
        int ret;                        \
        if (ret = (X) != FFSUCCESS)     \
        {                               \
            printf("Error: %i\n", ret); \
            exit(-1);                   \
        }                               \
    }
extern int num_threads;

void print_array(char *name, int pid, int iter, float *array, int len)
{
    printf("%s , rank=%d, iter=%d, array [ ", name, pid, iter);
    for (int i = 0; i < len; i++)
    {
        printf("%.2f ", array[i]);
    }
    printf("]\n");
}
#define N 1
#define LIMITER 1024 

int main(int argc, char *argv[])
{

    int rank, size, count, iters;
    float slowp;
    if (argc != 3)
    {
        printf("Usage: %s <count> <iters>\n", argv[0]);
        exit(1);
    }
    int16_t tag = 0;
    count = atoi(argv[1]);
    iters = atoi(argv[2]);
    float *to_reduce[N];
    float *reduced[N];
    float *reduced_results[N];
    for (int i = 0; i < N; i++)
    {
        to_reduce[i] = (float *)calloc(count, sizeof(float));
        reduced[i] = (float *)calloc(count, sizeof(float));
    }

    for (int k = 0; k < count; k++)
    {
        to_reduce[N-1][k] = 0.0;
        reduced[N-1][k] = 0.0;
    }

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&size);

    double begin, elapse, total_elapse;
    ffschedule_h solo_allreduce_sched[N];

    for (int i = 0; i < N; i++)
    {
        ffsolo_allreduce(to_reduce[i], reduced[i], count, tag++, FFSUM, FFFLOAT, 0, LIMITER, &(solo_allreduce_sched[i]));
    }
    
    srand(rank);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int j = 0; j < N; j++)
    {
        ffschedule_start(solo_allreduce_sched[j]);
    }
    for (int i = 0; i < iters; i++)
    {
        
        usleep(rank*1000);

        for (int j = 0; j < N; j++)
        {
            //memcpy to input buffer
            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 1.0;
            }

            begin = MPI_Wtime();

            ffschedule_post(solo_allreduce_sched[j]);
            ffschedule_wait(solo_allreduce_sched[j]);
	    
            elapse = MPI_Wtime() - begin;
            
            //randomly select two processes to check the results
            if(rank==0 || rank==2)
	    {	
                printf("iter=%d, count=%d, result=%.2f, rank=%d \n", i, count, reduced[0][0], rank);
	    }

            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 0.0;
            }
        }
        //printf("iter=%d, count=%d, runtime=%f, rank=%d \n", i, count, elapse, rank);
        MPI_Reduce(&elapse, &total_elapse, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        if(rank == 0)
            printf("iter=%d, count=%d, average runtime=%f, rank=%d \n", i, count, total_elapse/size, rank);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    FFLOG("Completed\n");

    fffinalize();

    for (int i = 0; i < N; i++)
    {
        free(reduced[i]);
        free(to_reduce[i]);
    }
    return 0;
}
