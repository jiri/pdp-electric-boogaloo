#include "Problem.hpp"

#include <cassert>
#include <string>

#include <mpi.h>

#include "Util.hpp"

using namespace std::string_literals;

Problem Problem::load(std::string_view path) {
    Problem p;

    p.name = path;

    FILE *file = fopen(path.data(), "r");
    assert(file);

    fscanf(file, "%u %u %u", &p.n, &p.k, &p.b);

    Node a, b;
    float value;
    for (int32_t i = 0; i < p.n * p.k / 2; i++) {
        fscanf(file, "%u %u %f", &a, &b, &value);
        p.edges.emplace_back(a, b, value);
    }

    for (int32_t i = 0; i < p.b; i++) {
        fscanf(file, "%u %u", &a, &b);
        p.exclusions[a] = b;
    }

    fclose(file);
    return p;
}

Problem Problem::load(int argc, const char **argv) {
    if (argc < 2) {
        Util::print_usage_and_exit(argc, argv);
    }

    return Problem::load(argv[1]);
}

#ifdef USE_MPI

void Problem::send(int dest) const {
    // Params
    MPI_Send(&this->n, 1, MPI_UINT32_T, dest, 0, MPI_COMM_WORLD);
    MPI_Send(&this->k, 1, MPI_UINT32_T, dest, 0, MPI_COMM_WORLD);
    MPI_Send(&this->b, 1, MPI_UINT32_T, dest, 0, MPI_COMM_WORLD);

    // Edges
    int32_t edges_size = this->edges.size();
    MPI_Send(&edges_size, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);

    for (const auto& [a, b, v] : this->edges) {
        MPI_Send(&a, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);
        MPI_Send(&b, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);
        MPI_Send(&v, 1, MPI_FLOAT, dest, 0, MPI_COMM_WORLD);
    }

    // Exclusions
    int32_t exclusions_size = this->exclusions.size();
    MPI_Send(&exclusions_size, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);

    for (const auto& [a, b] : this->exclusions) {
        MPI_Send(&a, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);
        MPI_Send(&b, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);
    }
}

Problem Problem::receive(int src) {
    Problem result;

    MPI_Recv(&result.n, 1, MPI_UINT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&result.k, 1, MPI_UINT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&result.b, 1, MPI_UINT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Edges
    int32_t edges_size;
    MPI_Recv(&edges_size, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (size_t i = 0; i < edges_size; i++) {
        Node a;
        Node b;
        float v;

        MPI_Recv(&a, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&b, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&v, 1, MPI_FLOAT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        result.edges.emplace_back(a, b, v);
    }

    // Exclusions
    int32_t exclusions_size;
    MPI_Recv(&exclusions_size, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (size_t i = 0; i < exclusions_size; i++) {
        Node a;
        Node b;

        MPI_Recv(&a, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&b, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        result.exclusions[a] = b;
    }

    return result;
}

#endif
