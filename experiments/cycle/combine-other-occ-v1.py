#! /usr/bin/env python3

import os
import sys

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

sys.path.append(os.getcwd() + '/..')
import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/cyclic"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    "2ed127649",
]
BUILDS = ["ipc23_opt"]

CONFIGS = [
    IssueConfig(
        generator,
        ["--evaluator",
         "hlm=cyclic(lm_factory=fact_translator(lm_reasonable_orders_hps("
         f"lm_rhw())),cycle_generator=johnson,additional_constraint_generators=[{generator}()])",
         "--search",
         "astar(hlm)"],
        build_options=[build],
        driver_options=["--build", build],
    ) for generator in [
        "delete_relaxation_constraints",
        "lmcut_constraints",
        "pho_constraints",
        "state_equation_constraints",
    ]
    for build in BUILDS
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_3",
    # memory_per_cpu="7744M",
    email="clemens.buechner@unibas.ch",
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
# exp.add_scatter_plot_step(relative=True, attributes=["search_time", "cost"])
# exp.add_scatter_plot_step(relative=False, attributes=["search_time", "cost"])

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
