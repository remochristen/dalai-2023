#include "landmark_constraints.h"

#include "../algorithms/johnson_cycle_detection.h"
#include "../landmarks/dalm_graph.h"
#include "../landmarks/dalm_status_manager.h"
#include "../plugins/plugin.h"

using namespace landmarks;
using namespace std;

namespace operator_counting {
static AdjacencyList compute_adj_list(
    const DisjunctiveActionLandmarkGraph &lm_graph,
    const DisjunctiveActionLandmarkStatusManager &/*lm_status_manager*/) {
    int num_lms = static_cast<int>(lm_graph.get_number_of_landmarks());
    AdjacencyList adj(num_lms, vector<int>{});
    for (int id = 0; id < num_lms; ++id) {
        for (auto &parent : lm_graph.get_dependencies(id)) {
            if (!lm_graph.is_true_in_initial(parent.first)) {
                adj[parent.first].push_back(id);
            }
        }
    }
    return adj;
}

LandmarkConstraints::LandmarkConstraints(
    const plugins::Options &opts,
    const shared_ptr<DisjunctiveActionLandmarkGraph> &lm_graph,
    const shared_ptr<DisjunctiveActionLandmarkStatusManager> &lm_status_manager)
    : cycle_generator(opts.get<CycleGenerator>("cycle_generator")),
      strong(opts.get<bool>("strong")),
      lm_graph(lm_graph),
      lm_status_manager(lm_status_manager) {
}

void LandmarkConstraints::add_landmark_constraints(
    named_vector::NamedVector<lp::LPConstraint> &constraints,
    double infinity) const {
    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());
    for (int lm_id = 0; lm_id < num_landmarks; ++lm_id) {
        constraints.emplace_back(0, infinity);
        lp::LPConstraint &constraint = constraints.back();
        for (int op_id : lm_graph->get_actions(lm_id)) {
            constraint.insert(op_id, 1);
        }
    }
}

void LandmarkConstraints::add_johnson_cycle_constraints(
    named_vector::NamedVector<lp::LPConstraint> &constraints,
    double infinity) {
    AdjacencyList adj = compute_adj_list(*lm_graph, *lm_status_manager);
    for (auto &list : adj) {
        sort(list.begin(), list.end());
    }
    cycles = johnson_cycles::compute_elementary_cycles(adj);
    if (strong) {
        remove_strong_orderings_from_cycles();
    }
    utils::g_log << "Johnson's algorithm found " << cycles.size()
                 << " cycles in the initial state." << endl;

    if (cycles.empty()) {
        utils::g_log << "Turning off cycle heuristic." << endl;
        cycle_generator = CycleGenerator::NONE;
        return;
    }

    for (const vector<int> &cycle : cycles) {
        constraints.push_back(compute_constraint(cycle, infinity));
    }
}

void LandmarkConstraints::remove_strong_orderings_from_cycles() {
    assert(strong);
    for (auto &cycle : cycles) {
        vector<int> filtered_cycle{};
        for (size_t j = 0; j < cycle.size(); ++j) {
            int id = cycle[j];
            int succ = cycle[(j + 1) % cycle.size()];
            if (lm_graph->get_ordering_type(id, succ) == OrderingType::WEAK) {
                filtered_cycle.push_back(succ);
            }
        }
        cycle = filtered_cycle;
    }
}

lp::LPConstraint LandmarkConstraints::compute_constraint(
    const vector<int> &cycle, double infinity) const {
    unordered_map<int, size_t> counts = count_lm_occurrences(cycle);

    lp::LPConstraint constraint(cycle.size() + 1.0, infinity);
    for (pair<int, size_t> op : counts) {
        constraint.insert(op.first, op.second);
    }
    return constraint;
}

unordered_map<int, size_t> LandmarkConstraints::count_lm_occurrences(
    const vector<int> &cycle) const {
    unordered_map<int, size_t> occurrences{};
    for (int id : cycle) {
        for (int op : lm_graph->get_actions(id)) {
            occurrences[op] = occurrences.count(op) ? occurrences[op] + 1 : 1;
        }
    }
    return occurrences;
}

void LandmarkConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> &/*task*/, lp::LinearProgram &lp) {
    assert(lm_graph);
    named_vector::NamedVector<lp::LPConstraint> &constraints =
        lp.get_constraints();
    double infinity = lp.get_infinity();
    add_landmark_constraints(constraints, infinity);
    if (cycle_generator == CycleGenerator::JOHNSON) {
        add_johnson_cycle_constraints(constraints, infinity);
    }
}

bool LandmarkConstraints::update_constraints(
    const State &ancestor_state, lp::LPSolver &lp_solver) {
    bool dead_end = update_landmark_constraints(ancestor_state, lp_solver);
    if (cycle_generator != CycleGenerator::NONE) {
        dead_end &= update_cycle_constraints(ancestor_state, lp_solver);
    }
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

bool LandmarkConstraints::update_cycle_constraints(const State &ancestor_state,
                                                   lp::LPSolver &lp_solver) {
    assert(cycle_generator != CycleGenerator::NONE);
    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());
    int num_cycles = static_cast<int>(cycles.size());
    if (cycle_generator == CycleGenerator::JOHNSON) {
        assert(!cycles.empty());
        for (int i = 0; i < num_cycles; ++i) {
            bool cycle_active = all_of(
                cycles[i].begin(), cycles[i].end(), [&](int id) {
                    return lm_status_manager->get_landmark_status(
                        ancestor_state, id) == landmarks::FUTURE;
                });
            double lower_bound = cycle_active ? cycles[i].size() + 1.0 : 0.0;
            lp_solver.set_constraint_lower_bound(
                num_landmarks + i, lower_bound);
        }
    } else {
        utils::g_log << "Only JOHNSON cycle generation implemented yet."
                     << endl;
    }
    return false;
}

//void LandmarkConstraints::add_constraints_for_all_cycles(
//    const State &ancestor_state, lp::LPSolver &lp_solver) {
//    AdjacencyList adj =
//        compute_adj_list(*lm_graph, *lm_status_manager, ancestor_state);
//    for (auto &list : adj) {
//        sort(list.begin(), list.end());
//    }
//    //cycles = johnson_cycles::compute_elementary_cycles(adj);
//    //utils::g_log << "Johnson's algorithm found " << cycles.size()
//    //             << " cycles in the initial state." << endl;
//    //
//    //if (cycles.empty()) {
//    //    utils::g_log << "Turning off cycle heuristic." << endl;
//    //    cycle_generator = CycleGenerator::NONE;
//    //    return;
//    //}
//
//    vector<lp::LPConstraint> constraints{};
//    constraints.reserve(cycles.size());
//    for (const vector<int> &cycle : cycles) {
//        constraints.push_back(
//            compute_constraint(cycle, lp_solver.get_infinity()));
//    }
//    lp_solver.add_temporary_constraints(constraints);
//}

void LandmarkConstraints::add_options_to_feature(plugins::Feature &feature) {
    feature.add_option<CycleGenerator>(
        "cycle_generator",
        "method to generate cycles",
        "floyd_warshall");
    feature.add_option<bool>(
        "strong",
        "use observation that predecessors of strong orderings are not "
        "candidates to be reached twice within a cycle.",
        "true");
}

static plugins::TypedEnumPlugin<CycleGenerator> _enum_plugin({
    {"none", "add no cycle constraints"},
    {"johnson", "add all elementary cycles found using Johnson's algorithm"},
    {"floyd_warshall", "use oracle approach with Floyd-Warshall's algorithm"},
    {"depth_first", "use oracle approach with DFS search"}
});
}
