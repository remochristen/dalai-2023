#! /usr/bin/env python3

import os
import sys

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

sys.path.append(os.getcwd() + '/..')
import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/anytime"
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    ("5da5a63dd2978fce7b8d33a2a3df7fd58a41993b", ""),
]
BUILDS = ["ipc23_sat"]

CONFIGS = [
    IssueConfig(
        f"sat-final?",
        [],
        build_options=[build],
        driver_options=["--build", build, "--alias", "dalai-sat-2023"],
    ) for build in BUILDS
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    # memory_per_cpu="7744M",
    email="remo.christen@unibas.ch",
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
exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
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
