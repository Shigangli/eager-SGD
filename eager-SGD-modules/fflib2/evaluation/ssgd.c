#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdint.h>

#define FFCALL(X)                       \
    {                                   \
        int ret;                        \
        if (ret = (X) != FFSUCCESS)     \
        {                               \
            printf("Error: %i\n", ret); \
            exit(-1);                   \
        }                               \
    }

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

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);	
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    double begin, elapse, total_elapse;
    
    srand(rank);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < iters; i++)
    {

        usleep(rank*1000);
   
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < count; k++)
            {
                to_reduce[j][k] = 1.0;
            }

            begin = MPI_Wtime();
  
            MPI_Allreduce(to_reduce[j], reduced[j], count, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
	    
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



    for (int i = 0; i < N; i++)
    {
        free(reduced[i]);
        free(to_reduce[i]);
    }
    MPI_Finalize();
    return 0;
}
