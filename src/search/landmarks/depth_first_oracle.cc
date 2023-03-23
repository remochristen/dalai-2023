#include "depth_first_oracle.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace landmarks {
DepthFirstOracle::DepthFirstOracle(const vector<unordered_map<int, bool>> &graph,
                                   bool ignore_ordering_types)
    : CycleOracle(graph, ignore_ordering_types) {
}

void DepthFirstOracle::unblock(size_t start_index, size_t n) {
    assert(start_index < n);
    for (size_t i = start_index; i < n; ++i) {
        blocked[i] = false;
    }
}

vector<int> DepthFirstOracle::find_cycle(const vector<float> &counts) {
    this->counts = counts;
    size_t n = graph.size();
    assert(counts.size() == n);

    blocked.resize(n, false);
    vector<int> cycle{};
    size_t s = 0;
    while (s < n && cycle.empty()) {
        assert(stack.empty());
        unblock(s, n);
        cycle = circuit(s, s, 0.0);
        ++s;
    }
    stack.clear();
    return cycle;
}

vector<int> DepthFirstOracle::circuit(int v, int s,
                                      float accumulated_weight) {
    stack.push_back(v);
    blocked[v] = true;
    for (const pair<const int, bool> &w : graph[v]) {
        float new_accumulated_weight = accumulated_weight +
            (w.second || ignore_ordering_types ? counts[w.first] - 1.0 : 0.0);
        if (new_accumulated_weight < 1) {
            vector<int> cycle = process_successor(
                w.first, s, new_accumulated_weight);
            if (!cycle.empty()) {
                return cycle;
            }
        }
    }
    stack.pop_back();
    return {};
}

vector<int> DepthFirstOracle::process_successor(
    int w, int s, float accumulated_weight) {
    if (w == s) {
        assert(stack.size() > 1); // No self loops in LM-graphs
        /* FIXME: Self loops might be introduced when translating from
            fact- to disjunctive action landmarks. If this happens,
            requiring two action applications from that set should be
            valid. However, Johnson's algorithm, from which this oracle
            is inspired, cannot deal with self loops. */
        return stack;
    } else if (!blocked[w] && w > s) {
        return circuit(w, s, accumulated_weight);
    } else {
        return {};
    }
}
}
