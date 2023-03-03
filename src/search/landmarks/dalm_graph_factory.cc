#include "dalm_graph_factory.h"

#include "../plugins/plugin.h"

namespace landmarks {
static class DisjunctiveActionLandmarkGraphFactoryCategoryPlugin
    : public plugins::TypedCategoryPlugin<LandmarkGraphFactory> {
public:
    DisjunctiveActionLandmarkGraphFactoryCategoryPlugin()
        : TypedCategoryPlugin("DalmGraphFactory") {
        document_synopsis("TODO: Doc");
        allow_variable_binding();
    }
}
_category_plugin;
}
