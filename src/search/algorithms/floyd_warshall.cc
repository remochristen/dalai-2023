#include "floyd_warshall.h"

#include <cassert>

using namespace std;

namespace floyd_warshall {
FloydWarshall::FloydWarshall(const adj_list &graph, bool cyclic,
                             float infinity)
    : graph(graph), cyclic(cyclic), infinity(infinity) {
}

void FloydWarshall::run() {
    initialize();
    execute();
}

void FloydWarshall::initialize() {
    size_t n = graph.size();
    dist.resize(n, vector<float>(n, infinity));
    next.resize(n, vector<int>(n, -1));

    for (size_t v = 0; v < n; ++v) {
        if (!cyclic) {
            dist[v][v] = 0;
            next[v][v] = v;
        }
        for (pair<int, float> w : graph[v]) {
            dist[v][w.first] = w.second;
            next[v][w.first] = w.first;
        }
    }
}

void FloydWarshall::execute() {
    for (size_t k = 0; k < graph.size(); ++k) {
        for (size_t v = 0; v < graph.size(); ++v) {
            for (size_t w = 0; w < graph.size(); ++w) {
                if (dist[v][w] > dist[v][k] + dist[k][w]) {
                    dist[v][w] = dist[v][k] + dist[k][w];
                    next[v][w] = next[v][k];
                }
            }
        }
    }
}

float FloydWarshall::get_length(int source, int target) const {
    assert(0 <= source && static_cast<size_t>(source) < graph.size());
    assert(0 <= target && static_cast<size_t>(target) < graph.size());
    return dist[source][target];
}

std::vector<int> FloydWarshall::reconstruct_path(int source, int target) {
    assert(0 <= source && static_cast<size_t>(source) < graph.size());
    assert(0 <= target && static_cast<size_t>(target) < graph.size());

    vector<int> path{};
    int index = source;
    do {
        path.push_back(index);
        index = next[index][target];
        assert(0 <= index && static_cast<size_t>(index) < graph.size());
    } while (index != target);
    if (source != target) {
        path.push_back(target);
    }
    return path;
}
}
