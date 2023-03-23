#ifndef ALGORITHMS_JOHNSON_CYCLE_DETECTION_H
#define ALGORITHMS_JOHNSON_CYCLE_DETECTION_H

#include <vector>

namespace johnson_cycles {
/*
  This function implements Johnson's algorithm for finding elementary
  cycles in a directed graph. The runtime is O((n + m)(c + 1)) for a
  directed graph with n vertices, m arcs, and c cycles.

  Input: a directed graph represented as a vector of vectors, where
  *graph[i]* is the vector of successors of vertex i. Successors must be
  ordered in ascending order for Johnson's algorithm to work.

  Output: a vector of cycles, each of which is a vector of vertices.
 */
std::vector<std::vector<int>> compute_elementary_cycles(
    const std::vector<std::vector<int>> &graph);
}


#endif
