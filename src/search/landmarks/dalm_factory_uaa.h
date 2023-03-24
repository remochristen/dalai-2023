#ifndef LANDMARKS_DALM_FACTORY_UAA_H
#define LANDMARKS_DALM_FACTORY_UAA_H

#include "dalm_graph_factory.h"

#include "../plugins/options.h"

namespace landmarks {
class DalmFactoryUAA : public LandmarkGraphFactory {
    std::shared_ptr<LandmarkGraphFactory> dalm_factory;
    int max_uaa_dalm_size;

    std::shared_ptr<DisjunctiveActionLandmarkGraph> compute_landmark_graph(
            const std::shared_ptr<AbstractTask> &task) override;
public:
    explicit DalmFactoryUAA(const plugins::Options &opts);
};
}


#endif