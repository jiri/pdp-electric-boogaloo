#include <cstdio>

#include <string_view>

#include "Problem.hpp"
#include "Util.hpp"

struct Result {
    Solution solution;
    float weight = std::numeric_limits<float>::infinity();
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

int main(int argc, const char** argv) {
    /* Load data */
    Problem problem = Problem::load(argc, argv);

    /* Solve problem */
    Result result;

    auto elapsed_time = timed {
        #pragma omp parallel
        {
            Result localBestResult;

            #pragma omp for schedule(guided, 8)
            for (Solution i = (1 << (problem.n - 1)); i < (1ul << problem.n); i++) {
                localBestResult = std::min(localBestResult, evaluate(problem, i));
            }

            #pragma omp critical
            {
                result = std::min(result, localBestResult);
            }
        }
    };


    /* Print results */
    printf("Variant: Data parallelism\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf_solution("Solution", result.solution, problem.n);
    printf("Weight: %f\n", result.weight);
    printf("Elapsed time: %3fs\n", elapsed_time.count());

    return 0;
}