Eager-SGD
---------

**Eager-SGD** is a **decentralized asynchronous SGD** for distributed deep learning training based on **gradient averaging**. It utilizes novel partial collectives operations (partial allreduce) to accumulate the gradients across all the processes. Different from the traditional collectives operations (such as MPI, NCCL), a partial collective is an asynchronous operation where a subset of the processes can trigger and contribute the latest data to the collective operation. 

Eager-SGD may bring staleness to the gradients. Thanks to our sophisticated implementation of solo-allreduce and majority-allreduce, the **staleness is bounded** and therefore eager-SGD is stale-synchronous. Due to the asynchrony feature of eager-SGD, it can better handle the deep learning training with load imbalance. To the best of our knowledge, this is the first work that implements asynchronous and stale-synchronous decentralized SGD where the messages propagate to all nodes in one step.

Demo
---------
A script to run eager-SGD on ResNet-50/ImageNet with SLURM job scheduler can be found [here](https://github.com/Shigangli/eager-SGD/blob/master/test-models/tf-models-r1.11/official/resnet/test_scripts_imagenet/daint_eagersgd_imagenet.sh).
Generally, to evaluate other neural network models with the [customized optimizers](https://github.com/Shigangli/eager-SGD/blob/master/test-models/tf-models-r1.11/official/utils/) (e.g., gradient averaging using solo/majority-allreduce), one can simply wrap the default optimizer using the customized optimizers. See the example for ResNet-50 [here](https://github.com/Shigangli/eager-SGD/blob/master/test-models/tf-models-r1.11/official/resnet/resnet_run_loop_solo_imagenet_300.py#L384).



Publication
-----------
The work of eager-SGD is pulished in PPoPP'20. See the paper for details. If you use eager-SGD, cite us:
```bibtex
@inproceedings{li2020taming,
  title={Taming unbalanced training workloads in deep learning with partial collective operations},
  author={Li, Shigang and Ben-Nun, Tal and Girolamo, Salvatore Di and Alistarh, Dan and Hoefler, Torsten},
  booktitle={Proceedings of the 25th ACM SIGPLAN Symposium on Principles and Practice of Parallel Programming},
  pages={45--61},
  year={2020}
}
```

License
-------
See [LICENSE](LICENSE).
