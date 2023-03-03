#include "dalm_sum_heuristic.h"

#include "dalm_graph.h"
#include "dalm_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

using namespace std;

namespace landmarks {
DisjunctiveActionLandmarkSumHeuristic::DisjunctiveActionLandmarkSumHeuristic(
    const plugins::Options &opts)
    : DisjunctiveActionLandmarkHeuristic(opts) {
    if (log.is_at_least_normal()) {
        log << "Initializing disjunctive action landmark sum heuristic..."
            << endl;
    }
    initialize(opts);
    compute_landmark_costs();
}

void DisjunctiveActionLandmarkSumHeuristic::compute_landmark_costs() {
    /*
      This function runs under the assumption that landmark node IDs go
      from 0 to the number of landmarks - 1, therefore the entry in
      *min_first_achiever_costs* and *min_possible_achiever_costs*
      at index i corresponds to the entry for the landmark node with ID i.
    */

    const vector<int> &op_costs =
        task_properties::get_operator_costs(task_proxy);

    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());
    min_costs.reserve(num_landmarks);
    for (int lm_id = 0; lm_id < num_landmarks; ++lm_id) {
        int min_cost = numeric_limits<int>::max();
        for (int op_id : lm_graph->get_actions(lm_id)) {
            min_cost = min(min_cost, op_costs[op_id]);
        }
        min_costs.push_back(min_cost);
    }
}

int DisjunctiveActionLandmarkSumHeuristic::get_heuristic_value(
    const State &ancestor_state) {
    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());

    int h = 0;
    for (int id = 0; id < num_landmarks; ++id) {
        if (lm_status_manager->get_landmark_status(
            ancestor_state, id) != PAST) {
            if (min_costs[id] == numeric_limits<int>::max()) {
                return DEAD_END;
            }
            h += min_costs[id];
        }
    }
    return h;
}

bool DisjunctiveActionLandmarkSumHeuristic::dead_ends_are_reliable() const {
    // TODO: Check if this is actually true.
    return false;
}

class DisjunctiveActionLandmarkSumHeuristicFeature : public plugins::TypedFeature<Evaluator, DisjunctiveActionLandmarkSumHeuristic> {
public:
    DisjunctiveActionLandmarkSumHeuristicFeature() : TypedFeature("dalm_sum") {
        document_title("TODO");
        document_synopsis("TODO");
        DisjunctiveActionLandmarkHeuristic::add_options_to_feature(*this);

        // TODO: document language support.
        // TODO: document properties.
    }
};

static plugins::FeaturePlugin<DisjunctiveActionLandmarkSumHeuristicFeature> _plugin;
}
