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
#define LIMITER 19968 
//#define LIMITER 16
unsigned int seed1 = 6545343;
unsigned int seed2 = 32768;

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
        reduced_results[i] = (float *)calloc(count, sizeof(float));
    }

    ffinit(&argc, &argv);
    ffrank(&rank);
    ffsize(&size);

    double begin, elapse;
    ffschedule_h solo_allreduce_sched[N];

    for (int i = 0; i < N; i++)
    {
        ffrand_allreduce(to_reduce[i], reduced[i], count, tag++, FFSUM, FFFLOAT, 0, seed1, LIMITER, &(solo_allreduce_sched[i]));
    }

    if(rank==0)
       printf("balanced\n");

    MPI_Barrier(MPI_COMM_WORLD);
    for (int j = 0; j < N; j++)
    {
        ffschedule_start(solo_allreduce_sched[j]);
    }
    
    for (int i = 0; i < iters; i++)
    {
        //begin = MPI_Wtime();
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 1.0;
            }

            //memcpy to input buffer
            ffschedule_post(solo_allreduce_sched[j]);
            ffschedule_wait(solo_allreduce_sched[j]);

	    memcpy(reduced_results[j], reduced[j], sizeof(float)*count);
            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 0.0;
            }
            if(rank==0 || rank==2)
	    {	
                print_array("balanced", rank, i, reduced_results[j], count);
	    }
        }
        //MPI_Barrier(MPI_COMM_WORLD);
	//elapse = MPI_Wtime() - begin;
        //if(rank==0)
	//  printf("Rank = %d, iteration = %d, Total time = %f s\n", rank, i, elapse);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(rank==0)
       printf("randomly imbalanced\n");
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < iters; i++)
    {
        if((rank == rand_r(&seed2) % size) || (rank == (rand_r(&seed2) + 2)  % size) || (rank == (rand_r(&seed2) + 4) % size) || (rank == (rand_r(&seed2) + 6)  % size))
            usleep(200000);

        //synch to prevent send buffer data race
        MPI_Barrier(MPI_COMM_WORLD);
        //begin = MPI_Wtime();
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 1.0;
            }

            //memcpy to input buffer

            ffschedule_post(solo_allreduce_sched[j]);
            ffschedule_wait(solo_allreduce_sched[j]);
	    memcpy(reduced_results[j], reduced[j], sizeof(float)*count);

            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 0.0;
            }
            if(rank==0 || rank==2)
	    {	
                print_array("random imbalanced", rank, i, reduced_results[j], count);
	    }
        }
        //MPI_Barrier(MPI_COMM_WORLD);
	//elapse = MPI_Wtime() - begin;
        //if(rank==0)
	//  printf("Rank = %d, iteration = %d, Total time = %f s\n", rank, i, elapse);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(rank==0)
       printf("p1 and p0 constantly imbalanced\n");
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < iters; i++)
    {
        if((rank == 1) || (rank == 0))
            usleep(200000);

        //begin = MPI_Wtime();
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 1.0;
            }

            //memcpy to input buffer

            ffschedule_post(solo_allreduce_sched[j]);
            ffschedule_wait(solo_allreduce_sched[j]);
	    memcpy(reduced_results[j], reduced[j], sizeof(float)*count);

            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 0.0;
            }
            if(rank==0 || rank==2)
	    {	
                print_array("const imbalanced", rank, i, reduced_results[j], count);
	    }
        }
        //MPI_Barrier(MPI_COMM_WORLD);
	//elapse = MPI_Wtime() - begin;
        //if(rank==0)
	//  printf("Rank = %d, iteration = %d, Total time = %f s\n", rank, i, elapse);
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
