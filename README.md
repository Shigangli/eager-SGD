# eager-SGD
Eager-SGD is a decentralized asynchronous SGD. It utilizes novel partial collectives operations to accumulate the gradients across all the processes. Different from the traditional collectives operations (such as MPI, NCCL), a partial collective is an asynchronous operation where a subset of the processes can trigger and complete the collective operation.
