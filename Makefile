all: sequential task data mpi

sequential: sequential.cpp Problem.cpp Problem.hpp Util.hpp
	g++ sequential.cpp Problem.cpp -o sequential --std=c++2a -g -O3

task: task.cpp Problem.cpp Problem.hpp Util.hpp
	g++ task.cpp Problem.cpp -o task --std=c++2a -g -O3 -fopenmp

data: data.cpp Problem.cpp Problem.hpp Util.hpp
	g++ data.cpp Problem.cpp -o data --std=c++2a -g -O3 -fopenmp

mpi: mpi.cpp Problem.cpp Problem.hpp Util.hpp
	mpic++ mpi.cpp Problem.cpp -o mpi --std=c++2a -g -O3