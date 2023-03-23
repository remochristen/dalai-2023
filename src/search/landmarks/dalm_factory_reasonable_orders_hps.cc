#include "dalm_factory_reasonable_orders_hps.h"

//#include "landmark.h"

#include "util.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/markup.h"

using namespace std;
namespace landmarks {
DalmFactoryReasonableOrdersHPS::DalmFactoryReasonableOrdersHPS(const plugins::Options &opts)
    : LandmarkGraphFactory(),
      dalm_factory(opts.get<shared_ptr<LandmarkGraphFactory>>("dalm_factory")),
      ignore_disj_falms(opts.get<bool>("ignore_disj_falms")) {
}

shared_ptr<DisjunctiveActionLandmarkGraph> DalmFactoryReasonableOrdersHPS::compute_landmark_graph(
            const std::shared_ptr<AbstractTask> &task) {
    utils::g_log << "Building a landmark graph with reasonable orders." << endl;

    shared_ptr<DisjunctiveActionLandmarkGraph> dalm_graph = dalm_factory->compute_landmark_graph(task);

    TaskProxy task_proxy(*task);
    utils::g_log << "approx. reasonable orders" << endl;
    approximate_reasonable_orders(task_proxy, dalm_graph);

    utils::g_log << "Landmark graph of initial state contains "
                 << dalm_graph->get_number_of_landmarks() << " landmarks." << endl;
    utils::g_log << "Landmark graph of initial state contains "
                 << dalm_graph->get_number_of_orderings()
                 << " orderings of which "
                 << dalm_graph->get_number_of_strong_orderings()
                 << " are strong and "
                 << dalm_graph->get_number_of_weak_orderings()
                 << " are weak." << endl;

    return dalm_graph;
}

void DalmFactoryReasonableOrdersHPS::approximate_reasonable_orders(
    const TaskProxy &task_proxy, shared_ptr<DisjunctiveActionLandmarkGraph> dalm_graph) {
    /*
      Approximate reasonable and obedient reasonable orders according
      to Hoffmann et al. If flag "obedient_orders" is true, we
      calculate obedient reasonable orders, otherwise reasonable
      orders.

      If node_p is in goal, then any node2_p which interferes with
      node_p can be reasonably ordered before node_p. Otherwise, if
      node_p is greedy necessary predecessor of node2, and there is
      another predecessor "parent" of node2, then parent and all
      predecessors of parent can be ordered reasonably before node_p if
      they interfere with node_p.
    */
    State initial_state = task_proxy.get_initial_state();
    VariablesProxy variables = task_proxy.get_variables();

    for (pair<FactPair, size_t> goal_dalm : dalm_graph->get_goal_achiever_lms()) {
        for (size_t i = 0; i < dalm_graph->get_number_of_landmarks(); ++i) {
            if (i == goal_dalm.second) {
                continue;
            }
            vector<FactPair> goal_vec = {goal_dalm.first};
            if (interferes(task_proxy, dalm_graph->get_actions(i), goal_vec)) {
                dalm_graph->add_edge(i, goal_dalm.second, false);
            }
        }
    }

    for (const precondition_achiever_triple &triple : dalm_graph->get_precondition_achiever_lms()) {
        const vector<FactPair> &facts = triple.facts;

        if (ignore_disj_falms && facts.size() > 1) {
            continue;
        }

        size_t parent_id = triple.achiever_lm;
        size_t child_id = triple.preconditioned_lm;

        // TODO somewhat hacky - I think the problem is that different fact landmarks get put together if they don't have achievers
        if (dalm_graph->get_actions(parent_id).empty()) {
            continue;
        }

        list<size_t> open;
        unordered_set<size_t> interesting_dalms;
        open.push_back(child_id);
        while(!open.empty()) {
            size_t dalm_id = open.front();
            open.pop_front();
            if (dalm_id == parent_id) {
                continue;
            }
            for (pair<int, OrderingType> dependency : dalm_graph->get_dependencies(dalm_id)) {
                if(dependency.second == OrderingType::STRONG && interesting_dalms.count(dependency.first) == 0) {
                    open.push_back(dependency.first);
                    interesting_dalms.insert(dependency.first);
                }
            }
        }

        for (size_t lm_id : interesting_dalms) {
            if (interferes(task_proxy, dalm_graph->get_actions(lm_id), facts)) {
                // TODO figure out why it is still possible to reach both landmarks at the same time
                if (! dalm_graph->landmarks_overlap(lm_id, parent_id)) {
                    dalm_graph->add_edge(lm_id, parent_id, false);
                }
            }
        }

    }
}

bool DalmFactoryReasonableOrdersHPS::interferes(
    const TaskProxy &task_proxy, const set<int> &achievers_of_a,
    const vector<FactPair> &landmark_b) const {

    VariablesProxy variables = task_proxy.get_variables();
    unordered_map<int, int> shared_eff;
    bool init = true;
    //const vector<int> &op_or_axiom_ids = get_operators_including_eff(lm_fact_a);
    // Intersect operators that achieve a one by one
    for (int op_or_axiom_id : achievers_of_a) {
        // If no shared effect among previous operators, break
        if (!init && shared_eff.empty())
            break;
        // Else, insert effects of this operator into set "next_eff" if
        // it is an unconditional effect or a conditional effect that is sure to
        // happen. (Such "trivial" conditions can arise due to our translator,
        // e.g. in Schedule. There, the same effect is conditioned on a disjunction
        // of conditions of which one will always be true. We test for a simple kind
        // of these trivial conditions here.)
        EffectsProxy effects = get_operator_or_axiom(task_proxy, op_or_axiom_id).get_effects();
        set<FactPair> trivially_conditioned_effects;
        bool trivial_conditioned_effects_found = effect_always_happens(variables, effects,
                                                                       trivially_conditioned_effects);
        unordered_map<int, int> next_eff;
        for (EffectProxy effect : effects) {
            FactPair effect_fact = effect.get_fact().get_pair();
            if (effect.get_conditions().empty()) {
                next_eff.emplace(effect_fact.var, effect_fact.value);
            } else if (trivial_conditioned_effects_found &&
                       trivially_conditioned_effects.find(effect_fact)
                       != trivially_conditioned_effects.end())
                next_eff.emplace(effect_fact.var, effect_fact.value);
        }
        // Intersect effects of this operator with those of previous operators
        if (init)
            swap(shared_eff, next_eff);
        else {
            unordered_map<int, int> result;
            for (const auto &eff1 : shared_eff) {
                auto it2 = next_eff.find(eff1.first);
                if (it2 != next_eff.end() && it2->second == eff1.second)
                    result.insert(eff1);
            }
            swap(shared_eff, result);
        }
        init = false;
    }
    // Test whether for each fact in b there is a shared effect inconsistent with it
    std::vector<FactProxy> proxies;
    unordered_set<size_t> uncovered_indices;
    for (size_t i = 0; i < landmark_b.size(); ++i) {
        proxies.push_back(variables[landmark_b[i].var].get_fact(landmark_b[i].value));
        uncovered_indices.insert(i);
    }
    for (const pair<const int, int> &eff : shared_eff) {
        const FactProxy &effect_fact = variables[eff.first].get_fact(eff.second);
        for (size_t i : uncovered_indices) {
            if(effect_fact != proxies[i] && effect_fact.is_mutex(proxies[i])) {
                uncovered_indices.erase(i);
                break;
            }
        }
        if (uncovered_indices.size() == 0) {
            return true;
        }
    }
    return false;
}


bool DalmFactoryReasonableOrdersHPS::effect_always_happens(
    const VariablesProxy &variables, const EffectsProxy &effects,
    set<FactPair> &eff) const {
    /* Test whether the condition of a conditional effect is trivial, i.e. always true.
     We test for the simple case that the same effect proposition is triggered by
     a set of conditions of which one will always be true. This is e.g. the case in
     Schedule, where the effect
     (forall (?oldpaint - colour)
     (when (painted ?x ?oldpaint)
     (not (painted ?x ?oldpaint))))
     is translated by the translator to: if oldpaint == blue, then not painted ?x, and if
     oldpaint == red, then not painted ?x etc.
     If conditional effects are found that are always true, they are returned in "eff".
     */
    // Go through all effects of operator and collect:
    // - all variables that are set to some value in a conditional effect (effect_vars)
    // - variables that can be set to more than one value in a cond. effect (nogood_effect_vars)
    // - a mapping from cond. effect propositions to all the conditions that they appear with
    set<int> effect_vars;
    set<int> nogood_effect_vars;
    map<int, pair<int, vector<FactPair>>> effect_conditions_by_variable;
    for (EffectProxy effect : effects) {
        EffectConditionsProxy effect_conditions = effect.get_conditions();
        FactProxy effect_fact = effect.get_fact();
        int var_id = effect_fact.get_variable().get_id();
        int value = effect_fact.get_value();
        if (effect_conditions.empty() ||
            nogood_effect_vars.find(var_id) != nogood_effect_vars.end()) {
            // Var has no condition or can take on different values, skipping
            continue;
        }
        if (effect_vars.find(var_id) != effect_vars.end()) {
            // We have seen this effect var before
            assert(effect_conditions_by_variable.find(var_id) != effect_conditions_by_variable.end());
            int old_eff = effect_conditions_by_variable.find(var_id)->second.first;
            if (old_eff != value) {
                // Was different effect
                nogood_effect_vars.insert(var_id);
                continue;
            }
        } else {
            // We have not seen this effect var before
            effect_vars.insert(var_id);
        }
        if (effect_conditions_by_variable.find(var_id) != effect_conditions_by_variable.end()
            && effect_conditions_by_variable.find(var_id)->second.first == value) {
            // We have seen this effect before, adding conditions
            for (FactProxy effect_condition : effect_conditions) {
                vector<FactPair> &vec = effect_conditions_by_variable.find(var_id)->second.second;
                vec.push_back(effect_condition.get_pair());
            }
        } else {
            // We have not seen this effect before, making new effect entry
            vector<FactPair> &vec = effect_conditions_by_variable.emplace(
                var_id, make_pair(
                    value, vector<FactPair> ())).first->second.second;
            for (FactProxy effect_condition : effect_conditions) {
                vec.push_back(effect_condition.get_pair());
            }
        }
    }

    // For all those effect propositions whose variables do not take on different values...
    for (const auto &effect_conditions : effect_conditions_by_variable) {
        if (nogood_effect_vars.find(effect_conditions.first) != nogood_effect_vars.end()) {
            continue;
        }
        // ...go through all the conditions that the effect has, and map condition
        // variables to the set of values they take on (in unique_conds)
        map<int, set<int>> unique_conds;
        for (const FactPair &cond : effect_conditions.second.second) {
            if (unique_conds.find(cond.var) != unique_conds.end()) {
                unique_conds.find(cond.var)->second.insert(
                    cond.value);
            } else {
                set<int> &the_set = unique_conds.emplace(cond.var, set<int>()).first->second;
                the_set.insert(cond.value);
            }
        }
        // Check for each condition variable whether the number of values it takes on is
        // equal to the domain of that variable...
        bool is_always_reached = true;
        for (auto &unique_cond : unique_conds) {
            bool is_surely_reached_by_var = false;
            int num_values_for_cond = unique_cond.second.size();
            int num_values_of_variable = variables[unique_cond.first].get_domain_size();
            if (num_values_for_cond == num_values_of_variable) {
                is_surely_reached_by_var = true;
            }
            // ...or else if the condition variable is the same as the effect variable,
            // check whether the condition variable takes on all other values except the
            // effect value
            else if (unique_cond.first == effect_conditions.first &&
                     num_values_for_cond == num_values_of_variable - 1) {
                // Number of different values is correct, now ensure that the effect value
                // was the one missing
                unique_cond.second.insert(effect_conditions.second.first);
                num_values_for_cond = unique_cond.second.size();
                if (num_values_for_cond == num_values_of_variable) {
                    is_surely_reached_by_var = true;
                }
            }
            // If one of the condition variables does not fulfill the criteria, the effect
            // is not certain to happen
            if (!is_surely_reached_by_var)
                is_always_reached = false;
        }
        if (is_always_reached)
            eff.insert(FactPair(
                           effect_conditions.first, effect_conditions.second.first));
    }
    return eff.empty();
}

class DalmFactoryReasonableOrdersHPSFeature : public plugins::TypedFeature<LandmarkGraphFactory, DalmFactoryReasonableOrdersHPS> {
public:
    DalmFactoryReasonableOrdersHPSFeature() : TypedFeature("dalm_reasonable_orders_hps") {
        document_title("dalm HPS Orders");
        document_synopsis("TODO");

        add_option<shared_ptr<LandmarkGraphFactory>>("dalm_factory");
        add_option<bool>("ignore_disj_falms", "TODO", "false");

        // TODO: correct?
        document_language_support(
            "conditional_effects",
            "supported if subcomponent supports them");
    }
};

static plugins::FeaturePlugin<DalmFactoryReasonableOrdersHPSFeature> _plugin;
}
