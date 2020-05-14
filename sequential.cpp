#include <cstdint>
#include <cstdio>
#include <cassert>

#include <vector>

#include "Problem.hpp"
#include "Util.hpp"

using VSolution = std::vector<uint8_t>;

Problem problem;
VSolution solution;

float bestWeight = std::numeric_limits<float>::infinity();
VSolution bestSolution;

// Basic Branch & Bounds solution
void solve(int pos, float weight) {
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
            bestWeight = weight;
            bestSolution = solution;
        }
        return;
    }

    // Value already set
    if (solution[pos] != 0) {
        solve(pos + 1, weight);
        return;
    }

    // Recurse
    solution[pos] = 1;
    solve(pos + 1, weight);

    solution[pos] = 2;
    solve(pos + 1, weight);
}

int main(int argc, const char** argv) {
    problem = Problem::load(argc, argv);

    /* Solve problem */
    auto elapsed_time = timed {
        solution.resize(problem.n);
        solution[0] = 1;

        solve(1, 0.0f);
    };

    /* Print results */
    printf("Variant: Sequential\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf_vector("Solution", bestSolution);
    printf("Weight: %f\n", bestWeight);
    printf("Elapsed time: %3fs", elapsed_time.count());

    return 0;
}

