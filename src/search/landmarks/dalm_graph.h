#ifndef LANDMARKS_DALM_GRAPH_H
#define LANDMARKS_DALM_GRAPH_H

#include "landmark_graph.h"

#include <map>
#include <set>
#include <vector>

namespace landmarks {

enum class OrderingType {
    STRONG,
    WEAK,
};

class DisjunctiveActionLandmarkNode {
    std::map<int, OrderingType> dependencies;

public:
    const std::set<int> actions;

    DisjunctiveActionLandmarkNode(std::set<int> actions);
    bool overlaps_with(DisjunctiveActionLandmarkNode &other) const;
    bool satisfied_by(int op_id) const;

    void add_strong_dependency(int node_id, size_t &num_orderings,
                               size_t &num_weak_orderings);
    void add_weak_dependency(int node_id, size_t &num_weak_orderings);
    OrderingType get_dependency(int id) const;
    const std::map<int, OrderingType> &get_dependencies() const;
    bool depends_on(int id) const;
};

struct precondition_achiever_triple {
    // TODO: Is a vector the correct choice for this? Depends on order.
    const std::vector<FactPair> facts;
    const size_t achiever_lm;
    const size_t preconditioned_lm;

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

    std::vector<size_t> initially_past_lms;
    std::vector<std::pair<FactPair, size_t>> goal_achiever_lms;
    std::vector<precondition_achiever_triple> precondition_achiever_lms;

    void dump_lm(int id) const;
public:
    explicit DisjunctiveActionLandmarkGraph() = default;

    size_t add_node(const std::set<int> &actions);
    void add_edge(size_t from, size_t to, bool strong);

    void mark_lm_initially_past(size_t id);
    const std::vector<size_t> &get_initially_past_lms() const {
        return initially_past_lms;
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
    const std::set<int> &get_actions(int id);
    const std::map<int, OrderingType> &get_dependencies(int id);
    OrderingType get_ordering_type(int from, int to);
    void dump() const;
    void dump_dot() const;

    std::vector<std::map<int, bool>> to_adj_list() const;
};
}

#endif
