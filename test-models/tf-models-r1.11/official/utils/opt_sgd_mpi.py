import deep500 as d5
from deep500.frameworks import tensorflow as d5tf
import os
import tensorflow as tf

class EagerSGDOptimizer(object):
    """ Enforces a sequential order on gradient exchange (by creating false 
        dependencies) and invokes a Deep500 custom operator based on MPI. """

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
                print("gradient is None")
            else:
                if last_g is None:
                    last_g = grad
                self.compiled_op.op.inputs = [d5tf.desc_from_tensor(grad), d5tf.desc_from_tensor(last_g)]
                self.compiled_op.op.outputs = [d5tf.desc_from_tensor(grad)]
                op, lib, handle = d5tf.custom_op(self.compiled_op, return_handle=True)
                self._handles.append((lib, handle))
                # Apply on gradient
                allreduced = op(grad, last_g)
                new_gvs.append((allreduced / (self.comm_size), var))
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

int is_init = 0;
int counter = 0;

//number of allreduces for one step of ResNet-50

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
         
        int pid, nprocs;
        double begin, elaspe, avg_elaspe, min_elaspe;
        MPI_Comm_rank(MPI_COMM_WORLD,&pid);
        MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
        counter++;
        MPI_Allreduce(input, output, m_len, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    }


    void backward(const float *nextop_grad,
                  const float *fwd_input_tensor,
                  const float*,
                  const float *fwd_output_tensor,
                  float *input_tensor_grad,
                  float *) {
      // Do Nothing here
    }
};


D500_EXPORTED void *create_new_op(deep500::tensor_t *input_descriptors, 
                                  int num_inputs,
                                  deep500::tensor_t *output_descriptors,
                                  int num_outputs) {
    int is_init = 0;
    MPI_Initialized(&is_init);
    if (!is_init)
        MPI_Init(NULL, NULL);

    size_t totalsz = 1;
    for (int i = 0; i < input_descriptors[0].dims; ++i)
        totalsz *= input_descriptors[0].sizes[i];

    return new allreducef(totalsz);
}

D500_REGISTER_OP(allreducef);
"""
