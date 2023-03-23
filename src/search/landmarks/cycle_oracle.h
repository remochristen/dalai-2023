#ifndef LANDMARKS_CYCLE_ORACLE_H
#define LANDMARKS_CYCLE_ORACLE_H

#include <unordered_map>
#include <vector>

namespace landmarks {
using weighted_graph = std::vector<std::unordered_map<int, float>>;

class CycleOracle {
protected:
    const std::vector<std::unordered_map<int, bool>> graph;
    bool ignore_ordering_types;
public:
    CycleOracle() = delete;
    explicit CycleOracle(
        const std::vector<std::unordered_map<int, bool>> &graph,
        bool ignore_ordering_types);
    virtual ~CycleOracle() = default;
    virtual std::vector<int> find_cycle(
        const std::vector<float> &counts) = 0;
};
}

#endif
