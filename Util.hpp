#pragma once

#include <cassert>
#include <chrono>

#define timed Timer() + [&]()

class Timer {
public:
    template <typename F>
    std::chrono::duration<double> operator+(F&& f) {
        auto start_time = std::chrono::steady_clock::now();
        f();
        auto end_time = std::chrono::steady_clock::now();

        return end_time - start_time;
    }
};

void printf_vector(const char* title, const std::vector<uint8_t>& values) {
    printf("%s: [", title);
    for (int i = 0; i < values.size(); i++) {
        if (i > 0) printf(" ");
        printf("%d", values[i]);
    }
    printf("]\n");
}

void printf_solution(const char* title, uint64_t value, int n) {
    printf("%s: [", title);
    for (int i = 0; i < n; i++) {
        if (i > 0) printf(" ");
        printf("%d", (value & (1 << i)) ? 2 : 1);
    }
    printf("]\n");
}

namespace Util {
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

    void print_usage_and_exit(int argc, const char** argv) {
        fprintf(stderr, "Usage: %s PROBLEM", argv[0]);
        exit(EXIT_FAILURE);
    }
}