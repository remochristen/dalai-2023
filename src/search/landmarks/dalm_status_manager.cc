#include "dalm_status_manager.h"

#include "landmark.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks past, since we do an intersection when
  computing new landmark information.
*/
DisjunctiveActionLandmarkStatusManager::DisjunctiveActionLandmarkStatusManager(
    DisjunctiveActionLandmarkGraph &graph)
    : lm_graph(graph),
      progress_uaa_landmarks(graph.has_uaa_landmarks()),
      past_lms(vector<bool>(graph.get_last_relevant_past_id()+1, true)),
      future_lms(vector<bool>(graph.get_number_of_landmarks(), false)) {
}

BitsetView DisjunctiveActionLandmarkStatusManager::get_past_landmarks(
    const State &state) {
    return past_lms[state];
}

BitsetView DisjunctiveActionLandmarkStatusManager::get_future_landmarks(
    const State &state) {
    return future_lms[state];
}

void DisjunctiveActionLandmarkStatusManager::process_initial_state(
    const State &initial_state, utils::LogProxy &/*log*/) {
    BitsetView past = get_past_landmarks(initial_state);
    BitsetView future = get_future_landmarks(initial_state);
    int num_landmarks = static_cast<int>(lm_graph.get_number_of_landmarks());
    for (int id = 0; id < num_landmarks; ++id) {
        assert(lm_graph.is_true_in_initial(id)
               || lm_graph.is_initially_fut(id));
        if ((int) id < past.size() && !lm_graph.is_true_in_initial(id)) {
            past.reset(id);
        }
        if (lm_graph.is_initially_fut(id)) {
            future.set(id);
        }
    }
    progress_weak(past, future);
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
    assert(past.size() == (int)lm_graph.get_last_relevant_past_id()+1);
    assert(parent_past.size() == (int)lm_graph.get_last_relevant_past_id()+1);
    assert(fut.size() == num_landmarks);
    assert(parent_fut.size() == num_landmarks);

    progress_basic(parent_past, parent_fut, past, fut, op_id.get_index());
    progress_goal(ancestor_state, fut);
    progress_greedy_necessary(ancestor_state, past, fut);
    progress_weak(past, fut);
    if (progress_uaa_landmarks) {
        int lm_index = lm_graph.get_uaa_landmark_for_operator(op_id.get_index());
        if (lm_index >= 0) {
            fut.set(lm_index);
        }
    }
}

void DisjunctiveActionLandmarkStatusManager::progress_basic(
    const BitsetView &parent_past, const BitsetView &parent_fut,
    BitsetView &past, BitsetView &fut, int op_id) {

    utils::unused_variable(parent_fut);
    int num_landmarks = static_cast<int>(lm_graph.get_number_of_landmarks());

    // TODO: Is there a more efficient way to do this?
    for (int lm_id = 0; lm_id < num_landmarks; ++lm_id) {
        /*
         * We need to update the landmark information if the parent has "stronger" information
         * (parent: only fut, child: only past/past and fut; or parent: past and fut, child: only past)
         * and the landmark was not achieved in this transition (because otherwise the information would
         * degenerate to "only past").
         * In order to avoid calling count on the action set if possible, we first check for the "stronger" condition.
         * Note that the condition can only fire if the landmark is fut in parent.
         */
        if (parent_fut.test(lm_id)) {
            bool parent_fut_stronger = !fut.test(lm_id);
            bool parent_past_stronger = (lm_id < past.size() && !parent_past.test(lm_id) && past.test(lm_id));
            if ((parent_fut_stronger || parent_past_stronger) && lm_graph.get_actions(lm_id).count(op_id) == 0) {
                if (parent_fut_stronger) {
                    fut.set(lm_id);
                }
                if (parent_past_stronger) {
                    past.reset(lm_id);
                }
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
        if (!past.test(static_cast<int>(entry.preconditioned_lm))
            && none_of(entry.facts.begin(), entry.facts.end(),
                       [ancestor_state](const FactPair &fact_pair) {
            return ancestor_state[fact_pair.var].get_value() == fact_pair.value;
        })) {
            fut.set(static_cast<int>(entry.achiever_lm));
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

    if (!fut.test(id)) {
        return PAST;
    } else if ((int) id < past.size() && past.test(id)) {
        return PAST_AND_FUTURE;
    } else {
        return FUTURE;
    }
}
}
