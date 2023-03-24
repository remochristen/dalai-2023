#include "cyclic_landmark_heuristic.h"

#include "dalm_graph.h"

#include "../operator_counting/constraint_generator.h"
#include "../operator_counting/landmark_constraints.h"
#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/markup.h"

using namespace std;

namespace landmarks {
CyclicLandmarkHeuristic::CyclicLandmarkHeuristic(const plugins::Options &opts)
    : DisjunctiveActionLandmarkHeuristic(opts),
      lp_solver(opts.get<lp::LPSolverType>("lpsolver")),
      ip(opts.get<bool>("use_integer_operator_counts")) {
    initialize(opts);
    constraint_generators.emplace_back(
        make_shared<operator_counting::LandmarkConstraints>(
            opts, lm_graph, lm_status_manager));
    for (auto &constraint_generator : opts.get_list<shared_ptr<operator_counting::ConstraintGenerator>>("additional_constraint_generators")) {
        /* FIXME: This is an ugly hack to avoid that the LM-Cut constraint
            generator is added in our dalai-opt-2023 alias... */
        if (!task_properties::has_axioms(task_proxy)) {
            constraint_generators.push_back(constraint_generator);
        }
    }
    prepare_linear_program();
}

void CyclicLandmarkHeuristic::prepare_linear_program() {
    lp::LinearProgram lp(
        lp::LPObjectiveSense::MINIMIZE, compute_lp_variables(), {},
        lp_solver.get_infinity());
    for (const auto &generator : constraint_generators) {
        generator->initialize_constraints(task, lp);
    }
    lp_solver.load_problem(lp);
}

named_vector::NamedVector<lp::LPVariable> CyclicLandmarkHeuristic::compute_lp_variables() {
    double infinity = lp_solver.get_infinity();
    named_vector::NamedVector<lp::LPVariable> variables;
    for (OperatorProxy op : task_proxy.get_operators()) {
        int op_cost = op.get_cost();
        variables.emplace_back(0, infinity, op_cost, ip);
    }
    return variables;
}

int CyclicLandmarkHeuristic::get_heuristic_value(const State &ancestor_state) {
    assert(!lp_solver.has_temporary_constraints());
    // State state = convert_ancestor_state(ancestor_state);
    for (const auto &generator : constraint_generators) {
        bool dead_end =
            generator->update_constraints(ancestor_state, lp_solver);
        if (dead_end) {
            lp_solver.clear_temporary_constraints();
            return DEAD_END;
        }
    }
    int result;
    lp_solver.solve();
    if (lp_solver.has_optimal_solution()) {
        double epsilon = 0.01;
        double objective_value = lp_solver.get_objective_value();
        result = ceil(objective_value - epsilon);
    } else {
        result = DEAD_END;
    }
    lp_solver.clear_temporary_constraints();
    return result;
}

class CyclicLandmarkHeuristicFeature : public plugins::TypedFeature<Evaluator, CyclicLandmarkHeuristic> {
public:
    CyclicLandmarkHeuristicFeature() : TypedFeature("cyclic") {
        document_title("Cyclic landmark heuristic");
        document_synopsis(
            "See the paper" +
            utils::format_conference_reference(
                {"Clemens Büchner", "Thomas Keller", "Malte Helmert"},
                "Exploiting Cyclic Dependencies in Landmark Heuristics",
                "https://ai.dmi.unibas.ch/papers/buechner-et-al-icaps2021.pdf",
                "Proceedings of the 31st International Conference on "
                "Automated Planning and Search (ICAPS 2021)",
                "65-73",
                "AAAI Press",
                "2021"));

        add_option<bool>(
            "use_integer_operator_counts",
            "restrict operator counting variables to integer values. Computing "
            "the heuristic with integer variables can produce higher values "
            "but requires solving a MIP instead of an LP which is generally "
            "more computationally expensive. Turning this option on can thus "
            "drastically increase the runtime.",
            "false");
        add_list_option<shared_ptr<operator_counting::ConstraintGenerator>>(
            "additional_constraint_generators",
            "methods that generate constraints over operator-counting "
            "variables to complement (cyclic) landmark constraints.",
            "[]");

        document_language_support("action costs", "supported");
        document_language_support("conditional_effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "probably not"); // TODO: Figure it out.
        document_property("safe", "yes"); // TODO: At least I think so.
        document_property("preferred_operators", "no");

        lp::add_lp_solver_option_to_feature(*this);
        DisjunctiveActionLandmarkHeuristic::add_options_to_feature(*this);
        operator_counting::LandmarkConstraints::add_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<CyclicLandmarkHeuristicFeature> _plugin;
}
