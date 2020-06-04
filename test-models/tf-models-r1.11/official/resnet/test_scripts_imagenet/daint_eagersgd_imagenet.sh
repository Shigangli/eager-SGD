#!/bin/bash -l
#SBATCH --nodes=64
#SBATCH --ntasks=64
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=12
#SBATCH --constraint=gpu
#SBATCH --partition=normal
#SBATCH --time=05:35:00
#SBATCH --output=eagersgd.txt

module load daint-gpu
module load cudatoolkit

module unload PrgEnv-cray
module load PrgEnv-gnu

which python
source ~/.bashrc
export SLURM_NTASKS_PER_NODE=1
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export HDF5_USE_FILE_LOCKING=FALSE

export PYTHONPATH=

HOROVOD_FLAG="-solo"

DISABLE_WARMUP=

TESTNAME="test-eagersgd/"


# one epoch to warm up
cd ../ 
srun -n $SLURM_NTASKS --ntasks-per-node=$SLURM_NTASKS_PER_NODE -c $SLURM_CPUS_PER_TASK python imagenet_main_eagersgd_one_epoch.py -dd  -md  $HOROVOD_FLAG -ebe 100 -rs 50 -rv 1 -bs 128 $DISABLE_WARMUP

for((i=1;i<=9;i++));
do
cd /path/to/model/checkpoint
./synchm.sh

cd -
# ImageNet FB run
srun -n $SLURM_NTASKS --ntasks-per-node=$SLURM_NTASKS_PER_NODE -c $SLURM_CPUS_PER_TASK python imagenet_main_eagersgd.py -dd  -md  $HOROVOD_FLAG -ebe 100 -rs 50 -rv 1 -bs 128 $DISABLE_WARMUP
done

