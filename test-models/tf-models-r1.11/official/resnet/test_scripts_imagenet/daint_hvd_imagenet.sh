#!/bin/bash -l
#SBATCH --nodes=64
#SBATCH --ntasks=64
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=12
#SBATCH --constraint=gpu
#SBATCH --partition=normal
#SBATCH --time=00:30:00
#SBATCH --output=hvd.txt
module load daint-gpu

export LD_LIBRARY_PATH=

export PATH=

module load Horovod/0.16.4-CrayGNU-19.10-tf-1.14.0
source /../pythonVE/bin/activate

export SLURM_NTASKS_PER_NODE=1
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export PYTHONPATH=$PYTHONPATH:

HOROVOD_FLAG="-hvd"

DISABLE_WARMUP=
TESTNAME="test-hvd"


export NCCL_DEBUG=INFO
export NCCL_IB_HCA=ipogif0
export NCCL_IB_CUDA_SUPPORT=1

cd ..
pwd
# ImageNet FB run
srun -n $SLURM_NTASKS --ntasks-per-node=$SLURM_NTASKS_PER_NODE -c $SLURM_CPUS_PER_TASK python imagenet_main_hvd.py -dd -md $HOROVOD_FLAG -ebe 10 -rs 50 -rv 1 -bs 128 --log_dir= $DISABLE_WARMUP
