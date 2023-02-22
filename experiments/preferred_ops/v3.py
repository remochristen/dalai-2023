#! /usr/bin/env python3

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "ai/christen/ipc23-landmarks"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    "20d13dc22f5d94d7985b3608fc7906404833bd6b",
]


CONFIGS = [
    IssueConfig(
        f"lama-first-method=is_future-simple=false",
        ["--evaluator",
         "hlm=landmark_sum(lm_factory=lm_reasonable_orders_hps(lm_rhw()),"
         "transform=adapt_costs(one),pref=true)",
         "--search",
         "lazy_greedy([hlm],preferred=[hlm],cost_type=one,reopen_closed=false)"],
        driver_options=["--overall-memory-limit", "7600M"]),
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    memory_per_cpu="7744M",
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
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("landmark_parser.py")

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
