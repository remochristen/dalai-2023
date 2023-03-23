#ifndef LANDMARKS_FACT_LANDMARK_GRAPH_TRANSLATOR_FACTORY_H
#define LANDMARKS_FACT_LANDMARK_GRAPH_TRANSLATOR_FACTORY_H

#include "dalm_graph_factory.h"

#include "../plugins/options.h"

namespace landmarks {
class LandmarkFactory;

class FactLandmarkGraphTranslatorFactory : public LandmarkGraphFactory {
    const std::shared_ptr<LandmarkFactory> lm;
    std::map<std::set<int>, size_t> ids;
    bool uaa_landmarks;
    int max_uaa_dalm_size;

    void add_nodes(dalm_graph &graph, const LandmarkGraph &lm_graph,
                   const State &init_state);
    void add_edges(dalm_graph &graph, const LandmarkGraph &lm_graph,
                   const State &init_state);

    void add_uaa_landmarks(dalm_graph &graph, const TaskProxy task_proxy);
public:
    explicit FactLandmarkGraphTranslatorFactory(const plugins::Options &opts);

    virtual std::shared_ptr<DisjunctiveActionLandmarkGraph> compute_landmark_graph(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
