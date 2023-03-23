#include "johnson_cycle_detection.h"

#include <algorithm>
#include <cassert>
#include <list>

using namespace std;

namespace johnson_cycles {
// TODO: convert to class to get rid of global variables.

vector<vector<int>> cycles;
// TODO: is it safe to replace *vector<int>* with *unordered_map<int>* in *B*?
vector<vector<int>> B;
vector<bool> blocked;
vector<int> stack;

void unblock(int u) {
    blocked[u] = false;
    while (!B[u].empty()) {
        int w = B[u].back();
        B[u].pop_back();
        if (blocked[w]) {
            unblock(w);
        }
    }
}

bool circuit(int v, int s, const vector<vector<int>> &A) {
    bool f = false;
    stack.push_back(v);
    blocked[v] = true;

    for (int w : A[v]) {
        if (w == s) {
            vector<int> cycle;
            cycle.reserve(stack.size());
            for (int n : stack) {
                cycle.push_back(n);
            }
            if (cycle.size() > 1) {
                cycles.push_back(cycle);
            }
            f = true;
        } else if (!blocked[w] && w > s) {
            if (circuit(w, s, A)) {
                f = true;
            }
        }
    }

    if (f) {
        unblock(v);
    } else {
        for (int w : A[v]) {
            if (find(B[w].begin(), B[w].end(), v) == B[w].end()) {
                B[w].push_back(v);
            }
        }
    }

    stack.pop_back();
    return f;
}

vector<vector<int>> compute_elementary_cycles(
    const vector<vector<int>> &graph) {
#ifndef NDEBUG
    for (const vector<int> &list : graph) {
        for (size_t i = 1; i < list.size(); ++i) {
            assert(list[i - 1] < list[i]);
        }
    }
#endif

    cycles.clear();
    int n = static_cast<int>(graph.size());
    B.resize(n);
    blocked.resize(n);

    int s = 0;
    while (s < n) {
        for (int i = s; i < n; ++i) {
            blocked[i] = false;
            B[i].clear();
        }
        circuit(s, s, graph);
        s++;
    }
    return cycles;
}
}
