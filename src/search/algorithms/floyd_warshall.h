#ifndef ALGORITHMS_FLOYD_WARSHALL_H
#define ALGORITHMS_FLOYD_WARSHALL_H

#include <unordered_map>
#include <vector>

namespace floyd_warshall {
using adj_list = std::vector<std::unordered_map<int, float>>;

class FloydWarshall {
    const adj_list &graph;
    const bool cyclic;
    const float infinity;

    std::vector<std::vector<float>> dist;
    std::vector<std::vector<int>> next;

    void initialize();
    void execute();
public:
    explicit FloydWarshall(const adj_list &graph, bool cyclic,
                           float infinity);
    void run();
    float get_length(int source, int target) const;
    std::vector<int> reconstruct_path(int source, int target);
};
}

#endif
