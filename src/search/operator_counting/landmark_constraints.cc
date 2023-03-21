#include "landmark_constraints.h"

#include "../landmarks/dalm_graph.h"
#include "../landmarks/dalm_status_manager.h"
#include "../plugins/plugin.h"

using namespace landmarks;
using namespace std;

namespace operator_counting {
LandmarkConstraints::LandmarkConstraints(
    const plugins::Options &/*options*/,
    const shared_ptr<DisjunctiveActionLandmarkGraph> &lm_graph,
    const shared_ptr<DisjunctiveActionLandmarkStatusManager> &lm_status_manager)
    : lm_graph(lm_graph), lm_status_manager(lm_status_manager) {
}

void LandmarkConstraints::add_landmark_constraints(
    named_vector::NamedVector<lp::LPConstraint> &constraints,
    double infinity) const {
    assert(lm_graph);
    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());
    for (int lm_id = 0; lm_id < num_landmarks; ++lm_id) {
        constraints.emplace_back(0, infinity);
        lp::LPConstraint &constraint = constraints.back();
        for (int op_id : lm_graph->get_actions(lm_id)) {
            constraint.insert(op_id, 1);
        }
    }
}

void LandmarkConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> &/*task*/, lp::LinearProgram &lp) {
    named_vector::NamedVector<lp::LPConstraint> &constraints =
        lp.get_constraints();
    double infinity = lp.get_infinity();
    add_landmark_constraints(constraints, infinity);
}

bool LandmarkConstraints::update_constraints(
    const State &state, lp::LPSolver &lp_solver) {
    bool dead_end = update_landmark_constraints(state, lp_solver);
    return dead_end;
}

bool LandmarkConstraints::update_landmark_constraints(
    const State &state, lp::LPSolver &lp_solver) {
    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());
    for (int id = 0; id < num_landmarks; ++id) {
        if (lm_status_manager->get_landmark_status(state, id)
            == landmarks::PAST) {
            lp_solver.set_constraint_lower_bound(id, 0);
        } else {
            lp_solver.set_constraint_lower_bound(id, 1);
        }
    }
    return false;
}
}
