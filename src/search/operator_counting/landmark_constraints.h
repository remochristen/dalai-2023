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
    CycleGenerator cycle_generator;
    const bool strong;
    const std::shared_ptr<landmarks::DisjunctiveActionLandmarkGraph> lm_graph;
    const std::shared_ptr<landmarks::DisjunctiveActionLandmarkStatusManager> lm_status_manager;
    //static lp::LPConstraint compute_constraint(
    //    const std::set<int> &actions, double infinity);

    std::vector<std::vector<int>> cycles;

    void add_landmark_constraints(
        named_vector::NamedVector<lp::LPConstraint> &constraints,
        double infinity) const;
    void add_johnson_cycle_constraints(
        named_vector::NamedVector<lp::LPConstraint> &constraints,
        double infinity);
    void remove_strong_orderings_from_cycles();

    lp::LPConstraint compute_constraint(const std::vector<int> &cycle,
                                        double infinity) const;
    std::unordered_map<int, size_t> count_lm_occurrences(
        const std::vector<int> &cycle) const;

    bool update_landmark_constraints(const State &ancestor_state,
                                     lp::LPSolver &lp_solver);
    bool update_cycle_constraints(const State &ancestor_state,
                                  lp::LPSolver &lp_solver);
    bool add_cycle_constraints_implicit_hitting_set_approach(
        const State &ancestor_state, lp::LPSolver &lp_solver);

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

    static void add_options_to_feature(plugins::Feature &feature);
};
}

#endif
