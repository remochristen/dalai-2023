#include "floyd_warshall_oracle.h"

#include <cassert>

using namespace std;

namespace landmarks {
FloydWarshallOracle::FloydWarshallOracle(
    const vector<unordered_map<int, bool>> &graph,
    bool ignore_ordering_types)
    : CycleOracle(graph, ignore_ordering_types),
      weights(graph.size(), 0.0),
      dist(graph.size(), vector<float>(graph.size(), infinity)),
      next(graph.size(), vector<int>(graph.size(), -1)) {
}

vector<int> FloydWarshallOracle::find_cycle(
    const vector<float> &counts) {
    assert(counts.size() == graph.size());
    compute_weights(counts);
    floyd_warshall();
    vector<int> cycle = extract_cycle_floyd_warshall();
    return cycle;
}

void FloydWarshallOracle::compute_weights(const vector<float> &counts) {
    assert(counts.size() == weights.size());
    for (size_t i = 0; i < counts.size(); ++i) {
        weights[i] = counts[i] - 1;
    }
}

void FloydWarshallOracle::floyd_warshall() {
    initialize_floyd_warshall();
    execute_floyd_warshall();
}

void FloydWarshallOracle::initialize_floyd_warshall() {
    size_t n = graph.size();
    assert(dist.size() == n);
    assert(next.size() == n);

    for (size_t v = 0; v < n; ++v) {
        dist[v] = vector<float>(n, infinity);
        next[v] = vector<int>(n, -1);
        /* We specifically do not want to initialize *dist[v][v]* to 0
           because we are interested in cycles. */
        for (pair<size_t, bool> w : graph[v]) {
            dist[v][w.first] =
                (w.second || ignore_ordering_types) ? weights[w.first] : 0;
            next[v][w.first] = w.first;
        }
    }
}

void FloydWarshallOracle::execute_floyd_warshall() {
    size_t n = graph.size();
    for (size_t k = 0; k < n; ++k) {
        for (size_t v = 0; v < n; ++v) {
            for (size_t w = 0; w < n; ++w) {
                if (dist[v][w] > dist[v][k] + dist[k][w]) {
                    dist[v][w] = dist[v][k] + dist[k][w];
                    next[v][w] = next[v][k];
                }
            }
        }
    }
}

vector<int> FloydWarshallOracle::extract_cycle_floyd_warshall() const {
    int index = find_shortest_cycle_index();
    if (index == -1) {
        return {};
    } else {
        vector<int> cycle = reconstruct_path(index, index);
        return cycle;
    }
}

int FloydWarshallOracle::find_shortest_cycle_index() const {
    int shortest_cycle_index = -1;
    float shortest_cycle_weight = infinity;

    for (size_t i = 0; i < graph.size(); ++i) {
        /* Without the epsilon, segfaults happen for some instances where
           precision seems to be an issue (also happens for double). */
        if (dist[i][i] < shortest_cycle_weight - epsilon) {
            shortest_cycle_index = i;
            shortest_cycle_weight = dist[i][i];
            if (shortest_cycle_weight == 0) {
                break;
            }
        }
    }
    return shortest_cycle_index;
}

vector<int> FloydWarshallOracle::reconstruct_path(int source,
                                                  int target) const {
    assert(0 <= source && static_cast<size_t>(source) < graph.size());
    assert(0 <= target && static_cast<size_t>(target) < graph.size());
    int index = source;
    vector<int> path{};
    do {
        path.push_back(index);
        index = next[index][target];
        assert(0 <= index && static_cast<size_t>(index) < graph.size());
    } while (index != target);
    return path;
}
}
