#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cmath>

#include <string_view>

#include <omp.h>

#include "Problem.hpp"
#include "Util.hpp"

using VSolution = std::vector<uint8_t>;

struct SuspendedExecution {
    int pos;
    VSolution solution;
    float weight;
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

int main(int argc, const char** argv) {
    // Override thread count
    if (argc >= 3) {
        int num_threads = std::stoi(argv[2]);
        omp_set_dynamic(0);
        omp_set_num_threads(num_threads);
    }

    maxDepth = log2(omp_get_num_threads()) + 1;

    // Load data
    problem = Problem::load(argc, argv);

    // Find partial solutions
    VSolution solution;
    solution.resize(problem.n);
    solution[0] = 1;

    partial_solve(1, std::move(solution), 0.0f, 0);

    // Solve problem
    auto elapsed_time = timed {
        #pragma omp parallel
        {
            #pragma omp for schedule(dynamic)
            for (size_t i = 0; i < suspensions.size(); i++) {
                solve(suspensions[i].pos, std::move(suspensions[i].solution), suspensions[i].weight);
            }
        }
    };

    // Print results
    printf("Variant: Data parallelism\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf("Threads: %d\n", omp_get_num_threads());
    printf_vector("Solution", bestSolution);
    printf("Weight: %f\n", bestWeight);
    printf("Elapsed time: %3fs\n", elapsed_time.count());

    return 0;
}