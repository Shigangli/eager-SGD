import deep500 as d5
from deep500.frameworks import tensorflow as d5tf
import os
import tensorflow as tf

class EagerSGDOptimizer(object):
    def __init__(self, optimizer: tf.train.Optimizer, comm_size: int):
        self.comm_size = comm_size
        self.optimizer = optimizer

        # Compile the operator
        opdesc = d5.compile_custom_cppop_inline('allreducef', _sallreduce,
                                                # Input tensor shapes (gradient, UNUSED last gradient op)
                                                [d5.tensordesc.runtime_shape(tf.float32), d5.tensordesc.runtime_shape(tf.float32)],
                                                # Output tensor shapes (reduced gradient)
                                                [d5.tensordesc.runtime_shape(tf.float32)],
                                                live_output=True, output_folder='/tmp')
        self.compiled_op = d5tf.custom_op(opdesc, compile_only=True)
        self._handles = []

    def compute_gradients(self, *args, **kwargs):
        return self.optimizer.compute_gradients(*args, **kwargs)

    def apply_gradients(self, grads_and_vars, global_step):
        optimizer =  self.optimizer
        new_gvs = []
        last_g = None
        for grad, var in reversed(grads_and_vars):
            if grad is None:
                new_gvs.append((grad, var))
            else:
                if last_g is None:
                    last_g = grad
                    
            self.compiled_op.op.inputs = [d5tf.desc_from_tensor(grad), d5tf.desc_from_tensor(last_g)]
            self.compiled_op.op.outputs = [d5tf.desc_from_tensor(grad)]
            op, lib, handle = d5tf.custom_op(self.compiled_op, return_handle=True)
            self._handles.append((lib, handle))
            # Apply on gradient
            allreduced = op(grad/(self.comm_size), last_g)
            new_gvs.append((allreduced, var))
            last_g = allreduced

        return self.optimizer.apply_gradients(new_gvs, global_step)

_sallreduce = """
#include <deep500/deep500.h>
#include <iostream>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cfloat>


#include <mpi.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

extern "C"
{
#include "ff.h"
}


int is_init = 0;
int counter = 0;
int16_t solo_coll_tag = 0;

#define LIMITER 32

//number of allreduces for one step of ResNet-50
#define OPS 161
int length[OPS] = {
1001,
2050048,
2048,
2048,
1048576,
512,
512,
2359296,
512,
512,
1048576,
2048,
2048,
1048576,
512,
512,
2359296,
512,
512,
1048576,
2048,
2048,
1048576,
512,
512,
2359296,
512,
512,
524288,
2048,
2048,
2097152,
1024,
1024,
262144,
256,
256,
589824,
256,
256,
262144,
1024,
1024,
262144,
256,
256,
589824,
256,
256,
262144,
1024,
1024,
262144,
256,
256,
589824,
256,
256,
262144,
1024,
1024,
262144,
256,
256,
589824,
256,
256,
262144,
1024,
1024,
262144,
256,
256,
589824,
256,
256,
262144,
1024,
1024,
262144,
256,
256,
589824,
256,
256,
131072,
1024,
1024,
524288,
512,
512,
65536,
128,
128,
147456,
128,
128,
65536,
512,
512,
65536,
128,
128,
147456,
128,
128,
65536,
512,
512,
65536,
128,
128,
147456,
128,
128,
65536,
512,
512,
65536,
128,
128,
147456,
128,
128,
32768,
512,
512,
131072,
256,
256,
16384,
64,
64,
36864,
64,
64,
16384,
256,
256,
16384,
64,
64,
36864,
64,
64,
16384,
256,
256,
16384,
64,
64,
36864,
64,
64,
4096,
256,
256,
16384,
64,
64,
9408
};

bool tag_crt_step[OPS] = {false};
bool tag_lst_step[OPS] = {false};

//pointers of the input/output buffers
float* grad_ptrs[OPS];
float* sum_grad_ptrs[OPS];

//schedules of solo allreduces
ffschedule_h solo_allreduces_sched[OPS];

class allreducef : public deep500::CustomOperator {
  protected:
    int m_len;
    int64_t m_totalbytes;
  public:
    allreducef(int len) : m_len(len), m_totalbytes(0) {
    }

    virtual ~allreducef () {
    }

    virtual bool supports_cuda() { return true; }
    
    virtual int64_t report(void *data) {
        return m_totalbytes;
    }

    void forward(const float *input, const float*, float *output) {

    srand(16);
    int pid, nprocs;
    double begin, elaspe, avg_elaspe, min_elaspe;
    ffrank(&pid); 
    ffsize(&nprocs); 
        
    //init all the solo-allreduce schedules in the first call 
    if(counter == 0)
    {  
        for(int i=0; i<OPS; i++) 
        {
            grad_ptrs[i] = (float *)calloc(length[i], sizeof(float));
            sum_grad_ptrs[i] = (float *)calloc(length[i], sizeof(float));
            ffsolo_allreduce(grad_ptrs[i], sum_grad_ptrs[i], length[i], solo_coll_tag++, FFSUM, FFFLOAT, 0, LIMITER, &(solo_allreduces_sched[i]));
        }

        MPI_Barrier(MPI_COMM_WORLD);

        for(int i=0; i<OPS; i++)
            ffschedule_start(solo_allreduces_sched[i]);
    }
        
    memcpy(grad_ptrs[counter%OPS], input, sizeof(float)*m_len);

    //post the sends for solo activations 
    ffschedule_post(solo_allreduces_sched[counter%OPS]);
    
    //wait for allreduce to be done
    ffschedule_wait(solo_allreduces_sched[counter%OPS]);

    memcpy(output, sum_grad_ptrs[counter%OPS], sizeof(float)*m_len); 

    for(int i=0; i<length[counter%OPS]; i++)
    {
        grad_ptrs[counter%OPS][i] = 0.0;
    }   
    
    counter++;
    
    }
    

    void backward(const float *nextop_grad,
                  const float *fwd_input_tensor,
                  const float*,
                  const float *fwd_output_tensor,
                  float *input_tensor_grad,
                  float *) {
    }
};


D500_EXPORTED void *create_new_op(deep500::tensor_t *input_descriptors, 
                                  int num_inputs,
                                  deep500::tensor_t *output_descriptors,
                                  int num_outputs) {
    if(is_init == 0)
    {
        ffinit(NULL, NULL);
        is_init = 1;
    }

    size_t totalsz = 1;
    for (int i = 0; i < input_descriptors[0].dims; ++i)
        totalsz *= input_descriptors[0].sizes[i];

    return new allreducef(totalsz);
}
D500_REGISTER_OP(allreducef);
"""
