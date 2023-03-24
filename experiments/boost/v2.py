#! /usr/bin/env python3

import itertools
import os
import sys

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

sys.path.append(os.getcwd() + '/..')
import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/boost/"
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [("55f65a462d8004fa475c83da288d9c56e700b966", "")]
BUILDS = ["ipc23"]
CONFIG_NICKS = [
    (f"agile_boost={boost}", [
        "--search",
        "let(hlm,"
        f"dalm_sum(fact_translator(lm_reasonable_orders_hps(lm_rhw())),"
        f"transform=adapt_costs(one),pref=True), lazy_greedy([hlm], boost={boost}, preferred=[hlm], cost_type=one,"
        "reopen_closed=false))"
    ]) for boost in [, "1", "2", "3", "5", "10", "50", "100"]]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=[
            "--search-time-limit", "5m",
            "--build", build])
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_3",
    email="remo.christen@unibas.ch",
)

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
#exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("../landmark_parser.py")

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    #Attribute("landmarks", min_wins=False),
    #Attribute("landmarks_conjunctive", min_wins=False),
    #Attribute("landmarks_disjunctive", min_wins=False),
    #Attribute("orderings", min_wins=False),
    #Attribute("orderings_necessary", min_wins=False),
    #Attribute("orderings_greedy-necessary", min_wins=False),
    #Attribute("orderings_natural", min_wins=False),
    #Attribute("orderings_reasonable", min_wins=False),
    #Attribute("orderings_obedient-reasonable", min_wins=False),
    #Attribute("prog_goal", min_wins=False),
    #Attribute("prog_gn", min_wins=False),
    #Attribute("prog_r", min_wins=False),
]

exp.add_absolute_report_step(attributes=ATTRIBUTES)
#exp.add_comparison_table_step()
#exp.add_scatter_plot_step(relative=True, attributes=["total_time", "memory"])

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
