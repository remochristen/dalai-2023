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

class DalmFactoryRhw : public LandmarkGraphFactory {
    std::shared_ptr<DisjunctiveActionLandmarkGraph> dalm_graph;

    std::vector<std::vector<std::vector<int>>> operators_eff_lookup;
    std::list<Landmark *> open_landmarks;
    std::vector<std::vector<int>> disjunction_classes;

    std::unordered_map<Landmark *, utils::HashSet<FactPair>> forward_orders;

    std::vector<Landmark *> fact_lms;
    std::unordered_map<Landmark *, std::pair<int,int>> flm_to_dalm;

    // dtg_successors[var_id][val] contains all successor values of val in the
    // domain transition graph for the variable
    std::vector<std::vector<std::unordered_set<int>>> dtg_successors;

    void build_dtg_successors(const TaskProxy &task_proxy);
    void add_dtg_successor(int var_id, int pre, int post);
    void find_forward_orders(const VariablesProxy &variables,
                             const std::vector<std::vector<bool>> &reached,
                             Landmark &landmark);
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
        const Landmark &landmark);

    std::shared_ptr<DisjunctiveActionLandmarkGraph> compute_landmark_graph(
        const std::shared_ptr<AbstractTask> &task) override;
    void approximate_lookahead_orders(const TaskProxy &task_proxy,
                                      const std::vector<std::vector<bool>> &reached,
                                      Landmark &lmp);
    bool domain_connectivity(const State &initial_state,
                             const FactPair &landmark,
                             const std::unordered_set<int> &exclude);

    void build_disjunction_classes(const TaskProxy &task_proxy);

    const std::vector<int> &get_operators_including_eff(const FactPair &eff) const {
        return operators_eff_lookup[eff.var][eff.value];
    }
    void generate_operators_lookups(const TaskProxy &task_proxy);

    Landmark *create_fact_and_dalm_landmark(const std::set<FactPair> &facts, const State &initial_state);
    size_t add_first_achiever_dalm(Landmark *fact_lm, const std::set<int> &first_achievers,
                                   const State &initial_state);
    void add_gn_edge(Landmark *parent, Landmark *child);
public:
    explicit DalmFactoryRhw(const plugins::Options &opts);
};
}

#endif
