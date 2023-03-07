#include "fact_landmark_graph_translator_factory.h"

#include "landmark_factory.h"

#include "../plugins/plugin.h"
#include "../utils/timer.h"

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
      uaa_landmarks(opts.get<bool>("uaa_landmarks")) {
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
    for (auto &node: lm_graph.get_nodes()) {
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
    // Collect for each fact which operators have it as precondition
    vector<vector<set<int>>> precondition_of(task_proxy.get_variables().size());
    for (size_t i = 0; i < precondition_of.size(); ++i) {
        precondition_of[i].resize(task_proxy.get_variables()[i].get_domain_size());
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

    utils::Timer gather_tagged(false);
    utils::Timer make_set(false);
    utils::Timer insert_lm(false);
    utils::Timer insert_set(false);
    GoalsProxy goal = task_proxy.get_goals();
    vector<bool> tagged(task_proxy.get_operators().size(), false);
    for (OperatorProxy op_proxy : task_proxy.get_operators()) {
        // If the operator makes a goal true, then it is useful in itself, no uaa landmark needed.
        bool effect_contains_goal = false;
        for (EffectProxy effect_proxy : op_proxy.get_effects()) {
            FactProxy effect = effect_proxy.get_fact();
            for (FactProxy goal_fact : goal) {
                if (effect == goal_fact) {
                    effect_contains_goal = true;
                    break;
                }
            }
            if (effect_contains_goal) {
                break;
            }
        }
        if (effect_contains_goal) {
            continue;
        }
        // The operator does not make a goal true -> one of its effect must be used as a precondition at some point.
        set<int> uaa_landmark;
        gather_tagged.resume();
        for (EffectProxy effect : op_proxy.get_effects()) {
            FactPair pair = effect.get_fact().get_pair();
//            set<int> &ops = precondition_of[pair.var][pair.value];
            for (int op : precondition_of[pair.var][pair.value]) {
                if (!tagged[op]) {
                    tagged[op] = true;
                }
            }
//            if (uaa_landmark.empty()) {
//                uaa_landmark = ops;
//            } else {
//                uaa_landmark.insert(ops.begin(), ops.end());
//            }
        }
        gather_tagged.stop();
        make_set.resume();
        for (size_t i = 0; i < tagged.size(); ++i) {
            if (tagged[i]) {
//                insert_set.resume();
                uaa_landmark.insert(uaa_landmark.end(), i);
//                insert_set.stop();
                tagged[i] = false;
            }
        }
        make_set.stop();
        insert_lm.resume();
        graph->add_node(uaa_landmark, true, op_proxy.get_id());
        insert_lm.stop();
    }
    cout << "Gather tagged timer: " << gather_tagged << endl;
    cout << "Make set timer:      " << make_set << endl;
    cout << "Insert set timer:    " << insert_set << endl;
    cout << "Insert lm timer:     " << insert_lm << endl;
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

    utils::g_log << "Landmark graph of initial state contains "
                 << graph->get_number_of_landmarks() << endl;
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
    }
};

static plugins::FeaturePlugin<FactLandmarkGraphTranslatorFactoryFeature> _plugin;
}
