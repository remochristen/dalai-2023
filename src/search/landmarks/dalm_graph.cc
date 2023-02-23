#include "dalm_graph.h"

#include <cassert>

using namespace std;

namespace landmarks {
DisjunctiveActionLandmarkNode::DisjunctiveActionLandmarkNode(set<int> actions)
    : actions(move(actions)) {
}

bool DisjunctiveActionLandmarkNode::overlaps_with(
    DisjunctiveActionLandmarkNode &other) const {

    vector<int> intersect;
    set_intersection(actions.begin(), actions.end(),
                     other.actions.begin(), other.actions.end(),
                     back_inserter(intersect));
    return !intersect.empty();
}

bool DisjunctiveActionLandmarkNode::satisfied_by(int op_id) const {
    return actions.find(op_id) != actions.end();
}

void DisjunctiveActionLandmarkNode::add_strong_dependency(
    int node_id, size_t &num_strong_orderings, size_t &num_weak_orderings) {
    if (dependencies.find(node_id) != dependencies.end()) {
        if (dependencies[node_id] == OrderingType::WEAK) {
            --num_weak_orderings;
        } else {
            // There already exists a strong ordering.
            return;
        }
    }
    dependencies[node_id] = OrderingType::STRONG;
    ++num_strong_orderings;
}

void DisjunctiveActionLandmarkNode::add_weak_dependency(
    int node_id, size_t &num_weak_orderings) {
    if (dependencies.find(node_id) == dependencies.end()) {
        dependencies[node_id] = OrderingType::WEAK;
        ++num_weak_orderings;
    }
}

OrderingType DisjunctiveActionLandmarkNode::get_dependency(int node_id) const {
    assert(depends_on(node_id));
    return dependencies.at(node_id);
}

const map<int, OrderingType> &DisjunctiveActionLandmarkNode::get_dependencies() const {
    return dependencies;
}

bool DisjunctiveActionLandmarkNode::depends_on(int node_id) const {
    return dependencies.count(node_id);
}

size_t DisjunctiveActionLandmarkGraph::add_node(const set<int> &actions) {
    auto it = ids.find(actions);
    if (it == ids.end()) {
        size_t id = ids.size();
        ids[actions] = id;
        lms.emplace_back(actions);
        return id;
    }
    return it->second;
}

void DisjunctiveActionLandmarkGraph::add_edge(
    size_t from, size_t to, bool strong) {
    assert(utils::in_bounds(from, lms));
    assert(utils::in_bounds(to, lms));

    DisjunctiveActionLandmarkNode &dalm_node = lms[to];
    if (strong) {
        dalm_node.add_strong_dependency(
            from, num_strong_orderings, num_weak_orderings);
    } else {
        dalm_node.add_weak_dependency(from, num_weak_orderings);
    }
}

void DisjunctiveActionLandmarkGraph::mark_lm_initially_past(size_t id) {
    initially_past_lms.push_back(id);
}

int DisjunctiveActionLandmarkGraph::get_id(const set<int> &actions) {
    if (ids.count(actions)) {
        return static_cast<int>(ids[actions]);
    } else {
        // No corresponding landmark exists in this graph.
        return -1;
    }
}

bool DisjunctiveActionLandmarkGraph::landmarks_overlap(size_t lm1, size_t lm2) {
    return lm1 == lm2 || lms[lm1].overlaps_with(lms[lm2]);
}

void DisjunctiveActionLandmarkGraph::dump_lm(int id) const {
    cout << "lm" << id << ": <";
    for (int action : lms[id].actions) {
        cout << action << " ";
    }
    cout << ">" << endl;

    if (!lms[id].get_dependencies().empty()) {
        cout << "\tdepends on ";
        for (auto dep : lms[id].get_dependencies()) {
            cout << "lm" << dep.first << "(";
            switch (dep.second) {
            case OrderingType::STRONG:
                cout << "s";
                break;
            case OrderingType::WEAK:
                cout << "w";
                break;
            default:
                cout << "?";
                break;
            }
            cout << ") ";
        }
        cout << endl;
    }
}

void DisjunctiveActionLandmarkGraph::dump() const {
    cout << "== Disjunctive Action Landmark Graph ==" << endl;
    for (size_t id = 0; id < lms.size(); ++id) {
        dump_lm(id);
    }
    cout << "== End of Graph ==" << endl;
}

void DisjunctiveActionLandmarkGraph::dump_dot() const {
    cout << "digraph graphname {\n";
    for (size_t id = 0; id < lms.size(); ++id) {
        cout << "  lm" << id << " [label=\"<";
        for (int a : lms[id].actions) {
            cout << a << " ";
        }
        cout << ">\"];\n";
    }
    cout << "\n";
    for (size_t id = 0; id < lms.size(); ++id) {
        for (pair<int, OrderingType> dep : lms[id].get_dependencies()) {
            cout << "  lm" << dep.first << " -> lm" << id;
            if (dep.second == OrderingType::WEAK) {
                cout << " [style=dotted]";
            }
            cout << ";\n";
        }
    }
    cout << "}" << endl;
}

size_t DisjunctiveActionLandmarkGraph::get_number_of_landmarks() const {
    return lms.size();
}

OrderingType DisjunctiveActionLandmarkGraph::get_ordering_type(
    int from, int to) {
    assert(0 <= from && from < static_cast<int>(lms.size()));
    assert(0 <= to && to < static_cast<int>(lms.size()));
    assert(lms[to].depends_on(from));
    return lms[to].get_dependency(from);
}

const set<int> &DisjunctiveActionLandmarkGraph::get_actions(int id) {
    assert(0 <= id && id < static_cast<int>(lms.size()));
    return lms[id].actions;
}

const map<int, OrderingType> &DisjunctiveActionLandmarkGraph::get_dependencies(
    int id) {
    assert(0 <= id && id < static_cast<int>(lms.size()));
    return lms[id].get_dependencies();
}

vector<map<int, bool>> DisjunctiveActionLandmarkGraph::to_adj_list() const {
    size_t n = get_number_of_landmarks();
    vector<map<int, bool>> adj(n, map<int, bool>{});
    for (size_t id = 0; id < n; ++id) {
        for (pair<int, OrderingType> dep : lms[id].get_dependencies()) {
            adj[dep.first][id] = dep.second == OrderingType::WEAK;
        }
    }
    return adj;
}
}
