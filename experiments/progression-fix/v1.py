#! /usr/bin/env python3

import os
import sys

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

sys.path.append(os.getcwd() + '/..')
import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/progression-fix"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    "91b29e166d560c507a2519ce46552dce8c981e25",
    "fb70f76836924538bb17717f3e44c3dea2b7cd6c"
]

CONFIG_NICKS = [
    ("dalm-sum", ["--search",
                 "let(hlm, dalm_sum(fact_translator(lm_reasonable_orders_hps(lm_rhw())), transform=adapt_costs(one)), lazy_greedy([hlm], preferred=[hlm], cost_type=one, reopen_closed=false))"]),
    ("dalm-greedy-hs", ["--search",
                 "let(hlm, dalm_greedy_hs(fact_translator(lm_reasonable_orders_hps(lm_rhw())), transform=adapt_costs(one)), lazy_greedy([hlm], preferred=[hlm], cost_type=one, reopen_closed=false))"])
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config)
    for config_nick, config in CONFIG_NICKS
]


SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    email="salome.eriksson@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=3)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("../landmark_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("landmarks_disjunctive", min_wins=False),
    Attribute("landmarks_conjunctive", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("lmgraph_generation_time", min_wins=True),
]

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch', merge=True)

exp.add_absolute_report_step(attributes=ATTRIBUTES)
exp.add_scatter_plot_step(relative=True, attributes=["search_time", "cost"])
exp.add_scatter_plot_step(relative=False, attributes=["search_time", "cost"])

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
