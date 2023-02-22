#ifndef LANDMARKS_DALM_GRAPH_FACTORY_H
#define LANDMARKS_DALM_GRAPH_FACTORY_H

#include "dalm_graph.h"

namespace landmarks {
class DisjunctiveActionLandmarkGraph;

using dalm_graph = std::shared_ptr<DisjunctiveActionLandmarkGraph>;

class LandmarkGraphFactory {
public:
    virtual ~LandmarkGraphFactory() = default;

    virtual void initialize(
        const std::shared_ptr<AbstractTask> &original_task) = 0;

    virtual dalm_graph get_landmark_graph(const State &state) = 0;
};
}

#endif
