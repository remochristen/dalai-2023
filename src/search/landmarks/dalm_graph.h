#ifndef LANDMARKS_DALM_GRAPH_H
#define LANDMARKS_DALM_GRAPH_H

#include "landmark_graph.h"

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace landmarks {

enum class OrderingType {
    STRONG,
    WEAK,
};

class DisjunctiveActionLandmarkNode {
    std::map<int, OrderingType> dependencies;

public:
    std::set<int> actions;

    DisjunctiveActionLandmarkNode(std::set<int> actions);
    bool overlaps_with(DisjunctiveActionLandmarkNode &other) const;
    bool satisfied_by(int op_id) const;

    void add_strong_dependency(int node_id, size_t &num_orderings,
                               size_t &num_weak_orderings);
    void add_weak_dependency(int node_id, size_t &num_weak_orderings);
    OrderingType get_dependency(int id) const;
    const std::map<int, OrderingType> &get_dependencies() const;
    bool depends_on(int id) const;
    void swap_ids(const std::unordered_map<size_t, size_t> &swap_mapping);
};

struct precondition_achiever_triple {
    // TODO: Is a vector the correct choice for this? Depends on order.
    const std::vector<FactPair> facts;
    size_t achiever_lm;
    size_t preconditioned_lm;

    precondition_achiever_triple(
        const std::vector<FactPair> &facts,
        size_t achiever_lm, size_t preconditioned_lm)
        : facts(facts),
          achiever_lm(achiever_lm),
          preconditioned_lm(preconditioned_lm) {
    };
};

class DisjunctiveActionLandmarkGraph {
    std::map<std::set<int>, size_t> ids;
    std::vector<DisjunctiveActionLandmarkNode> lms;
    size_t num_strong_orderings = 0;
    size_t num_weak_orderings = 0;
    size_t last_relevant_past_dalm;
    bool uaa_landmarks;

    std::vector<bool> lm_true_in_initial;
    std::vector<bool> lm_initially_fut;
    std::vector<std::pair<FactPair, size_t>> goal_achiever_lms;
    std::vector<precondition_achiever_triple> precondition_achiever_lms;
    std::vector<int> op_to_uaa_lm;

    void dump_lm(int id) const;
public:
    explicit DisjunctiveActionLandmarkGraph(bool uaa_landmarks, const TaskProxy task_proxy);

    size_t add_node(const std::set<int> &actions, bool true_in_initial, int op_id = -1);
    void add_edge(size_t from, size_t to, bool strong);

    bool is_true_in_initial(int id) const {
        return lm_true_in_initial[id];
    }
    bool is_initially_fut(int id) const {
        return lm_initially_fut[id];
    }
    void mark_lm_goal_achiever(const FactPair &fact_pair, size_t lm);
    void mark_lm_precondition_achiever (
        const std::vector<FactPair> &fact_pairs, size_t achiever_lm,
        size_t preconditioned_lm);

    const std::vector<std::pair<FactPair, size_t>> &get_goal_achiever_lms() const {
        return goal_achiever_lms;
    }
    const std::vector<precondition_achiever_triple> &get_precondition_achiever_lms() const {
        return precondition_achiever_lms;
    }

    int get_id(const std::set<int> &actions);

    bool landmarks_overlap(size_t lm1, size_t lm2);

    size_t get_number_of_landmarks() const;
    size_t get_number_of_orderings() const {
        return num_strong_orderings + num_weak_orderings;
    }
    size_t get_number_of_strong_orderings() const {
        return num_strong_orderings;
    }
    size_t get_number_of_weak_orderings() const {
        return num_weak_orderings;
    }
    const std::set<int> &get_actions(int id) const;
    const std::map<int, OrderingType> &get_dependencies(int id) const;
    OrderingType get_ordering_type(int from, int to);
    void dump() const;
    void dump_dot() const;

    std::vector<std::map<int, bool>> to_adj_list() const;

    void order_dalms_with_relevant_past_first();
    size_t get_last_relevant_past_id() {
        return last_relevant_past_dalm;
    }
    bool has_uaa_landmarks() const {
        return uaa_landmarks;
    }
    int get_uaa_landmark_for_operator(int op_id) const;
};
}

#endif
