#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string_view>

using Node = int32_t;
using Edge = std::tuple<Node, Node, float>;
using Solution = uint64_t;

class Problem {
public:
    std::string name = "<anonymous>";

    uint32_t n = 0;
    uint32_t k = 0;
    uint32_t b = 0;

    std::vector<Edge> edges;
    std::unordered_map<Node, Node> exclusions;

    static Problem load(int argc, const char** argv);
    static Problem load(std::string_view path);

#ifdef USE_MPI
    // OpenMPI convenience functions
    void send(int dest) const;
    static Problem receive(int src);
#endif
};
