#ifndef LANDMARKS_LANDMARK_FACTORY_REASONABLE_ORDERS_HPS_H
#define LANDMARKS_LANDMARK_FACTORY_REASONABLE_ORDERS_HPS_H

#include "dalm_graph_factory.h"

#include "../plugins/options.h"

namespace landmarks {
class DalmFactoryReasonableOrdersHPS : public LandmarkGraphFactory {
    std::shared_ptr<LandmarkGraphFactory> dalm_factory;
    bool ignore_disj_falms;

    std::shared_ptr<DisjunctiveActionLandmarkGraph> compute_landmark_graph(
            const std::shared_ptr<AbstractTask> &task) override;

    void approximate_reasonable_orders(const TaskProxy &task_proxy,
                                       std::shared_ptr<DisjunctiveActionLandmarkGraph> dalm_graph);
    bool interferes(
        const TaskProxy &task_proxy, const std::set<int> &achievers_of_a,
        const std::vector<FactPair> &landmark_b) const;
    bool effect_always_happens(
        const VariablesProxy &variables, const EffectsProxy &effects,
        std::set<FactPair> &eff) const;
public:
    explicit DalmFactoryReasonableOrdersHPS(const plugins::Options &opts);

};
}

#endif
