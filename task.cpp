#include <cstdint>
#include <cstdio>
#include <cassert>
#include <string_view>
#include <vector>

#include <omp.h>

#include "Problem.hpp"
#include "Util.hpp"

constexpr size_t THRESHOLD = 10;

using VSolution = std::vector<uint8_t>;

Problem problem;

VSolution bestSolution;
float bestWeight = std::numeric_limits<float>::infinity();

std::vector<std::vector<Edge>> bedges;

void solve(int pos, VSolution solution, float weight) {
    assert(pos > 0);

    // Satisfy exclusions
    if (problem.exclusions.count(pos - 1) > 0) {
        solution[problem.exclusions.at(pos - 1)] = Util::invert(solution[pos - 1]);
    }

    // Calculate the weight
    for (auto [a, b, v] : bedges[pos - 1]) {
        if (solution[a] != solution[b]) {
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
        solve(pos + 1, std::move(solution), weight);
        return;
    }

    // Recurse
    #pragma omp task if (pos < problem.n - THRESHOLD)
    {
        solution[pos] = 1;
        solve(pos + 1, solution, weight);
    }

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

    // Load data
    problem = Problem::load(argc, argv);

    // Calculate bedges
    bedges.resize(problem.n);
    for (auto& e : problem.edges) {
        auto [a, b, v] = e;
        bedges[b].push_back(e);
    }

    // Solve problem
    auto elapsed_time = timed {
        VSolution solution;
        solution.resize(problem.n);
        solution[0] = 1;

        #pragma omp parallel
        {
            #pragma omp single
            {
                solve(1, solution, 0.0f);
            }
        }
    };

    // Print results
    printf("Variant: Task parallelism\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf("Threads: %d\n", omp_get_num_threads());
    printf_vector("Solution", bestSolution);
    printf("Weight: %f\n", bestWeight);
    printf("Elapsed time: %3fs\n", elapsed_time.count());

    return 0;
}