#ifndef LANDMARKS_DALM_FACTORY_RHW_H
#define LANDMARKS_DALM_FACTORY_RHW_H

#include "dalm_graph_factory.h"

#include "../plugins/options.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
class Exploration;
class Landmark;

struct MixedLandmark {
    Landmark fact_landmark;
    int possible_achiever_dalm;
    int first_achiever_dalm;
    bool is_derived;
    int child_index;
    MixedLandmark(Landmark &&fact_landmark, size_t pa, size_t fa, bool derived, size_t child_index)
        : fact_landmark(fact_landmark), possible_achiever_dalm(pa), first_achiever_dalm(fa),
        is_derived(derived), child_index(child_index) {}
};

class DalmFactoryRhw : public LandmarkGraphFactory {
    const size_t max_preconditions;
    std::shared_ptr<DisjunctiveActionLandmarkGraph> dalm_graph;

    std::vector<std::vector<std::vector<int>>> operators_eff_lookup;
    std::vector<std::vector<int>> disjunction_classes;

    std::unordered_map<int, utils::HashSet<FactPair>> forward_orders;

    std::vector<MixedLandmark> landmarks;
    std::set<std::set<FactPair>> fact_lms;
    std::list<int> open_landmarks; //represents indices for landmarks vector

//    std::vector<Landmark *> fact_lms;
//    std::unordered_map<Landmark *, std::pair<int,int>> flm_to_dalm;

    // dtg_successors[var_id][val] contains all successor values of val in the
    // domain transition graph for the variable
    std::vector<std::vector<std::unordered_set<int>>> dtg_successors;

    void build_dtg_successors(const TaskProxy &task_proxy);
    void add_dtg_successor(int var_id, int pre, int post);
    void find_forward_orders(const VariablesProxy &variables,
                             const std::vector<std::vector<bool>> &reached, int landmark_index);
    void add_lm_forward_orders();

    void get_greedy_preconditions_for_lm(
        const TaskProxy &task_proxy, const Landmark &landmark,
        const OperatorProxy &op,
        std::unordered_map<int, int> &result) const;
    void compute_shared_preconditions(
        const TaskProxy &task_proxy,
        std::unordered_map<int, int> &shared_pre,
        std::set<int> &relevant_op_ids, const Landmark &landmark);
    void compute_disjunctive_preconditions(
        const TaskProxy &task_proxy,
        std::vector<std::set<FactPair>> &disjunctive_pre,
        std::set<int> &relevant_op_ids,
        const Landmark &landmark, const std::unordered_map<int, int> &shared_pre);

    std::shared_ptr<DisjunctiveActionLandmarkGraph> compute_landmark_graph(
        const std::shared_ptr<AbstractTask> &task) override;
    void approximate_lookahead_orders(const TaskProxy &task_proxy,
                                      const std::vector<std::vector<bool>> &reached, int landmark_index);
    bool domain_connectivity(const State &initial_state,
                             const FactPair &landmark,
                             const std::unordered_set<int> &exclude);

    void build_disjunction_classes(const TaskProxy &task_proxy);

    const std::vector<int> &get_operators_including_eff(const FactPair &eff) const {
        return operators_eff_lookup[eff.var][eff.value];
    }
    void generate_operators_lookups(const TaskProxy &task_proxy);

    int add_landmark(const std::set<FactPair> &facts, const State &initial_state, int child_index);
    void add_first_achiever_dalm(int index, const std::set<int> &first_achievers,
                                   const State &initial_state);
    void add_gn_edge(int parent_index, int child_index);
    void add_nat_edge(int parent_index, int child_index);
public:
    explicit DalmFactoryRhw(const plugins::Options &opts);
};
}

#endif
