#ifndef LANDMARKS_DALM_SUM_HEURISTIC_H
#define LANDMARKS_DALM_SUM_HEURISTIC_H

#include "dalm_heuristic.h"

namespace landmarks {
class DisjunctiveActionLandmarkSumHeuristic : public DisjunctiveActionLandmarkHeuristic {
    std::vector<int> min_costs;

    void compute_landmark_costs();

    int get_heuristic_value(const State &ancestor_state) override;
public:
    explicit DisjunctiveActionLandmarkSumHeuristic(
        const plugins::Options &opts);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
