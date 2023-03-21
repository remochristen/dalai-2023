#ifndef OPERATOR_COUNTING_CYCLIC_LANDMARK_CONSTRAINTS_H
#define OPERATOR_COUNTING_CYCLIC_LANDMARK_CONSTRAINTS_H

#include "constraint_generator.h"

#include "../lp/lp_solver.h"
#include "../plugins/options.h"

#include <set>

namespace landmarks {
class DisjunctiveActionLandmarkGraph;
class DisjunctiveActionLandmarkStatusManager;
}

namespace operator_counting {
enum class CycleGenerator {
    NONE,
    JOHNSON,
    FLOYD_WARSHALL,
    DEPTH_FIRST,
};

class LandmarkConstraints : public ConstraintGenerator {
    const std::shared_ptr<landmarks::DisjunctiveActionLandmarkGraph> lm_graph;
    const std::shared_ptr<landmarks::DisjunctiveActionLandmarkStatusManager> lm_status_manager;
    //static lp::LPConstraint compute_constraint(
    //    const std::set<int> &actions, double infinity);

    void add_landmark_constraints(
        named_vector::NamedVector<lp::LPConstraint> &constraints,
        double infinity) const;

    bool update_landmark_constraints(
        const State &state, lp::LPSolver &lp_solver);

public:
    LandmarkConstraints(
        const plugins::Options &options,
        const std::shared_ptr<landmarks::DisjunctiveActionLandmarkGraph> &lm_graph,
        const std::shared_ptr<landmarks::DisjunctiveActionLandmarkStatusManager> &lm_status_manager);
    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> &task,
        lp::LinearProgram &lp) override;
    virtual bool update_constraints(
        const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif
