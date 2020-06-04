#--------------------------------------------------
# find MPI
#--------------------------------------------------
find_package(MPI REQUIRED)

#--------------------------------------------------
# setup tests
#--------------------------------------------------
set(TEST_C_SRCS
  send_recv
  ping_pong_dep
  simple_computation
  custom_computation
  ping_pong_schedule
  allreduce
  allreduce_inplace
  allreduce_buffers_user_managed
  allreduce_buffers_user_managed_inplace
  allreduce_buffers_user_managed_callback
  allreduce_buffers_fflib_managed
  allreduce_buffers_fflib_managed_inplace
  activation_1
  activation_tree
  activation_tree_multiple
  first_dep
  limiter
  solo_allreduce_single
  solo_allreduce_multiple
  solo_allreduce_correctness
  ssgd
  ssgd_synch_send
  ssgd_synch_recv
  rsgd
  randomsgd
  rsgd_synch_send
  rsgd_synch_recv
  rand_allreduce
  rand_allreduce_correctness
)

set(TEST_CPP_SRCS
)

include_directories(${MPI_C_INCLUDE_PATH})
message("include_directories(${MPI_INCLUDE_PATH})")

foreach(test ${TEST_C_SRCS})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} fflib)
endforeach()

foreach(test ${TEST_CPP_SRCS})
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} fflib)
endforeach()


