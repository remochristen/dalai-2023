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

DisjunctiveActionLandmarkGraph::DisjunctiveActionLandmarkGraph(bool uaa_landmarks, const TaskProxy task_proxy)
    : uaa_landmarks(uaa_landmarks), non_uaa_dalms(-1) {
    if (uaa_landmarks) {
        op_to_uaa_lm = vector<int>(task_proxy.get_operators().size(), -1);
    }
}

size_t DisjunctiveActionLandmarkGraph::add_node(const set<int> &actions, bool true_in_initial, int op_id) {
    auto it = ids.find(actions);
    if (it == ids.end()) {
        size_t id = ids.size();
        ids[actions] = id;
        lms.emplace_back(actions);
        lm_true_in_initial.push_back(true_in_initial);
        if (uaa_landmarks && op_id >= 0) {
            op_to_uaa_lm[op_id] = id;
        }
        return id;
    }
    /*
     * If several fact landmarks lead to the same dalm, then it is true in initial
     * only if *all* fact landmarks are true in the initial state.
     */
    if (!true_in_initial) {
        lm_true_in_initial[it->second] = false;
    }
    if (uaa_landmarks && op_id >= 0) {
        op_to_uaa_lm[op_id] = it->second;
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

void DisjunctiveActionLandmarkGraph::mark_lm_goal_achiever(
    const FactPair &fact_pair, size_t lm) {
    //assert(goal_achiever_lms.count(fact_pair) == 0);
    goal_achiever_lms.emplace_back(fact_pair, lm);
}

void DisjunctiveActionLandmarkGraph::mark_lm_precondition_achiever(
    const vector<FactPair> &fact_pairs, size_t achiever_lm,
    size_t preconditioned_lm) {
    //assert(!get_actions(achiever_lm).empty());
    //assert(precondition_achiever_lms.count(pair) == 0);
    precondition_achiever_lms.emplace_back(
        fact_pairs, achiever_lm, preconditioned_lm);
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

int DisjunctiveActionLandmarkGraph::get_uaa_landmark_for_operator(int op_id) const {
    assert(op_to_uaa_lm.size() > op_id);
    return op_to_uaa_lm[op_id];
}
}
