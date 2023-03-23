#ifndef LANDMARKS_FLOYD_WARSHALL_ORACLE_H
#define LANDMARKS_FLOYD_WARSHALL_ORACLE_H

#include "cycle_oracle.h"

#include "../algorithms/floyd_warshall.h"

#include <unordered_map>
#include <vector>

namespace landmarks {
class FloydWarshallOracle : public CycleOracle {
    // We're never interested in values >1, so 1.0 is infinite enough.
    static constexpr float infinity = 1.0;
    static constexpr float epsilon = 0.001;

    std::vector<float> weights;
    std::vector<std::vector<float>> dist;
    std::vector<std::vector<int>> next;

    void compute_weights(const std::vector<float> &counts);
    void floyd_warshall();
    void initialize_floyd_warshall();
    void execute_floyd_warshall();
    std::vector<int> extract_cycle_floyd_warshall() const;
    int find_shortest_cycle_index() const;
    std::vector<int> reconstruct_path(int source, int target) const;
public:
    explicit FloydWarshallOracle(
        const std::vector<std::unordered_map<int, bool>> &graph,
        bool ignore_ordering_types);
    std::vector<int> find_cycle(
        const std::vector<float> &counts) override;
};
}

#endif
