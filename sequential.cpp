#include <cstdint>
#include <cstdio>
#include <cassert>

#include <vector>

#include "Problem.hpp"
#include "Util.hpp"

class Solver {
    using Solution = std::vector<uint8_t>;

public:
    Problem& problem;

    float bestWeight = std::numeric_limits<float>::infinity();
    Solution bestSolution;

    explicit Solver(Problem& p)
        : problem { p }
    { }

    void solve() {
        Solution solution;
        solution.resize(this->problem.n);
        solution[0] = 1;

        this->solve(1, solution, 0.0f);
    }

protected:
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

    void solve(int pos, Solution solution, float weight) {
        assert(pos > 0);

        // Satisfy exclusions
        if (this->problem.exclusions.count(pos - 1) > 0) {
            solution[this->problem.exclusions.at(pos - 1)] = this->invert(solution[pos - 1]);
        }

        // Calculate the weight
        for (auto [a, b, v] : this->problem.edges) {
            if (b == pos - 1 && solution[a] != solution[b]) {
                weight += v;
            }
        }

        // Can't do better
        if (this->bestWeight < weight) {
            return;
        }

        if (pos == this->problem.n) {
            if (this->bestWeight > weight) {
                this->bestWeight = weight;
                this->bestSolution = solution;
            }
            return;
        }

        // Value already set
        if (solution[pos] != 0) {
            solve(pos + 1, solution, weight);
            return;
        }

        // Recurse
        solution[pos] = 1;
        solve(pos + 1, solution, weight);

        solution[pos] = 2;
        solve(pos + 1, solution, weight);
    }
};

int main(int argc, const char** argv) {
    Problem problem = Problem::load(argc, argv);

    /* Solve problem */
    Solver solver { problem };

    auto elapsed_time = timed {
        solver.solve();
    };

    /* Print results */
    printf("Variant: Sequential\n");
    printf("Problem: %s\n", problem.name.c_str());
    printf_vector("Solution", solver.bestSolution);
    printf("Weight: %f\n", solver.bestWeight);
    printf("Elapsed time: %3fs", elapsed_time.count());

    return 0;
}

