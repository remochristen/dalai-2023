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
    bool progress_greedy_necessary_orderings,
    bool progress_reasonable_orderings)
    : lm_graph(graph),
      progress_greedy_necessary_orderings(progress_greedy_necessary_orderings),
      progress_reasonable_orderings(progress_reasonable_orderings),
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
    past_lms[initial_state].reset();
    future_lms[initial_state].set();
    for (size_t id : lm_graph.get_initially_past_lms()) {
        past_lms[initial_state].set(id);
        future_lms[initial_state].reset(id);
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
    assert(past.size() == num_landmarks);
    assert(parent_past.size() == num_landmarks);
    assert(fut.size() == num_landmarks);
    assert(parent_fut.size() == num_landmarks);

    progress_basic(parent_past, parent_fut, past, fut, op_id.get_index());

    for (int id = 0; id < num_landmarks; ++id) {
        if (progress_greedy_necessary_orderings) {
            progress_greedy_necessary(id, ancestor_state, past, fut);
        }
        if (progress_reasonable_orderings) {
            progress_reasonable(id, past, fut);
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

void DisjunctiveActionLandmarkStatusManager::progress_greedy_necessary(
    int /*id*/, const State &/*ancestor_state*/, const BitsetView &/*past*/,
    BitsetView &/*fut*/) {
    /* TODO: decide how to deal with greedy-necessary orderings.

    if (past.test(id)) {
        return;
    }

    for (auto &parent : lm_graph.get_node(id)->parents) {
        if (parent.second != EdgeType::GREEDY_NECESSARY
            || fut.test(parent.first->get_id())) {
            continue;
        }
        if (!parent.first->get_landmark().is_true_in_state(ancestor_state)) {
            ++gn_progression_counter;
            fut.set(parent.first->get_id());
        }
    }
    */
}

void DisjunctiveActionLandmarkStatusManager::progress_reasonable(
    int /*id*/, const BitsetView &/*past*/, BitsetView &/*fut*/) {
    /* TODO: decide how to deal with reasonable orderings.

    if (past.test(id)) {
        return;
    }

    for (auto &child : lm_graph.get_node(id)->children) {
        if (child.second == EdgeType::REASONABLE) {
            ++reasonable_progression_counter;
            fut.set(child.first->get_id());
        }
    }
    */
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
