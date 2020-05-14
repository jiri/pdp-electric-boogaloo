#include "Util.hpp"

#include <cassert>
#include <cstdio>

void printf_vector(const char *title, const std::vector<uint8_t> &values) {
    printf("%s: [", title);
    for (int i = 0; i < values.size(); i++) {
        if (i > 0) printf(" ");
        printf("%d", values[i]);
    }
    printf("]\n");
}

void printf_solution(const char *title, uint64_t value, int n) {
    printf("%s: [", title);
    for (int i = 0; i < n; i++) {
        if (i > 0) printf(" ");
        printf("%d", (value & (1 << i)) ? 2 : 1);
    }
    printf("]\n");
}

int32_t Util::invert(int32_t group) {
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

void Util::print_usage_and_exit(int argc, const char **argv) {
    fprintf(stderr, "Usage: %s PROBLEM", argv[0]);
    exit(EXIT_FAILURE);
}
