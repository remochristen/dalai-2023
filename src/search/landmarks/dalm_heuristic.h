#ifndef LANDMARKS_DALM_HEURISTIC_H
#define LANDMARKS_DALM_HEURISTIC_H

#include "../heuristic.h"

class BitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkGraphFactory;
class DisjunctiveActionLandmarkGraph;
class DisjunctiveActionLandmarkNode;
class DisjunctiveActionLandmarkStatusManager;

class DisjunctiveActionLandmarkHeuristic : public Heuristic {
    const bool use_preferred_operators;

    std::shared_ptr<successor_generator::SuccessorGenerator> successor_generator;
    std::unordered_map<int, std::vector<int>> landmarks_by_operator;
protected:
    std::shared_ptr<DisjunctiveActionLandmarkGraph> lm_graph;

    std::shared_ptr<DisjunctiveActionLandmarkStatusManager> lm_status_manager;

    void initialize(const plugins::Options &opts);
    void compute_landmark_graph(const plugins::Options &opts);

    /*
      Unlike most landmark-related code, this function takes the
      task-transformation of the state, not the original one (i.e., not
      *ancestor_state*). This is because updating the landmark status manager
      happens in *compute_heuristic(...)* before *get_heuristic_value(...)*
      is called. Here, we only compute a heuristic value based on the
      information in the landmark status manager, which does not require the
      state at this point. The only reason we need this argument is to guarantee
      goal-awareness of the LM-count heuristic which does not hold under the
      current function used for progressing the landmark statuses. Checking
      whether a state is a goal state requires the task-transformed state.
    */
    virtual int get_heuristic_value(const State &ancestor_state) = 0;

    void generate_preferred_operators(const State &state);
    virtual int compute_heuristic(const State &ancestor_state) override;

    void precompute_landmarks_by_operator_lookup();

    const std::vector<int> &get_landmark_ids_for_operator(int op_id) {
        return landmarks_by_operator[op_id];
    }
public:
    explicit DisjunctiveActionLandmarkHeuristic(const plugins::Options &opts);

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    static void add_options_to_feature(plugins::Feature &feature);

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};
}

#endif
