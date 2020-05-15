#include <cassert>
#include <cstdio>
#include <cmath>
#include <optional>
#include <queue>
#include <deque>

#include <omp.h>
#include <mpi.h>

#include "Problem.hpp"
#include "Util.hpp"

using VSolution = std::vector<uint8_t>;

struct SuspendedExecution {
    int pos;
    VSolution solution;
    float weight;

    void send(int dest) const {
        MPI_Send(&this->pos, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);

        // Solution
        int32_t solution_size = this->solution.size();
        MPI_Send(&solution_size, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);

        for (const auto& v : this->solution) {
            MPI_Send(&v, 1, MPI_UINT8_T, dest, 0, MPI_COMM_WORLD);
        }

        MPI_Send(&this->weight, 1, MPI_FLOAT, dest, 0, MPI_COMM_WORLD);
    }

    static std::optional<SuspendedExecution> receive(int src) {
        SuspendedExecution result;

        MPI_Recv(&result.pos, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (result.pos == -1) {
            return std::nullopt;
        }

        // Solution
        int32_t solution_size;
        MPI_Recv(&solution_size, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        result.solution.resize(solution_size);

        for (size_t i = 0; i < solution_size; i++) {
            uint8_t v;
            MPI_Recv(&v, 1, MPI_UINT8_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            result.solution[i] = v;
        }

        MPI_Recv(&result.weight, 1, MPI_FLOAT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        return result;
    }
};

struct Result {
    VSolution solution;
    float weight;

    void send(int dest) const {
        // Solution
        int32_t solution_size = this->solution.size();
        MPI_Send(&solution_size, 1, MPI_INT32_T, dest, 0, MPI_COMM_WORLD);

        for (const auto& v : this->solution) {
            MPI_Send(&v, 1, MPI_UINT8_T, dest, 0, MPI_COMM_WORLD);
        }

        MPI_Send(&this->weight, 1, MPI_FLOAT, dest, 0, MPI_COMM_WORLD);
    }

    static Result receive(int src) {
        Result result;

        // Solution
        int32_t solution_size;
        MPI_Recv(&solution_size, 1, MPI_INT32_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        result.solution.reserve(solution_size);

        for (size_t i = 0; i < solution_size; i++) {
            uint8_t v;
            MPI_Recv(&v, 1, MPI_UINT8_T, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            result.solution.push_back(v);
        }

        MPI_Recv(&result.weight, 1, MPI_FLOAT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        return result;
    }
};

Problem problem;

VSolution bestSolution;
float bestWeight = std::numeric_limits<float>::infinity();

int maxDepth = 0;
std::vector<SuspendedExecution> suspensions;

void partial_solve(int pos, VSolution solution, float weight, int depth) {
    assert(pos > 0);

    // Satisfy exclusions
    if (problem.exclusions.count(pos - 1) > 0) {
        solution[problem.exclusions.at(pos - 1)] = Util::invert(solution[pos - 1]);
    }

    // Calculate the weight
    for (auto [a, b, v] : problem.edges) {
        if (b == pos - 1 && solution[a] != solution[b]) {
            weight += v;
        }
    }

    // Can't do better
    if (bestWeight < weight) {
        return;
    }

    if (pos == problem.n) {
        if (bestWeight > weight) {
            #pragma omp critical
            {
                if (bestWeight > weight) {
                    bestSolution = solution;
                    bestWeight = weight;
                }
            }
        }
        return;
    }

    // Value already set
    if (solution[pos] != 0) {
        partial_solve(pos + 1, std::move(solution), weight, depth);
        return;
    }

    if (depth >= maxDepth) {
        solution[pos] = 1;
        suspensions.push_back({ pos + 1, solution, weight });

        solution[pos] = 2;
        suspensions.push_back({ pos + 1, std::move(solution), weight });
    }
    else {
        // Recurse
        solution[pos] = 1;
        partial_solve(pos + 1, solution, weight, depth + 1);

        solution[pos] = 2;
        partial_solve(pos + 1, std::move(solution), weight, depth + 1);
    }
}

void solve(int pos, VSolution solution, float weight) {
    assert(pos > 0);

    // Satisfy exclusions
    if (problem.exclusions.count(pos - 1) > 0) {
        solution[problem.exclusions.at(pos - 1)] = Util::invert(solution[pos - 1]);
    }

    // Calculate the weight
    for (auto [a, b, v] : problem.edges) {
        if (b == pos - 1 && solution[a] != solution[b]) {
            weight += v;
        }
    }

    // Can't do better
    if (bestWeight < weight) {
        return;
    }

    if (pos == problem.n) {
        if (bestWeight > weight) {
            #pragma omp critical
            {
                if (bestWeight > weight) {
                    bestSolution = solution;
                    bestWeight = weight;
                }
            }
        }
        return;
    }

    // Value already set
    if (solution[pos] != 0) {
        return solve(pos + 1, std::move(solution), weight);
    }

    // Recurse
    solution[pos] = 1;
    solve(pos + 1, solution, weight);

    solution[pos] = 2;
    solve(pos + 1, std::move(solution), weight);
}

#define LOG(format, ...) printf(("#%d " format "\n"), proc_num __VA_OPT__(,) __VA_ARGS__)

int main(int argc, char** argv) {
    int provided;
    int required = MPI_THREAD_FUNNELED;
    MPI_Init_thread(&argc, &argv, required, &provided);

    int proc_num;
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);

    int num_procs;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Load or receive problem
    if (proc_num == 0) {
        LOG("Inside master");
        problem = Problem::load(argc, const_cast<const char **>(argv));

        for (int dest = 1; dest < num_procs; dest++) {
            LOG("Sending problem to %d", dest);
            problem.send(dest);
        }

        LOG("Sent problems");

        // Calculate maxDepth for workers
        maxDepth = log2(num_procs) + 1;
        LOG("Setting maxDepth = %d", maxDepth);

        auto elapsed_time = timed {
            // Find partial solutions
            VSolution solution;
            solution.resize(problem.n);
            solution[0] = 1;

            partial_solve(1, std::move(solution), 0.0f, 0);

            std::queue<int> workers;
            for (int i = 1; i < num_procs; i++) {
                workers.push(i);
            }

            std::deque<SuspendedExecution> jobs;
            std::move(suspensions.begin(), suspensions.end(), std::back_inserter(jobs));

            while (!jobs.empty() || workers.size() < num_procs - 1) {
                while (!jobs.empty() && !workers.empty()) {
                    uint32_t worker = workers.front();
                    workers.pop();

                    jobs.front().send(worker);
                    jobs.pop_front();
                }

                int worker_id;
                MPI_Recv(&worker_id, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                auto result = Result::receive(worker_id);
                if (result.weight < bestWeight) {
                    bestSolution = std::move(result.solution);
                    bestWeight = result.weight;
                }

                LOG("Worker %d done", maxDepth);
                workers.push(worker_id);
            }

            int minus_one = -1;
            while (!workers.empty()) {
                MPI_Send(&minus_one, 1, MPI_INT, workers.front(), 0, MPI_COMM_WORLD);
                workers.pop();
            }
        };

        // Print results
        printf("Variant: Data parallelism\n");
        printf("Problem: %s\n", problem.name.c_str());
        printf_vector("Solution", bestSolution);
        printf("Weight: %f\n", bestWeight);
        printf("Elapsed time: %3fs\n", elapsed_time.count());

        MPI_Finalize();
        exit(EXIT_SUCCESS);
    }
    else {
        problem = Problem::receive(0);
        LOG("Problem received [n=%d]", problem.n);

        // Calculate maxDepth for threads
        maxDepth = log2(omp_get_num_threads()) + 1;

        while (auto job = SuspendedExecution::receive(0)) {
            LOG("Job received [pos=%d, weight=%f]", job->pos, job->weight);

            // Find partial solutions
            partial_solve(job->pos, std::move(job->solution), job->weight, 0);
            LOG("%d partial solutions found", suspensions.size());

            #pragma omp parallel
            {
                #pragma omp for schedule(dynamic)
                for (size_t i = 0; i < suspensions.size(); i++) {
                    LOG("Processing job %d [pos=%d, solution.size=%u, weight=%f]", i, suspensions[i].pos, suspensions[i].solution.size(), suspensions[i].weight);
                    printf_vector("DEBUG", suspensions[i].solution);
                    solve(suspensions[i].pos, std::move(suspensions[i].solution), suspensions[i].weight);
                    LOG("Done with job %d", i);
                }
            }

            LOG("Solution found");

            suspensions.clear();

            MPI_Send(&proc_num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            Result result { std::move(bestSolution), bestWeight };
            result.send(0);

            LOG("Solution sent");
        }

        MPI_Finalize();
        exit(EXIT_SUCCESS);
    }

    MPI_Finalize();

    return 0;
}