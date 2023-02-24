#ifndef LANDMARKS_GREEDY_HITTING_SET_HEURISTIC_H
#define LANDMARKS_GREEDY_HITTING_SET_HEURISTIC_H

#include "dalm_heuristic.h"

namespace landmarks {
    class DisjunctiveActionLandmarkGreedyHittingSetHeuristic : public DisjunctiveActionLandmarkHeuristic {
        std::vector<int> min_costs;
        std::vector<bool> landmark_active;
        std::vector<int> op_hits;

        int get_heuristic_value(const State &ancestor_state) override;
    public:
        explicit DisjunctiveActionLandmarkGreedyHittingSetHeuristic(
                const plugins::Options &opts);

        virtual bool dead_ends_are_reliable() const override;
    };
}

#endif
