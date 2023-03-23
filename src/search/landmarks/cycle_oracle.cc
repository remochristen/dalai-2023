#include "cycle_oracle.h"

using namespace std;

namespace landmarks {
CycleOracle::CycleOracle(const vector<unordered_map<int, bool>> &graph,
                         bool ignore_ordering_types)
    : graph(graph), ignore_ordering_types(ignore_ordering_types) {
}
}
