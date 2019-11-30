# eager-SGD
Eager-SGD is a decentralized asynchronous SGD. It utilizes novel partial collectives operations to accumulate the gradients across all the processes. Different from the traditional collectives operations (such as MPI, NCCL), a partial collective is an asynchronous operation where a subset of the processes can trigger and complete the collective operation.

# Reference
This work was accepted as a regular paper of PPoPP'20. A preprint version can be downloaded from Arxiv: https://arxiv.org/pdf/1908.04207.pdf
