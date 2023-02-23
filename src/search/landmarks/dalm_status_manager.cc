#include "dalm_status_manager.h"

#include "landmark.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks past, since we do an intersection when
  computing new landmark information.
*/
DisjunctiveActionLandmarkStatusManager::DisjunctiveActionLandmarkStatusManager(
    DisjunctiveActionLandmarkGraph &graph,
    bool progress_goals,
    bool progress_greedy_necessary_orderings,
    bool progress_weak_orderings)
    : lm_graph(graph),
      progress_goals(progress_goals),
      progress_greedy_necessary_orderings(progress_greedy_necessary_orderings),
      progress_weak_orderings(progress_weak_orderings),
      past_lms(vector<bool>(graph.get_number_of_landmarks(), true)),
      future_lms(vector<bool>(graph.get_number_of_landmarks(), false)) {
}

BitsetView DisjunctiveActionLandmarkStatusManager::get_past_landmarks(const State &state) {
    return past_lms[state];
}

BitsetView DisjunctiveActionLandmarkStatusManager::get_future_landmarks(const State &state) {
    return future_lms[state];
}

void DisjunctiveActionLandmarkStatusManager::process_initial_state(
    const State &initial_state, utils::LogProxy &/*log*/) {
    BitsetView past = get_past_landmarks(initial_state);
    BitsetView future = get_future_landmarks(initial_state);
    past.reset();
    future.set();
    for (size_t id : lm_graph.get_initially_past_lms()) {
        past.set(static_cast<int>(id));
        future.reset(static_cast<int>(id));
    }
    if (progress_weak_orderings) {
        progress_weak(past, future);
    }
}

void DisjunctiveActionLandmarkStatusManager::process_state_transition(
    const State &parent_ancestor_state, OperatorID op_id,
    const State &ancestor_state) {

    const BitsetView parent_past = get_past_landmarks(parent_ancestor_state);
    BitsetView past = get_past_landmarks(ancestor_state);

    const BitsetView parent_fut = get_future_landmarks(parent_ancestor_state);
    BitsetView fut = get_future_landmarks(ancestor_state);

    int num_landmarks = static_cast<int>(lm_graph.get_number_of_landmarks());
    utils::unused_variable(num_landmarks);
    assert(past.size() == num_landmarks);
    assert(parent_past.size() == num_landmarks);
    assert(fut.size() == num_landmarks);
    assert(parent_fut.size() == num_landmarks);

    progress_basic(parent_past, parent_fut, past, fut, op_id.get_index());

    if (progress_goals) {
        progress_goal(ancestor_state, fut);
    }
    if (progress_greedy_necessary_orderings) {
        progress_greedy_necessary(ancestor_state, past, fut);
    }
    if (progress_weak_orderings) {
        progress_weak(past, fut);
    }
}

void DisjunctiveActionLandmarkStatusManager::progress_basic(
    const BitsetView &parent_past, const BitsetView &parent_fut,
    BitsetView &past, BitsetView &fut, int op_id) {

    utils::unused_variable(parent_fut);
    int num_landmarks = static_cast<int>(lm_graph.get_number_of_landmarks());

    // TODO: Is there a more efficient way to do this?
    for (int lm_id = 0; lm_id < num_landmarks; ++lm_id) {
        if (!parent_past.test(lm_id)) {
            assert(parent_fut.test(lm_id));
            if (past.test(lm_id)
                && lm_graph.get_actions(lm_id).count(op_id) == 0) {
                past.reset(lm_id);
                fut.set(lm_id);
            }
        }
    }
}

void DisjunctiveActionLandmarkStatusManager::progress_goal(
    const State &ancestor_state, BitsetView &fut) {
    for (const auto &entry : lm_graph.get_goal_achiever_lms()) {
        const FactPair &fact_pair = entry.first;
        int lm_id = static_cast<int>(entry.second);
        // TODO: Does this check make sense?
        if (ancestor_state[fact_pair.var].get_value() != fact_pair.value) {
            fut.set(lm_id);
        }
    }
}

void DisjunctiveActionLandmarkStatusManager::progress_greedy_necessary(
    const State &ancestor_state, const BitsetView &past, BitsetView &fut) {
    for (const auto &entry : lm_graph.get_precondition_achiever_lms()) {
        const vector<FactPair> &fact_pairs = entry.first.first;
        int preconditioned_id = static_cast<int>(entry.first.second);
        int achiever_id = static_cast<int>(entry.second);
        if (!past.test(preconditioned_id)
            && none_of(fact_pairs.begin(), fact_pairs.end(),
                       [ancestor_state](const FactPair &fact_pair) {
            return ancestor_state[fact_pair.var].get_value() == fact_pair.value;
        })) {
            fut.set(achiever_id);
        }
    }
}

void DisjunctiveActionLandmarkStatusManager::progress_weak(
    const BitsetView &past, BitsetView &fut) {
    int num_landmarks = static_cast<int>(lm_graph.get_number_of_landmarks());
    for (int id = 0; id < num_landmarks; ++id) {
        for (auto ordering : lm_graph.get_dependencies(id)) {
            if (ordering.second == OrderingType::WEAK
                && !past.test(ordering.first)) {
                fut.set(id);
            }
        }
    }
}

LandmarkStatus DisjunctiveActionLandmarkStatusManager::get_landmark_status(
    const State &ancestor_state, size_t id) {
    assert(id < lm_graph.get_number_of_landmarks());

    const BitsetView past = get_past_landmarks(ancestor_state);
    const BitsetView fut = get_future_landmarks(ancestor_state);

    if (!past.test(id)) {
        assert(fut.test(id));
        return FUTURE;
    } else if (!fut.test(id)) {
        assert(past.test(id));
        return PAST;
    } else {
        return PAST_AND_FUTURE;
    }
}
}
