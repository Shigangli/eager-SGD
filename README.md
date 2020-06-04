Eager-SGD
---------

Eager-SGD is a decentralized asynchronous SGD. It utilizes novel partial collectives operations to accumulate the gradients across all the processes. Different from the traditional collectives operations (such as MPI, NCCL), a partial collective is an asynchronous operation where a subset of the processes can trigger and complete the collective operation. Due to the asynchrony feature of eager-SGD, it can better handle the deep learning training with load imbalance.


Publication
-----------

If you use eager-SGD, cite us:
```bibtex
@inproceedings{li2020taming,
  title={Taming unbalanced training workloads in deep learning with partial collective operations},
  author={Li, Shigang and Ben-Nun, Tal and Girolamo, Salvatore Di and Alistarh, Dan and Hoefler, Torsten},
  booktitle={Proceedings of the 25th ACM SIGPLAN Symposium on Principles and Practice of Parallel Programming (PPoPP'20)},
  pages={45--61},
  year={2020}
}
```

License
-------
See [LICENSE](LICENSE).
