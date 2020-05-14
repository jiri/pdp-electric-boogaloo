all: sequential task data mpi

sequential: sequential.cpp Problem.cpp Problem.hpp Util.cpp Util.hpp
	g++ sequential.cpp Problem.cpp Util.cpp -o sequential --std=c++2a -g -O3

task: task.cpp Problem.cpp Problem.hpp Util.cpp Util.hpp
	g++ task.cpp Problem.cpp Util.cpp -o task --std=c++2a -g -O3 -fopenmp

data: data.cpp Problem.cpp Problem.hpp Util.cpp Util.hpp
	g++ data.cpp Problem.cpp Util.cpp -o _data --std=c++2a -g -O3 -fopenmp

mpi: mpi.cpp Problem.cpp Problem.hpp Util.cpp Util.hpp
	mpic++ -DUSE_MPI mpi.cpp Problem.cpp Util.cpp -o mpi --std=c++2a -g -O3