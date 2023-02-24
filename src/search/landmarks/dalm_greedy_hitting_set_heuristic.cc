#include "dalm_greedy_hitting_set_heuristic.h"

#include "dalm_graph.h"
#include "dalm_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

using namespace std;

namespace landmarks {
    DisjunctiveActionLandmarkGreedyHittingSetHeuristic::DisjunctiveActionLandmarkGreedyHittingSetHeuristic(
            const plugins::Options &opts)
            : DisjunctiveActionLandmarkHeuristic(opts),
              op_hits(task_proxy.get_operators().size(), 0) {
        if (log.is_at_least_normal()) {
            log << "Initializing disjunctive action landmark sum heuristic..."
                << endl;
        }
        initialize(opts);
        landmark_active = vector<bool>(lm_graph->get_number_of_landmarks(), false);
    }

    int DisjunctiveActionLandmarkGreedyHittingSetHeuristic::get_heuristic_value(
            const State &ancestor_state) {

        for (int id = 0; id < (int)landmark_active.size(); ++id) {
            if (lm_status_manager->get_landmark_status(
                    ancestor_state, id) != PAST) {
                landmark_active[id] = true;
                for (int op_id : lm_graph->get_actions(id)) {
                    op_hits[op_id]++;
                }
            } else {
                landmark_active[id] = false;
            }
        }

        int h = 0;
        while (true) {
            // Find op that hits max number of landmarks
            int chosen_op_id = 0;
            for (size_t i = 1; i < op_hits.size(); ++i) {
                if (op_hits[i] > op_hits[chosen_op_id]) {
                    chosen_op_id = i;
                }
            }

            // No operator hits an active landmark -> we're done
            if (op_hits[chosen_op_id] == 0) {
                return h;
            }

            h += task_proxy.get_operators()[chosen_op_id].get_cost();
            // Update active landmarks and operator hits
            for (int lm_id : get_landmark_ids_for_operator(chosen_op_id)) {
                if (landmark_active[lm_id]) {
                    for (int op_id : lm_graph->get_actions(lm_id)) {
                        op_hits[op_id]--;
                    }
                }
                landmark_active[lm_id] = false;
            }
        }

    }

    bool DisjunctiveActionLandmarkGreedyHittingSetHeuristic::dead_ends_are_reliable() const {
        // TODO: Check if this is actually true.
        return false;
    }

    class DisjunctiveActionLandmarkGreedyHittingSetHeuristicFeature : public plugins::TypedFeature<Evaluator, DisjunctiveActionLandmarkGreedyHittingSetHeuristic> {
    public:
        DisjunctiveActionLandmarkGreedyHittingSetHeuristicFeature() : TypedFeature("dalm_greedy_hs") {
            document_title("TODO");
            document_synopsis("TODO");
            DisjunctiveActionLandmarkHeuristic::add_options_to_feature(*this);

            // TODO: document language support.
            // TODO: document properties.
        }
    };

    static plugins::FeaturePlugin<DisjunctiveActionLandmarkGreedyHittingSetHeuristicFeature> _plugin;
}
