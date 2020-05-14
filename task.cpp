#include <cstdint>
#include <cstdio>
#include <cassert>

#include <string_view>
#include <vector>

#include "Problem.hpp"
#include "Util.hpp"

using VSolution = std::vector<uint8_t>;

struct Result {
    VSolution solution;
    float weight = std::numeric_limits<float>::infinity();
};

bool operator<(const Result& a, const Result& b) {
    return a.weight < b.weight;
}

Problem problem;

float bestWeight = std::numeric_limits<float>::infinity();

int32_t invert(int32_t group) {
    if (group == 1) {
        return 2;
    }
    else if (group == 2) {
        return 1;
    }
    else {
        assert(false);
    }
}

std::vector<std::vector<Edge>> bedges;

[[nodiscard]]
Result solve(int pos, VSolution solution, float weight) {
    assert(pos > 0);

    // Satisfy exclusions
    if (problem.exclusions.count(pos - 1) > 0) {
        solution[problem.exclusions.at(pos - 1)] = invert(solution[pos - 1]);
    }

    // Calculate the weight
    for (auto [a, b, v] : bedges[pos - 1]) {
        if (solution[a] != solution[b]) {
            weight += v;
        }
    }

    // Can't do better
    if (bestWeight < weight) {
        return { solution, std::numeric_limits<float>::infinity() };
    }

    if (pos == problem.n) {
        if (bestWeight > weight) {
            bestWeight = weight;
        }
        return { solution, weight };
    }

    // Value already set
    if (solution[pos] != 0) {
        return solve(pos + 1, solution, weight);
    }

    // Recurse
    Result result1, result2;

    solution[pos] = 1;
#pragma omp task shared(result1) if (pos < problem.n - 10)
    result1 = solve(pos + 1, solution, weight);

    solution[pos] = 2;
#pragma omp task shared(result2) if (pos < problem.n - 10)
    result2 = solve(pos + 1, solution, weight);

#pragma omp taskwait

    return std::min(result1, result2);
}

[[nodiscard]]
Result solve() {
    VSolution solution;
    solution.resize(problem.n);
    solution[0] = 1;

    bedges.resize(problem.n);
    for (auto& e : problem.edges) {
        auto [a, b, v] = e;
        bedges[b].push_back(e);
    }

    return solve(1, solution, 0.0f);
}

int main(int argc, const char** argv) {
    /* Load data */
    problem = Problem::load(argc, argv);

    /* Solve problem */
    Result result;

    auto elapsed_time = timed {
        #pragma omp parallel
        {
            #pragma omp single
            {
                result = solve();
            }
        }
    };

    /* Print results */
    printf("Variant: Task parallelism\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf_vector("Solution", result.solution);
    printf("Weight: %f\n", result.weight);
    printf("Elapsed time: %3fs", elapsed_time.count());

    return 0;
}