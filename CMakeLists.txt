cmake_minimum_required(VERSION 3.16)
project(MI_PDP_2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3")

# Libraries
find_package(fmt REQUIRED)
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
# find_package(Microsoft.GSL REQUIRED)

# Problem loading
add_library(problem Problem.cpp)
target_link_libraries(problem PUBLIC fmt::fmt MPI::MPI_CXX)

# Sequential solution
add_executable(sequential sequential.cpp)
target_link_libraries(sequential PUBLIC problem)

# Task parallelism
add_executable(task_parallelism task.cpp)
target_link_libraries(task_parallelism PUBLIC problem OpenMP::OpenMP_CXX)

# Data parallelism
add_executable(data_parallelism data.cpp)
target_link_libraries(data_parallelism PUBLIC problem OpenMP::OpenMP_CXX)

# OpenMPI
add_executable(openmpi mpi.cpp)
target_link_libraries(openmpi PUBLIC problem)
