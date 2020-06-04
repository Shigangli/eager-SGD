cmake_minimum_required(VERSION 3.1)
project(d500_custom_op)

# Additional variables
#set(TENSORFLOW_PATH "" CACHE STRING "Path to tensorflow module")
#set(TENSORFLOW_PATH "/apps/dom/UES/jenkins/7.0.UP01/gpu/easybuild/software/TensorFlow/1.14.0-CrayGNU-19.10-cuda-10.1.168-python3/lib/python3.6/site-packages/tensorflow")
set(TENSORFLOW_PATH "/../anaconda/lib/python3.6/site-packages/tensorflow")


include_directories(${TENSORFLOW_PATH}/include)
include_directories(${TENSORFLOW_PATH}/include/external/nsync/public)
include_directories("/../eager-SGD/eager-SGD-modules/fflib2/src")

add_definitions(-DD500_OPNAME=${D500_OPNAME})

# Compiler-specific definitions
if (MSVC_IDE)
    add_definitions(-DCOMPILER_MSVC;-DNOMINMAX)
    link_directories(${TENSORFLOW_PATH}/python)
    link_directories("/../eager-SGD/eager-SGD-modules/fflib2/lib")
else()
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
    #link_directories(${TENSORFLOW_LIB_PATH})
    link_directories(${TENSORFLOW_PATH})
    link_directories("/../eager-SGD/eager-SGD-modules/fflib2/lib")
endif()
include(${CMAKE_SOURCE_DIR}/../../reference/custom_operators/CMakeLists.txt)

if (MSVC_IDE)
    target_link_libraries(${D500_OPNAME} fflib pywrap_tensorflow_internal)
else()
    target_link_libraries(${D500_OPNAME} fflib tensorflow_framework)
endif()

