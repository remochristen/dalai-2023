#include "dalm_heuristic.h"

#include "dalm_graph_factory.h"
#include "dalm_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"

using namespace std;

namespace landmarks {
DisjunctiveActionLandmarkHeuristic::DisjunctiveActionLandmarkHeuristic(
    const plugins::Options &opts)
    : Heuristic(opts),
      use_preferred_operators(opts.get<bool>("pref")),
      successor_generator(nullptr) {
}

void DisjunctiveActionLandmarkHeuristic::initialize(const plugins::Options &opts) {
    /*
      Actually, we should test if this is the root task or a
      CostAdaptedTask *of the root task*, but there is currently no good
      way to do this, so we use this incomplete, slightly less safe test.
    */
    if (task != tasks::g_root_task
        && dynamic_cast<tasks::CostAdaptedTask *>(task.get()) == nullptr) {
        cerr << "The landmark heuristics currently only support "
             << "task transformations that modify the operator costs. "
             << "See issues 845 and 686 for details." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    compute_landmark_graph(opts);
    precompute_landmarks_by_operator_lookup();

    lm_status_manager =
        make_shared<DisjunctiveActionLandmarkStatusManager>(*lm_graph);
        //utils::make_unique_ptr<DisjunctiveActionLandmarkStatusManager>(*lm_graph);

    if (use_preferred_operators) {
        /* Ideally, we should reuse the successor generator of the main
           task in cases where it's compatible. See issue564. */
        successor_generator =
            make_shared<successor_generator::SuccessorGenerator>(task_proxy);
    }
}

void DisjunctiveActionLandmarkHeuristic::compute_landmark_graph(
    const plugins::Options &opts) {
    utils::Timer lm_graph_timer;
    if (log.is_at_least_normal()) {
        log << "Generating landmark graph..." << endl;
    }

    auto lm_graph_factory =
        opts.get<shared_ptr<LandmarkGraphFactory>>("lm_factory");
    lm_graph = lm_graph_factory->compute_landmark_graph(task);

    if (log.is_at_least_normal()) {
        log << "Landmark graph generation time: " << lm_graph_timer << endl;
        log << "Landmark graph contains " << lm_graph->get_number_of_landmarks()
            << " landmarks." << endl;
        log << "Landmark graph contains " << lm_graph->get_number_of_orderings()
            << " orderings." << endl;
    }
}

void DisjunctiveActionLandmarkHeuristic::generate_preferred_operators(
    const State &ancestor_state) {
    // Find operators that achieve landmarks and prefer them.
    assert(successor_generator);
    vector<OperatorID> applicable_operators;
    State state = convert_ancestor_state(ancestor_state);
    successor_generator->generate_applicable_ops(state, applicable_operators);

    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorID op_id : applicable_operators) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        for (int lm_id : landmarks_by_operator[op.get_id()]) {
            if (lm_status_manager->get_future_landmarks(
                ancestor_state).test(lm_id)) {
                set_preferred(operators[op_id]);
                break;
            }
        }
    }
}

int DisjunctiveActionLandmarkHeuristic::compute_heuristic(
    const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int h = get_heuristic_value(ancestor_state);

    if (use_preferred_operators) {
        generate_preferred_operators(ancestor_state);
    }

    return h;
}

void DisjunctiveActionLandmarkHeuristic::precompute_landmarks_by_operator_lookup() {
    int num_landmarks = static_cast<int>(lm_graph->get_number_of_landmarks());
    for (int lm_id = 0; lm_id < num_landmarks; ++lm_id) {
        for (auto op_id : lm_graph->get_actions(lm_id)) {
            if (!landmarks_by_operator.count(op_id)) {
                landmarks_by_operator[op_id] = {};
            }
            landmarks_by_operator[op_id].push_back(lm_id);
        }
    }
}

void DisjunctiveActionLandmarkHeuristic::notify_initial_state(
    const State &initial_state) {
    lm_status_manager->process_initial_state(initial_state, log);
}

void DisjunctiveActionLandmarkHeuristic::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    lm_status_manager->process_state_transition(parent_state, op_id, state);
    if (cache_evaluator_values) {
        /* TODO:  It may be more efficient to check that the reached landmark
            set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
    }
}


void DisjunctiveActionLandmarkHeuristic::add_options_to_feature(plugins::Feature &feature) {
    feature.add_option<shared_ptr<LandmarkGraphFactory>>(
        "lm_factory",
        "the set of landmarks to use for this heuristic. "
        "The set of landmarks can be specified here, "
        "or predefined (see LandmarkFactory).");
    feature.add_option<bool>(
        "pref",
        "identify preferred operators (see OptionCaveats#"
        "Using_preferred_operators_with_landmark_heuristics)",
        "false");

    Heuristic::add_options_to_feature(feature);

    feature.document_property("preferred operators",
                              "yes (if enabled; see ``pref`` option)");
}
}


