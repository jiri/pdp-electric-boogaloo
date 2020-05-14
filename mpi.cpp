#include <cstdio>

#include <mpi.h>

#include "Problem.hpp"
#include "Util.hpp"

struct Result {
    Solution solution;
    float weight = std::numeric_limits<float>::infinity();

    void send(int dest) const {
        MPI_Send(&this->weight, 1, MPI_FLOAT, dest, 0, MPI_COMM_WORLD);
        MPI_Send(&this->solution, 1, MPI_UINT64_T, dest, 0, MPI_COMM_WORLD);
    }

    static Result receive(int src) {
        Result result;

        MPI_Recv(&result.weight, 1, MPI_FLOAT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&result.solution, 1, MPI_UINT64_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        return result;
    }
};

bool operator<(const Result& a, const Result& b) {
    return a.weight < b.weight;
}

[[nodiscard]]
Result evaluate(const Problem& problem, const Solution& solution) {
    Result res { solution, 0.0f };

    for (auto [a, b] : problem.exclusions) {
        if (((solution & (1 << a)) >> a) == ((solution & (1 << b)) >> b)) {
            res.weight = std::numeric_limits<float>::infinity();
            return res;
        }
    }

    for (auto [a, b, v] : problem.edges) {
        if (((solution & (1 << a)) >> a) != ((solution & (1 << b)) >> b)) {
            res.weight += v;
        }
    }

    return res;
}

int main(int argc, char** argv) {
    int provided;
    int required = MPI_THREAD_FUNNELED;
    MPI_Init_thread(&argc, &argv, required, &provided);

    int proc_num;
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);

    int num_procs;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    Problem problem;

    Result result;

    auto elapsed_time = timed {
        // Load or receive problem
        if (proc_num == 0) {
            problem = Problem::load(argc, const_cast<const char **>(argv));

            for (int dest = 1; dest < num_procs; dest++) {
                problem.send(dest);
            }
        }
        else {
            problem = Problem::receive(0);
        }

        uint64_t num_solutions = 1UL << (problem.n - 1);
        uint64_t slice_size = (num_solutions / num_procs) + 1;

        uint64_t start_solution = slice_size * proc_num;
        uint64_t end_solution = std::min(slice_size * (proc_num+1), num_solutions);

        #pragma omp parallel
        {
            Result localBestResult;

            #pragma omp for schedule(guided, 8)
            for (Solution i = start_solution; i < end_solution; i++) {
                localBestResult = std::min(localBestResult, evaluate(problem, i));
            }

            #pragma omp critical
            {
                result = std::min(result, localBestResult);
            }
        }

        // Send result and exit
        if (proc_num != 0) {
            result.send(0);
            MPI_Finalize();
            exit(0);
        }

        // Aggregate results
        for (size_t src = 1; src < num_procs; src++) {
            auto localBestResult = Result::receive(src);
            result = std::min(result, localBestResult);
        }
    };

    // Print results
    printf("Variant: OpenMPI\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf_solution("Solution", result.solution, problem.n);
    printf("Weight: %f\n", result.weight);
    printf("Elapsed time: %3fs", elapsed_time.count());

    MPI_Finalize();

    return 0;
}