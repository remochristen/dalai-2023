#include "dalm_factory_rhw.h"

#include "exploration.h"
#include "landmark.h"
#include "util.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <cassert>
#include <limits>

using namespace std;
using utils::ExitCode;

namespace landmarks {
DalmFactoryRhw::DalmFactoryRhw(const plugins::Options &)
    : LandmarkGraphFactory(), dalm_graph(make_shared<DisjunctiveActionLandmarkGraph>()) {
}

void DalmFactoryRhw::build_dtg_successors(const TaskProxy &task_proxy) {
    // resize data structure
    VariablesProxy variables = task_proxy.get_variables();
    dtg_successors.resize(variables.size());
    for (VariableProxy var : variables)
        dtg_successors[var.get_id()].resize(var.get_domain_size());

    for (OperatorProxy op : task_proxy.get_operators()) {
        // build map for precondition
        unordered_map<int, int> precondition_map;
        for (FactProxy precondition : op.get_preconditions())
            precondition_map[precondition.get_variable().get_id()] = precondition.get_value();

        for (EffectProxy effect : op.get_effects()) {
            // build map for effect condition
            unordered_map<int, int> eff_condition;
            for (FactProxy effect_condition : effect.get_conditions())
                eff_condition[effect_condition.get_variable().get_id()] = effect_condition.get_value();

            // whenever the operator can change the value of a variable from pre to
            // post, we insert post into dtg_successors[var_id][pre]
            FactProxy effect_fact = effect.get_fact();
            int var_id = effect_fact.get_variable().get_id();
            int post = effect_fact.get_value();
            if (precondition_map.count(var_id)) {
                int pre = precondition_map[var_id];
                if (eff_condition.count(var_id) && eff_condition[var_id] != pre)
                    continue; // confliction pre- and effect condition
                add_dtg_successor(var_id, pre, post);
            } else {
                if (eff_condition.count(var_id)) {
                    add_dtg_successor(var_id, eff_condition[var_id], post);
                } else {
                    int dom_size = effect_fact.get_variable().get_domain_size();
                    for (int pre = 0; pre < dom_size; ++pre)
                        add_dtg_successor(var_id, pre, post);
                }
            }
        }
    }
}

void DalmFactoryRhw::add_dtg_successor(int var_id, int pre, int post) {
    if (pre != post)
        dtg_successors[var_id][pre].insert(post);
}

void DalmFactoryRhw::get_greedy_preconditions_for_lm(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const OperatorProxy &op, unordered_map<int, int> &result) const {
    // Computes a subset of the actual preconditions of o for achieving lmp - takes into account
    // operator preconditions, but only reports those effect conditions that are true for ALL
    // effects achieving the LM.

    vector<bool> has_precondition_on_var(task_proxy.get_variables().size(), false);
    for (FactProxy precondition : op.get_preconditions()) {
        result.emplace(precondition.get_variable().get_id(), precondition.get_value());
        has_precondition_on_var[precondition.get_variable().get_id()] = true;
    }

    // If there is an effect but no precondition on a variable v with domain
    // size 2 and initially the variable has the other value than required by
    // the landmark then at the first time the landmark is reached the
    // variable must still have the initial value.
    State initial_state = task_proxy.get_initial_state();
    EffectsProxy effects = op.get_effects();
    for (EffectProxy effect : effects) {
        FactProxy effect_fact = effect.get_fact();
        int var_id = effect_fact.get_variable().get_id();
        if (!has_precondition_on_var[var_id] && effect_fact.get_variable().get_domain_size() == 2) {
            for (const FactPair &lm_fact : landmark.facts) {
                if (lm_fact.var == var_id &&
                    initial_state[var_id].get_value() != lm_fact.value) {
                    result.emplace(var_id, initial_state[var_id].get_value());
                    break;
                }
            }
        }
    }

    // Check for lmp in conditional effects
    set<int> lm_props_achievable;
    for (EffectProxy effect : effects) {
        FactProxy effect_fact = effect.get_fact();
        for (size_t j = 0; j < landmark.facts.size(); ++j)
            if (landmark.facts[j] == effect_fact.get_pair())
                lm_props_achievable.insert(j);
    }
    // Intersect effect conditions of all effects that can achieve lmp
    unordered_map<int, int> intersection;
    bool init = true;
    for (int lm_prop : lm_props_achievable) {
        for (EffectProxy effect : effects) {
            FactProxy effect_fact = effect.get_fact();
            if (!init && intersection.empty())
                break;
            unordered_map<int, int> current_cond;
            if (landmark.facts[lm_prop] == effect_fact.get_pair()) {
                EffectConditionsProxy effect_conditions = effect.get_conditions();
                if (effect_conditions.empty()) {
                    intersection.clear();
                    break;
                } else {
                    for (FactProxy effect_condition : effect_conditions)
                        current_cond.emplace(effect_condition.get_variable().get_id(),
                                             effect_condition.get_value());
                }
            }
            if (init) {
                init = false;
                intersection = current_cond;
            } else
                intersection = _intersect(intersection, current_cond);
        }
    }
    result.insert(intersection.begin(), intersection.end());
}

void DalmFactoryRhw::compute_shared_preconditions(
    const TaskProxy &task_proxy, unordered_map<int, int> &shared_pre,
    set<int> &relevant_op_ids, const Landmark &landmark) {
    /*
      Compute the shared preconditions of all operators that can potentially
      achieve landmark bp, given the reachability in the relaxed planning graph.
    */
    bool init = true;
    for (int op_id : relevant_op_ids) {
        OperatorProxy op = get_operator_or_axiom(task_proxy, op_id);
        if (!init && shared_pre.empty())
            break;

        unordered_map<int, int> next_pre;
        get_greedy_preconditions_for_lm(task_proxy, landmark,
                                        op, next_pre);
        if (init) {
            init = false;
            shared_pre = next_pre;
        } else
            shared_pre = _intersect(shared_pre, next_pre);
    }
}

static string get_predicate_for_fact(const VariablesProxy &variables,
                                     int var_no, int value) {
    const string fact_name = variables[var_no].get_fact(value).get_name();
    if (fact_name == "<none of those>")
        return "";
    int predicate_pos = 0;
    if (fact_name.substr(0, 5) == "Atom ") {
        predicate_pos = 5;
    } else if (fact_name.substr(0, 12) == "NegatedAtom ") {
        predicate_pos = 12;
    }
    size_t paren_pos = fact_name.find('(', predicate_pos);
    if (predicate_pos == 0 || paren_pos == string::npos) {
        cerr << "error: cannot extract predicate from fact: "
             << fact_name << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    return string(fact_name.begin() + predicate_pos, fact_name.begin() + paren_pos);
}

void DalmFactoryRhw::build_disjunction_classes(const TaskProxy &task_proxy) {
    /* The RHW landmark generation method only allows disjunctive
       landmarks where all atoms stem from the same PDDL predicate.
       This functionality is implemented via this method.

       The approach we use is to map each fact (var/value pair) to an
       equivalence class (representing all facts with the same
       predicate). The special class "-1" means "cannot be part of any
       disjunctive landmark". This is used for facts that do not
       belong to any predicate.

       Similar methods for restricting disjunctive landmarks could be
       implemented by just changing this function, as long as the
       restriction could also be implemented as an equivalence class.
       For example, we might simply use the finite-domain variable
       number as the equivalence class, which would be a cleaner
       method than what we currently use since it doesn't care about
       where the finite-domain representation comes from. (But of
       course making such a change would require a performance
       evaluation.)
    */

    typedef map<string, int> PredicateIndex;
    PredicateIndex predicate_to_index;

    VariablesProxy variables = task_proxy.get_variables();
    disjunction_classes.resize(variables.size());
    for (VariableProxy var : variables) {
        int num_values = var.get_domain_size();
        disjunction_classes[var.get_id()].reserve(num_values);
        for (int value = 0; value < num_values; ++value) {
            string predicate = get_predicate_for_fact(variables, var.get_id(), value);
            int disj_class;
            if (predicate.empty()) {
                disj_class = -1;
            } else {
                // Insert predicate into unordered_map or extract value that
                // is already there.
                pair<string, int> entry(predicate, predicate_to_index.size());
                disj_class = predicate_to_index.insert(entry).first->second;
            }
            disjunction_classes[var.get_id()].push_back(disj_class);
        }
    }
}

void DalmFactoryRhw::compute_disjunctive_preconditions(
    const TaskProxy &task_proxy, vector<set<FactPair>> &disjunctive_pre,
    set<int> &relevant_op_ids, const Landmark &landmark) {
    /*
      Compute disjunctive preconditions from all operators than can potentially
      achieve landmark bp, given the reachability in the relaxed planning graph.
      A disj. precondition is a set of facts which contains one precondition
      fact from each of the operators, which we additionally restrict so that
      each fact in the set stems from the same PDDL predicate.
    */

    unordered_map<int, vector<FactPair>> preconditions;   // maps from
    // pddl_proposition_indeces to props
    unordered_map<int, set<int>> used_operators;  // tells for each
    // proposition which operators use it
    for (int op_id  : relevant_op_ids) {
        OperatorProxy op = get_operator_or_axiom(task_proxy, op_id);
        unordered_map<int, int> next_pre;
        get_greedy_preconditions_for_lm(task_proxy, landmark, op, next_pre);
        for (const auto &pre : next_pre) {
            int disj_class = disjunction_classes[pre.first][pre.second];
            if (disj_class == -1) {
                // This fact may not participate in any disjunctive LMs
                // since it has no associated predicate.
                continue;
            }

            // Only deal with propositions that are not shared preconditions
            // (those have been found already and are simple landmarks).
            const FactPair pre_fact(pre.first, pre.second);
            //if (!dalm_graph->contains_simple_landmark(pre_fact)) { TODO: re-enable this test
                preconditions[disj_class].push_back(pre_fact);
                used_operators[disj_class].insert(op_id);
            //}
        }
    }
    for (const auto &pre : preconditions) {
        if (used_operators[pre.first].size() == relevant_op_ids.size()) {
            set<FactPair> pre_set;  // the set gets rid of duplicate predicates
            pre_set.insert(pre.second.begin(), pre.second.end());
            if (pre_set.size() > 1) { // otherwise this LM is not actually a disjunctive LM
                disjunctive_pre.push_back(pre_set);
            }
        }
    }
}

Landmark *DalmFactoryRhw::create_fact_and_dalm_landmark(const set<FactPair> &facts, const State &initial_state) {
    vector<FactPair> fact_lm_vec;
    set<int> dalm_ops = {};
    for (const FactPair fact : facts) {
        fact_lm_vec.push_back(fact);
        const vector<int> &achieving_ops = get_operators_including_eff(fact);
        dalm_ops.insert(achieving_ops.begin(), achieving_ops.end());
    }
    Landmark *lm = new Landmark(fact_lm_vec, (fact_lm_vec.size() > 1), false);
    fact_lms.push_back(lm);
    size_t dalm_id = dalm_graph->add_node(dalm_ops,
                                          lm->is_true_in_state(initial_state));
    flm_to_dalm.insert(make_pair(lm, make_pair(dalm_id, -1)));
    return lm;
}

size_t DalmFactoryRhw::add_first_achiever_dalm(Landmark *fact_lm, const set<int> &first_achievers,
                                               const State &initial_state) {
    size_t ret = dalm_graph->add_node(first_achievers,
                                      fact_lm->is_true_in_state(initial_state));
    flm_to_dalm[fact_lm].second = ret;
    return ret;
}

void DalmFactoryRhw::add_gn_edge(Landmark *parent, Landmark *child) {
    dalm_graph->add_edge(flm_to_dalm[parent].first, flm_to_dalm[child].first, true);
    dalm_graph->add_edge(flm_to_dalm[parent].first, flm_to_dalm[child].second, true );
    dalm_graph->mark_lm_precondition_achiever(parent->facts,
                                              flm_to_dalm[parent].first,
                                              flm_to_dalm[child].second);
}

std::shared_ptr<DisjunctiveActionLandmarkGraph> DalmFactoryRhw::compute_landmark_graph(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    Exploration exploration(task_proxy, utils::g_log);

    utils::g_log << "Generating landmarks with dalm rhw" << endl;

    build_dtg_successors(task_proxy);
    build_disjunction_classes(task_proxy);
    generate_operators_lookups(task_proxy);

    State initial_state = task_proxy.get_initial_state();

    for (FactProxy goal : task_proxy.get_goals()) {
        set<FactPair> lm_set = {goal.get_pair()};
        Landmark *landmark = create_fact_and_dalm_landmark(lm_set, initial_state);
        dalm_graph->mark_lm_goal_achiever(goal.get_pair(), flm_to_dalm[landmark].first);
        open_landmarks.push_back(landmark);
    }

    while (!open_landmarks.empty()) {
        Landmark *landmark = open_landmarks.front();
        open_landmarks.pop_front();
        assert(forward_orders[landmark].empty());

        if (!landmark->is_true_in_state(initial_state)) {
            /*
              Backchain from *landmark* and compute greedy necessary
              predecessors.
              Firstly, collect which propositions can be reached without
              achieving the landmark.
            */
            vector<int> excluded_op_ids;
            vector<vector<bool>> reached =
                    exploration.compute_relaxed_reachability(landmark->facts, excluded_op_ids);

            set<int> first_achievers;
            for (const FactPair &lm_fact : landmark->facts) {
                const vector<int> &op_ids = get_operators_including_eff(lm_fact);
                for (int op_or_axiom_id : op_ids) {
                    if (possibly_reaches_lm(get_operator_or_axiom(task_proxy, op_or_axiom_id), reached, *landmark)) {
                        first_achievers.insert(op_or_axiom_id);
                    }
                }
            }
            add_first_achiever_dalm(landmark, first_achievers, initial_state);
            /*
              Use this information to determine all operators that can
              possibly achieve *landmark* for the first time, and collect
              any precondition propositions that all such operators share
              (if there are any).
            */
            unordered_map<int, int> shared_pre;
            compute_shared_preconditions(task_proxy, shared_pre, first_achievers, *landmark);
            /*
              All such shared preconditions are landmarks, and greedy
              necessary predecessors of *landmark*.
            */
            for (const auto &pre : shared_pre) {
                set<FactPair> pre_set({FactPair(pre.first, pre.second)});
                Landmark *parent = create_fact_and_dalm_landmark(pre_set, initial_state);
                add_gn_edge(parent, landmark);
                open_landmarks.push_back(parent);
            }
            // Extract additional orders from the relaxed planning graph and DTG.
            approximate_lookahead_orders(task_proxy, reached, *landmark);

            // Process achieving operators again to find disjunctive LMs
            vector<set<FactPair>> disjunctive_pre;
            compute_disjunctive_preconditions(
                    task_proxy, disjunctive_pre, first_achievers, *landmark);
            for (const auto &preconditions : disjunctive_pre)
                // We don't want disjunctive LMs to get too big.
                if (preconditions.size() < 5) { // TODO make this an adjustable option
                    Landmark *parent = create_fact_and_dalm_landmark(preconditions, initial_state);
                    add_gn_edge(parent, landmark);
                    open_landmarks.push_back(parent);
                }
        }
    }
    add_lm_forward_orders();
    for (Landmark *lm : fact_lms) {
        delete lm;
    }
    dalm_graph->dump_dot();
    return dalm_graph;
}

void DalmFactoryRhw::approximate_lookahead_orders(
    const TaskProxy &task_proxy, const vector<vector<bool>> &reached, Landmark &landmark) {
    /*
      Find all var-val pairs that can only be reached after the landmark
      (according to relaxed plan graph as captured in reached).
      The result is saved in the node member variable forward_orders, and
      will be used later, when the phase of finding LMs has ended (because
      at the moment we don't know which of these var-val pairs will be LMs).
    */
    VariablesProxy variables = task_proxy.get_variables();
    find_forward_orders(variables, reached, landmark);

    /*
      Use domain transition graphs to find further orders. Only possible
      if landmark is a simple landmark.
    */
    if (landmark.disjunctive)
        return;
    const FactPair &lm_fact = landmark.facts[0];

    /*
      Collect in *unreached* all values of the LM variable that cannot be
      reached before the LM value (in the relaxed plan graph).
    */
    int domain_size = variables[lm_fact.var].get_domain_size();
    unordered_set<int> unreached(domain_size);
    for (int value = 0; value < domain_size; ++value)
        if (!reached[lm_fact.var][value] && lm_fact.value != value)
            unreached.insert(value);
    /*
      The set *exclude* will contain all those values of the LM variable that
      cannot be reached before the LM value (as in *unreached*) PLUS
      one value that CAN be reached.
    */
    State initial_state = task_proxy.get_initial_state();
    for (int value = 0; value < domain_size; ++value)
        if (unreached.find(value) == unreached.end() && lm_fact.value != value) {
            unordered_set<int> exclude(domain_size);
            exclude = unreached;
            exclude.insert(value);
            /*
              If that value is crucial for achieving the LM from the
              initial state, we have found a new landmark.
            */
            if (!domain_connectivity(initial_state, lm_fact, exclude)) {
                //    found_simple_lm_and_order(FactPair(lm_fact.var, value), landmark, EdgeType::NATURAL);
                set<FactPair> new_lm_fact = {FactPair(lm_fact.var, value)};
                Landmark *ptr = create_fact_and_dalm_landmark(new_lm_fact, initial_state);
                size_t parent_pa = flm_to_dalm[ptr].first;
                size_t child_pa = flm_to_dalm[&landmark].first;
                size_t child_fa = flm_to_dalm[&landmark].second;
                dalm_graph->add_edge(parent_pa, child_pa, true);
                dalm_graph->add_edge(parent_pa, child_fa, true);

                // TODO: this was not done in original rhw - why not?
                open_landmarks.push_back(ptr);
            }
        }
}

bool DalmFactoryRhw::domain_connectivity(const State &initial_state,
                                                 const FactPair &landmark,
                                                 const unordered_set<int> &exclude) {
    /*
      Tests whether in the domain transition graph of the LM variable, there is
      a path from the initial state value to the LM value, without passing through
      any value in "exclude". If not, that means that one of the values in "exclude"
      is crucial for achieving the landmark (i.e. is on every path to the LM).
    */
    int var = landmark.var;
    assert(landmark.value != initial_state[var].get_value()); // no initial state landmarks
    // The value that we want to achieve must not be excluded:
    assert(exclude.find(landmark.value) == exclude.end());
    // If the value in the initial state is excluded, we won't achieve our goal value:
    if (exclude.find(initial_state[var].get_value()) != exclude.end())
        return false;
    list<int> open;
    unordered_set<int> closed(initial_state[var].get_variable().get_domain_size());
    closed = exclude;
    open.push_back(initial_state[var].get_value());
    closed.insert(initial_state[var].get_value());
    const vector<unordered_set<int>> &successors = dtg_successors[var];
    while (closed.find(landmark.value) == closed.end()) {
        if (open.empty()) // landmark not in closed and nothing more to insert
            return false;
        const int c = open.front();
        open.pop_front();
        for (int val : successors[c]) {
            if (closed.find(val) == closed.end()) {
                open.push_back(val);
                closed.insert(val);
            }
        }
    }
    return true;
}

void DalmFactoryRhw::find_forward_orders(const VariablesProxy &variables,
                                                 const vector<vector<bool>> &reached,
                                                 Landmark &landmark) {
    /*
      landmark is ordered before any var-val pair that cannot be reached before
      landmark according to relaxed planning graph (as captured in reached).
      These orders are saved in the node member variable "forward_orders".
    */
    for (VariableProxy var: variables)
        for (int value = 0; value < var.get_domain_size(); ++value) {
            if (reached[var.get_id()][value])
                continue;
            const FactPair fact(var.get_id(), value);

            bool insert = true;
            for (const FactPair &lm_fact: landmark.facts) {
                if (fact != lm_fact) {
                    // Make sure there is no operator that reaches both lm and (var, value) at the same time
                    bool intersection_empty = true;
                    const vector<int> &reach_fact =
                            get_operators_including_eff(fact);
                    const vector<int> &reach_lm =
                            get_operators_including_eff(lm_fact);
                    for (size_t j = 0; j < reach_fact.size() && intersection_empty; ++j)
                        for (size_t k = 0; k < reach_lm.size()
                                           && intersection_empty; ++k)
                            if (reach_fact[j] == reach_lm[k])
                                intersection_empty = false;

                    if (!intersection_empty) {
                        insert = false;
                        break;
                    }
                } else {
                    insert = false;
                    break;
                }
            }
            if (insert)
                forward_orders[&landmark].insert(fact);
        }
}

void DalmFactoryRhw::add_lm_forward_orders() {
    for (Landmark *lm : fact_lms) {
        for (const auto &fact : forward_orders[lm]) {
            for (Landmark *lm2 : fact_lms) {
                if (lm2->facts.size() == 1 && lm2->facts[0] == fact) {
                    size_t parent_pa = flm_to_dalm[lm].first;
                    size_t child_pa = flm_to_dalm[lm2].first;
                    size_t child_fa = flm_to_dalm[lm2].second;
                    dalm_graph->add_edge(parent_pa, child_pa, true);
                    dalm_graph->add_edge(parent_pa, child_fa, true);
                }
            }
        }
    }
}

void DalmFactoryRhw::generate_operators_lookups(const TaskProxy &task_proxy) {
    /* Build datastructures for efficient landmark computation. Map propositions
    to the operators that achieve them or have them as preconditions */

    VariablesProxy variables = task_proxy.get_variables();
    operators_eff_lookup.resize(variables.size());
    for (VariableProxy var : variables) {
        operators_eff_lookup[var.get_id()].resize(var.get_domain_size());
    }
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators) {
        const EffectsProxy effects = op.get_effects();
        for (EffectProxy effect : effects) {
            const FactProxy effect_fact = effect.get_fact();
            operators_eff_lookup[effect_fact.get_variable().get_id()][effect_fact.get_value()].push_back(
                    get_operator_or_axiom_id(op));
        }
    }
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        const EffectsProxy effects = axiom.get_effects();
        for (EffectProxy effect : effects) {
            const FactProxy effect_fact = effect.get_fact();
            operators_eff_lookup[effect_fact.get_variable().get_id()][effect_fact.get_value()].push_back(
                    get_operator_or_axiom_id(axiom));
        }
    }
}


class DalmFactoryRhwFeature : public plugins::TypedFeature<LandmarkGraphFactory, DalmFactoryRhw> {
public:
    DalmFactoryRhwFeature() : TypedFeature("dalm_rhw") {
        document_title("dalm RHW Landmarks");
        document_synopsis(
            "TODO");

        document_language_support(
            "conditional_effects",
            "supported");
    }
};

static plugins::FeaturePlugin<DalmFactoryRhwFeature> _plugin;
}
