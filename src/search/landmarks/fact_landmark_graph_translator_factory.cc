#include "fact_landmark_graph_translator_factory.h"

#include "landmark_factory.h"

#include "../plugins/plugin.h"

#include <algorithm>

using namespace std;

namespace landmarks {
static void remove_derived_landmarks(LandmarkGraph &lm_graph) {
    // THIS IS SUPER HACKY!!!
    int removed_lms = 0;
    int num_landmarks = lm_graph.get_num_landmarks();
    for (int id = 0; id < num_landmarks; ++id) {
        if (lm_graph.get_node(id - removed_lms)->get_landmark().is_derived) {
            lm_graph.remove_node(lm_graph.get_node(id - removed_lms));
            ++removed_lms;
        }
    }
    utils::g_log << "Removed " << removed_lms << " derived variables." << endl;
    lm_graph.set_landmark_ids();
}

FactLandmarkGraphTranslatorFactory::FactLandmarkGraphTranslatorFactory(
    const plugins::Options &opts)
    : lm(opts.get<shared_ptr<LandmarkFactory>>("lm")),
      uaa_landmarks(opts.get<bool>("uaa_landmarks")),
      max_uaa_dalm_size(opts.get<int>("max_uaa_dalm_size")) {
}

void FactLandmarkGraphTranslatorFactory::add_nodes(
    dalm_graph &graph, const LandmarkGraph &lm_graph, const State &init_state) {
    for (const unique_ptr<LandmarkNode> &node : lm_graph.get_nodes()) {
        const Landmark &fact_lm = node->get_landmark();
        if (fact_lm.is_derived) {
            continue;
        }

        bool is_initially_true = fact_lm.is_true_in_state(init_state);
        if (!is_initially_true) {
            graph->add_node(fact_lm.first_achievers, is_initially_true);
        }
        if (fact_lm.is_true_in_goal) {
            size_t lm_id = graph->add_node(fact_lm.possible_achievers, is_initially_true);
            assert(fact_lm.facts.size() == 1);
            graph->mark_lm_goal_achiever(fact_lm.facts[0], lm_id);
        }
        for (auto &parent : node->parents) {
            if (parent.second == EdgeType::REASONABLE) {
                graph->add_node(fact_lm.possible_achievers, is_initially_true);
                break;
            }
        }
        for (auto &child: node->children) {
            if (child.second == EdgeType::GREEDY_NECESSARY) {
                Landmark &child_fact_lm = child.first->get_landmark();
                if (child_fact_lm.is_derived) {
                    continue;
                }
                size_t achiever_id =
                    graph->add_node(fact_lm.possible_achievers, is_initially_true);
                size_t preconditioned_id =
                    graph->add_node(child_fact_lm.first_achievers, child_fact_lm.is_true_in_state(init_state));
                graph->mark_lm_precondition_achiever(
                    fact_lm.facts, achiever_id, preconditioned_id);
            }
        }
    }
}

void FactLandmarkGraphTranslatorFactory::add_edges(
    dalm_graph &graph, const LandmarkGraph &lm_graph, const State &init_state) {
    for (auto &node : lm_graph.get_nodes()) {
        Landmark &fact_lm = node->get_landmark();
        int from_id = -1;
        if (fact_lm.is_true_in_state(init_state)) {
            from_id = graph->get_id(fact_lm.possible_achievers);
        } else {
            from_id = graph->get_id(fact_lm.first_achievers);
            assert(from_id >= 0);
        }
        /*
         * We do not add landmarks if they are true in the initial state
         * and don't have orderings that can make them future again.
         */
        if (from_id == -1) {
            continue;
        }
        for (auto &child : node->children) {
            Landmark &child_fact_lm = child.first->get_landmark();
            int to_id = graph->get_id(child.second >= EdgeType::NATURAL
                                      ? child_fact_lm.first_achievers
                                      : child_fact_lm.possible_achievers);
            assert(to_id >= 0);
            /*
              If there is an action which occurs in both landmarks, applying it
              resolves both landmarks as well as the ordering in one step. This
              special case (which is a consequence of the definition of
              reasonable orderings) makes a lot of things very complicated.
              Ignoring these cases may be desired sometimes which is why we do
              not take them over into our DALM-graph here.
            */
            if (!graph->landmarks_overlap(from_id, to_id)) {
                graph->add_edge(
                    from_id, to_id, child.second >= EdgeType::NATURAL);
            }
        }
    }
}

void FactLandmarkGraphTranslatorFactory::add_uaa_landmarks(dalm_graph &graph, const TaskProxy task_proxy) {
    int too_large_sets  = 0;
    // Collect for each fact which operators have it as precondition
    vector<vector<set<int>>> precondition_of(task_proxy.get_variables().size());
    vector<vector<bool>> fact_is_relevant_for_axioms(precondition_of.size());
    for (size_t i = 0; i < precondition_of.size(); ++i) {
        precondition_of[i].resize(task_proxy.get_variables()[i].get_domain_size());
        fact_is_relevant_for_axioms[i].resize(task_proxy.get_variables()[i].get_domain_size(), false);
    }

    for (OperatorProxy axiom_proxy : task_proxy.get_axioms()) {
        for (FactProxy cond : axiom_proxy.get_effects()[0].get_conditions()) {
            fact_is_relevant_for_axioms[cond.get_pair().var][cond.get_pair().value] = true;
        }
    }

    for (OperatorProxy op_proxy : task_proxy.get_operators()) {
        for (FactProxy pre : op_proxy.get_preconditions()) {
            precondition_of[pre.get_pair().var][pre.get_pair().value].insert(op_proxy.get_id());
        }
        for (EffectProxy eff : op_proxy.get_effects()) {
            for (FactProxy eff_cond : eff.get_conditions()) {
                precondition_of[eff_cond.get_pair().var][eff_cond.get_pair().value].insert(op_proxy.get_id());
            }
        }
    }


    GoalsProxy goal = task_proxy.get_goals();
    for (OperatorProxy op_proxy : task_proxy.get_operators()) {
        vector<const set<int> *> dalm_op_sets;
        dalm_op_sets.reserve(op_proxy.get_effects().size());
        int largest_set_index = 0;
        int size_estimate = 0;
        /*
         * We discard a potential uaa if
         *  - it is relevant for an axiom -> the derived variable might get used rather than the effect
         *  - it makes a goal true -> then it is useful in itself
         *  - its size exceeds a given threshold -> larger dalms are less useful and require more computational effort
         */
        bool discard = false;
        for (EffectProxy effect_proxy : op_proxy.get_effects()) {
            FactProxy effect = effect_proxy.get_fact();
            if (fact_is_relevant_for_axioms[effect.get_pair().var][effect.get_pair().value]) {
                discard = true;
                break;
            }
            for (FactProxy goal_fact : goal) {
                if (effect == goal_fact) {
                    discard = true;
                    break;
                }
            }
            if (discard) {
                break;
            }
            const set<int> *op_set = &(precondition_of[effect.get_pair().var][effect.get_pair().value]);
            size_estimate += op_set->size();
            if (size_estimate >= max_uaa_dalm_size) {
                discard = true;
                too_large_sets++;
                break;
            }
            dalm_op_sets.push_back(op_set);
            if (op_set->size() > dalm_op_sets[largest_set_index]->size()) {
                largest_set_index = dalm_op_sets.size()-1;
            }
        }
        if (discard) {
            continue;
        }

        // Swap the largest set to the front since copy assignment is faster than range insert.
        if (largest_set_index > 0) {
            swap(dalm_op_sets[0], dalm_op_sets[largest_set_index]);
        }
        set<int> uaa_landmark = *(dalm_op_sets[0]);
        for (size_t i = 1; i < dalm_op_sets.size(); ++i) {
            uaa_landmark.insert(dalm_op_sets[i]->begin(), dalm_op_sets[i]->end());
        }
        graph->add_node(uaa_landmark, true, op_proxy.get_id());
    }
    utils::g_log << "Number of too large uaa dalms: " << too_large_sets << endl;
}

shared_ptr<DisjunctiveActionLandmarkGraph> FactLandmarkGraphTranslatorFactory::compute_landmark_graph(
    const shared_ptr<AbstractTask> &task) {
    const TaskProxy task_proxy(*task);
    const State &initial_state = task_proxy.get_initial_state();
    LandmarkGraph &fact_graph = *lm->compute_lm_graph(task);
    remove_derived_landmarks(fact_graph);
    dalm_graph graph = make_shared<DisjunctiveActionLandmarkGraph>(uaa_landmarks, task_proxy);
    add_nodes(graph, fact_graph, initial_state);
    add_edges(graph, fact_graph, initial_state);
    if (uaa_landmarks) {
        add_uaa_landmarks(graph, task_proxy);
    }
    if (graph->get_number_of_landmarks() == 0) {
        graph->add_node({}, true);
    }
    if (uaa_landmarks) {
        graph->order_dalms_with_relevant_past_first();
    }

    utils::g_log << "Landmark graph of initial state contains "
                 << graph->get_number_of_landmarks() << " landmarks." << endl;
    utils::g_log << "Landmark graph of initial state contains "
                 << graph->get_number_of_orderings()
                 << " orderings of which "
                 << graph->get_number_of_strong_orderings()
                 << " are strong and "
                 << graph->get_number_of_weak_orderings()
                 << " are weak." << endl;
    //graph->dump_dot();
    return graph;
}

class FactLandmarkGraphTranslatorFactoryFeature
    : public plugins::TypedFeature<LandmarkGraphFactory, FactLandmarkGraphTranslatorFactory> {
public:
    FactLandmarkGraphTranslatorFactoryFeature()
        : TypedFeature("fact_translator") {
        document_title("TODO");
        document_synopsis(
            "Fact to Disjunctive Action Landmark Graph Translator");
        add_option<shared_ptr<LandmarkFactory>>(
            "lm", "Method to produce landmarks");
        add_option<bool>("uaa_landmarks",
                         "TODO",
                         "false");
        add_option<int>("max_uaa_dalm_size",
                         "TODO",
                         "1000");
    }
};

static plugins::FeaturePlugin<FactLandmarkGraphTranslatorFactoryFeature> _plugin;
}
