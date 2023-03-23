#ifndef LANDMARKS_CYCLIC_LANDMARK_HEURISTIC_H
#define LANDMARKS_CYCLIC_LANDMARK_HEURISTIC_H

#include "dalm_heuristic.h"

#include "../lp/lp_solver.h"

namespace operator_counting {
class ConstraintGenerator;
class LandmarkConstraints;
}

namespace landmarks {
class CyclicLandmarkHeuristic : public DisjunctiveActionLandmarkHeuristic {
    lp::LPSolver lp_solver;
    const bool ip;
    std::vector<std::shared_ptr<operator_counting::ConstraintGenerator>> constraint_generators;

    void prepare_linear_program();
    named_vector::NamedVector<lp::LPVariable> compute_lp_variables();

    virtual int get_heuristic_value(const State &ancestor_state) override;

public:
    explicit CyclicLandmarkHeuristic(const plugins::Options &opts);

    // TODO: Check if this makes sens.
    virtual bool dead_ends_are_reliable() const override {return true;};
};
}

#endif
