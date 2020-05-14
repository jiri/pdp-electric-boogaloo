all: sequential task data mpi

sequential:
	g++ sequential.cpp Problem.cpp -o sequential --std=c++2a -g -O3

task:
	g++ task.cpp Problem.cpp -o task --std=c++2a -g -O3 -fopenmp

data:
	g++ data.cpp Problem.cpp -o data --std=c++2a -g -O3 -fopenmp

mpi:
	mpic++ mpi.cpp Problem.cpp -o mpi --std=c++2a -g -O3