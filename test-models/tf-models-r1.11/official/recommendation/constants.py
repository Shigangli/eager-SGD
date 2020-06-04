# Copyright 2018 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Central location for NCF specific values."""

import os
import time


# ==============================================================================
# == Main Thread Data Processing ===============================================
# ==============================================================================
class Paths(object):
  """Container for various path information used while training NCF."""

  def __init__(self, data_dir, cache_id=None):
    self.cache_id = cache_id or int(time.time())
    self.data_dir = data_dir
    self.cache_root = os.path.join(
        self.data_dir, "{}_ncf_recommendation_cache".format(self.cache_id))
    self.train_shard_subdir = os.path.join(self.cache_root,
                                           "raw_training_shards")
    self.train_shard_template = os.path.join(self.train_shard_subdir,
                                             "positive_shard_{}.pickle")
    self.train_epoch_dir = os.path.join(self.cache_root, "training_epochs")
    self.eval_data_subdir = os.path.join(self.cache_root, "eval_data")
    self.eval_raw_file = os.path.join(self.eval_data_subdir, "raw.pickle")
    self.eval_record_template_temp = os.path.join(self.eval_data_subdir,
                                                  "eval_records.temp")
    self.eval_record_template = os.path.join(
        self.eval_data_subdir, "padded_eval_batch_size_{}.tfrecords")
    self.subproc_alive = os.path.join(self.cache_root, "subproc.alive")


APPROX_PTS_PER_TRAIN_SHARD = 128000

# In both datasets, each user has at least 20 ratings.
MIN_NUM_RATINGS = 20

# The number of negative examples attached with a positive example
# when performing evaluation.
NUM_EVAL_NEGATIVES = 999

# ==============================================================================
# == Subprocess Data Generation ================================================
# ==============================================================================
CYCLES_TO_BUFFER = 3  # The number of train cycles worth of data to "run ahead"
                      # of the main training loop.

READY_FILE = "ready.json"
TRAIN_RECORD_TEMPLATE = "train_{}.tfrecords"

TIMEOUT_SECONDS = 3600 * 2  # If the train loop goes more than two hours without
                            # consuming an epoch of data, this is a good
                            # indicator that the main thread is dead and the
                            # subprocess is orphaned.
