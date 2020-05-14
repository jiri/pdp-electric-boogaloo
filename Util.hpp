#pragma once

#include <chrono>
#include <vector>

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

void printf_vector(const char* title, const std::vector<uint8_t>& values);
void printf_solution(const char* title, uint64_t value, int n);

namespace Util {
    int32_t invert(int32_t group);
    void print_usage_and_exit(int argc, const char** argv);
}