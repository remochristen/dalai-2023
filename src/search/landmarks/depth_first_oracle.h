#ifndef CYCLIC_LANDMARKS_DEPTH_FIRST_ORACLE_H
#define CYCLIC_LANDMARKS_DEPTH_FIRST_ORACLE_H

#include "cycle_oracle.h"

#include <vector>

namespace landmarks {
class DepthFirstOracle : public CycleOracle {
    std::vector<bool> blocked;
    std::vector<int> stack;
    std::vector<float> counts;

    void unblock(size_t start_index, size_t n);
    std::vector<int> circuit(int v, int s, float accumulated_weight);
    std::vector<int> process_successor(int w, int s,
                                       float accumulated_weight);

public:
    explicit DepthFirstOracle(
        const std::vector<std::unordered_map<int, bool>> &graph,
        bool ignore_ordering_types);
    std::vector<int> find_cycle(
        const std::vector<float> &counts) override;
};
}

#endif
