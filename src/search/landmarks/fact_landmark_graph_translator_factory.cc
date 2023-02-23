#include "fact_landmark_graph_translator_factory.h"

#include "landmark_factory.h"

#include "../plugins/plugin.h"

#include <algorithm>

using namespace std;

namespace landmarks {
FactLandmarkGraphTranslatorFactory::FactLandmarkGraphTranslatorFactory(
    const plugins::Options &opts)
    : lm(opts.get<std::shared_ptr<LandmarkFactory>>("lm")) {
}

void FactLandmarkGraphTranslatorFactory::add_nodes(
    dalm_graph &graph, const LandmarkGraph &lm_graph, const State &init_state) {
    for (const unique_ptr<LandmarkNode> &node : lm_graph.get_nodes()) {
        const Landmark &fact_lm = node->get_landmark();

        bool is_initially_true = fact_lm.is_true_in_state(init_state);

        if (!is_initially_true) {
            graph->add_node(fact_lm.first_achievers);
        }
        if (fact_lm.is_true_in_goal) {
            size_t lm_id = graph->add_node(fact_lm.possible_achievers);
            assert(fact_lm.facts.size() == 1);
            graph->mark_lm_goal_achiever(fact_lm.facts[0], lm_id);
            if (is_initially_true) {
                graph->mark_lm_initially_past(lm_id);
            }
        }
        for (auto &parent : node->parents) {
            if (parent.second == EdgeType::REASONABLE) {
                graph->add_node(fact_lm.possible_achievers);
                break;
            }
        }
        for (auto &child : node->children) {
            if (child.second == EdgeType::GREEDY_NECESSARY) {
                size_t achiever_id =
                    graph->add_node(fact_lm.possible_achievers);
                size_t preconditioned_id = graph->add_node(
                    child.first->get_landmark().first_achievers);
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
        if (fact_lm.is_true_in_state(init_state)) {
            /* All edges starting in initially true facts are not
               interesting for us since the cycles they possibly induce
               are already resolved initially. */
            continue;
        }
        int from_id = graph->get_id(fact_lm.first_achievers);
        assert(from_id >= 0);
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

shared_ptr<DisjunctiveActionLandmarkGraph> FactLandmarkGraphTranslatorFactory::compute_landmark_graph(
    const shared_ptr<AbstractTask> &task) {
    const TaskProxy task_proxy(*task);
    const State &initial_state = task_proxy.get_initial_state();
    const LandmarkGraph &fact_graph = *lm->compute_lm_graph(task);
    dalm_graph graph = make_shared<DisjunctiveActionLandmarkGraph>();
    add_nodes(graph, fact_graph, initial_state);
    add_edges(graph, fact_graph, initial_state);

    utils::g_log << "Landmark graph of initial state contains "
                 << graph->get_number_of_landmarks() << endl;
    utils::g_log << "Landmark graph of initial state contains "
                 << graph->get_number_of_orderings()
                 << " orderings of which "
                 << graph->get_number_of_strong_orderings()
                 << " are strong and "
                 << graph->get_number_of_weak_orderings()
                 << " are weak." << endl;
    graph->dump_dot();
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
    }
};

static plugins::FeaturePlugin<FactLandmarkGraphTranslatorFactoryFeature> _plugin;
}
