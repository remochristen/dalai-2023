#ifndef LANDMARKS_FACT_LANDMARK_GRAPH_TRANSLATOR_FACTORY_H
#define LANDMARKS_FACT_LANDMARK_GRAPH_TRANSLATOR_FACTORY_H

#include "dalm_graph_factory.h"

#include "../plugins/options.h"

namespace landmarks {
class LandmarkFactory;

class FactLandmarkGraphTranslatorFactory : public LandmarkGraphFactory {
    const std::shared_ptr<LandmarkFactory> lm;

    std::shared_ptr<AbstractTask> task;

    std::map<std::set<int>, size_t> ids;

    bool initialized = false;

    std::vector<size_t> add_nodes(
        dalm_graph &graph, const LandmarkGraph &lm_graph,
        const State &init_state);
    void add_edges(dalm_graph &graph, const LandmarkGraph &lm_graph,
                   const State &init_state,
                   const std::vector<size_t> &fact_to_action_lm_map);
public:
    explicit FactLandmarkGraphTranslatorFactory(const plugins::Options &opts);

    virtual void initialize(
        const std::shared_ptr<AbstractTask> &original_task) override;

    virtual std::shared_ptr<DisjunctiveActionLandmarkGraph> get_landmark_graph(
        const State &state) override;
};

}

#endif
