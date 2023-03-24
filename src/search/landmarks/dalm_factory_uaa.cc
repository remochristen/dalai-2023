#include "dalm_factory_uaa.h"

#include "landmark_factory.h"
#include "../plugins/plugin.h"

#include <algorithm>

using namespace std;

namespace landmarks {
    DalmFactoryUAA::DalmFactoryUAA(const plugins::Options &opts)
             :LandmarkGraphFactory(),
              dalm_factory(opts.get<shared_ptr<LandmarkGraphFactory>>("dalm-factory")),
              max_uaa_dalm_size(opts.get<int>("max_uaa_dalm_size")) {
    }


    shared_ptr<DisjunctiveActionLandmarkGraph>  DalmFactoryUAA::compute_landmark_graph(const shared_ptr<AbstractTask> &task) {
        const TaskProxy task_proxy(*task);
        const State &initial_state = task_proxy.get_initial_state();
        int too_large_sets  = 0;
        shared_ptr<DisjunctiveActionLandmarkGraph> dalm_graph = dalm_factory->compute_landmark_graph(task);
        dalm_graph->order_dalms_with_relevant_past_first();

        // Collect for each fact which operators have it as precondition
        vector<vector<set<int>>> precondition_of(task_proxy.get_variables().size());
        vector<vector<bool>> fact_is_relevant_for_axioms(precondition_of.size());
        for (size_t i = 0; i < precondition_of.size(); ++i) {
            precondition_of[i].resize(task_proxy.get_variables()[i].get_domain_size());
            fact_is_relevant_for_axioms[i].resize(task_proxy.get_variables()[i].get_domain_size(), false);
        }

        for (OperatorProxy axiom_proxy : task_proxy.get_axioms()) {
            for (FactProxy cond : axiom_proxy.get_effects()[0].get_conditions()) {
                fact_is_relevant_for_axioms[cond.get_pair().var][cond.get_pair().value] = true;
            }
        }

        for (OperatorProxy op_proxy : task_proxy.get_operators()) {
            for (FactProxy pre : op_proxy.get_preconditions()) {
                precondition_of[pre.get_pair().var][pre.get_pair().value].insert(op_proxy.get_id());
            }
            for (EffectProxy eff : op_proxy.get_effects()) {
                for (FactProxy eff_cond : eff.get_conditions()) {
                    precondition_of[eff_cond.get_pair().var][eff_cond.get_pair().value].insert(op_proxy.get_id());
                }
            }
        }


        GoalsProxy goal = task_proxy.get_goals();
        for (OperatorProxy op_proxy : task_proxy.get_operators()) {
            vector<const set<int> *> dalm_op_sets;
            dalm_op_sets.reserve(op_proxy.get_effects().size());
            int largest_set_index = 0;
            int size_estimate = 0;
            /*
             * We discard a potential uaa if
             *  - it is relevant for an axiom -> the derived variable might get used rather than the effect
             *  - it makes a goal true -> then it is useful in itself
             *  - its size exceeds a given threshold -> larger dalms are less useful and require more computational effort
             */
            bool discard = false;
            for (EffectProxy effect_proxy : op_proxy.get_effects()) {
                FactProxy effect = effect_proxy.get_fact();
                if (fact_is_relevant_for_axioms[effect.get_pair().var][effect.get_pair().value]) {
                    discard = true;
                    break;
                }
                for (FactProxy goal_fact : goal) {
                    if (effect == goal_fact) {
                        discard = true;
                        break;
                    }
                }
                if (discard) {
                    break;
                }
                const set<int> *op_set = &(precondition_of[effect.get_pair().var][effect.get_pair().value]);
                size_estimate += op_set->size();
                if (size_estimate >= max_uaa_dalm_size) {
                    discard = true;
                    too_large_sets++;
                    break;
                }
                dalm_op_sets.push_back(op_set);
                if (op_set->size() > dalm_op_sets[largest_set_index]->size()) {
                    largest_set_index = dalm_op_sets.size()-1;
                }
            }
            if (discard) {
                continue;
            }

            // Swap the largest set to the front since copy assignment is faster than range insert.
            if (largest_set_index > 0) {
                swap(dalm_op_sets[0], dalm_op_sets[largest_set_index]);
            }
            set<int> uaa_landmark = *(dalm_op_sets[0]);
            for (size_t i = 1; i < dalm_op_sets.size(); ++i) {
                uaa_landmark.insert(dalm_op_sets[i]->begin(), dalm_op_sets[i]->end());
            }
            dalm_graph->add_node(uaa_landmark, true, op_proxy.get_id());
        }
        utils::g_log << "Number of too large uaa dalms: " << too_large_sets << endl;
        utils::g_log << "Landmark graph of initial state contains "
                     << dalm_graph->get_number_of_landmarks() << " landmarks." << endl;
        utils::g_log << "Landmark graph of initial state contains "
                     << dalm_graph->get_number_of_orderings()
                     << " orderings of which "
                     << dalm_graph->get_number_of_strong_orderings()
                     << " are strong and "
                     << dalm_graph->get_number_of_weak_orderings()
                     << " are weak." << endl;
        return dalm_graph;
    }

    class DalmFactoryUAAFeature : public plugins::TypedFeature<LandmarkGraphFactory, DalmFactoryUAA> {
    public:
        DalmFactoryUAAFeature()
                : TypedFeature("dalm_uaa") {
            document_title("TODO");
            document_synopsis(
                    "Unnecessary Action Application disjunctive action landmarks");
            add_option<shared_ptr<LandmarkGraphFactory>>(
                    "dalm-factory", "dalm graph factory");
            add_option<int>("max_uaa_dalm_size",
                            "TODO",
                            "1000");
        }
    };

    static plugins::FeaturePlugin<DalmFactoryUAAFeature> _plugin;
}
