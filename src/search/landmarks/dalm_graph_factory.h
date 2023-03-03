#ifndef LANDMARKS_DALM_GRAPH_FACTORY_H
#define LANDMARKS_DALM_GRAPH_FACTORY_H

#include "dalm_graph.h"

namespace landmarks {
class DisjunctiveActionLandmarkGraph;

using dalm_graph = std::shared_ptr<DisjunctiveActionLandmarkGraph>;

class LandmarkGraphFactory {
public:
    virtual ~LandmarkGraphFactory() = default;

    virtual std::shared_ptr<DisjunctiveActionLandmarkGraph> compute_landmark_graph(
        const std::shared_ptr<AbstractTask> &task) = 0;
};
}

#endif
