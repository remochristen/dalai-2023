#! /usr/bin/env python3

import os
import sys

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

sys.path.append(os.getcwd() + '/..')
import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/post-analysis"
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["IPC2023_AGL_BENCHMARKS"]
REVISIONS = [
    ("35c7de4b85bc0a9dff9038848891fe92ba25e2ac", ""),
]
BUILD = "release"

dalai_agl_lm_factory = "fact_translator(lm_reasonable_orders_hps(lm_rhw()))"
CONFIGS = [
    IssueConfig(
        f"lama-first",
        [],
        build_options=[BUILD],
        driver_options=[
            "--build", BUILD,
            "--alias", "lama-first",
            "--overall-time-limit", "5m",
        ],
    ),
    IssueConfig(
        f"dalai-agl",
        [],
        build_options=[BUILD],
        driver_options=[
            "--build", BUILD,
            "--alias", "dalai-agl-2023",
            "--overall-time-limit", "5m",
        ],
    ),
    IssueConfig(
        f"dalai-agl-plus-ff",
        [
            "--search",
            "--if-unit-cost",
            (
                f"let(hlm,dalm_sum({dalai_agl_lm_factory},pref=true),"
                "let(hff,ff(),"
                "lazy_greedy([hff,hlm],preferred=[hff,hlm],boost=0)))"
            ),
            "--if-non-unit-cost",
            (
                f"let(hlm,dalm_sum({dalai_agl_lm_factory},transform=adapt_costs(one),pref=true),"
                "let(hff,ff(),"
                "lazy_greedy([hff,hlm],preferred=[hff,hlm],boost=0,cost_type=one,reopen_closed=false)))"
            ),
            # Append --always to be on the safe side if we want to append
            # additional options later.
            "--always",
        ],
        build_options=[BUILD],
        driver_options=["--build", BUILD, "--overall-time-limit", "5m",],
    ),
]

SUITE = [
    "folding",
    "labyrinth",
    "quantum-layout",
    "recharging-robots",
    "ricochet-robots",
    "rubiks-cube",
    "slitherlink",
]

ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_3",
    # memory_per_cpu="7744M",
    email="remo.christen@unibas.ch",
    export=["PATH", "IPC23_AGL_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = ["rubiks-cube:p01.pddl"]
    ENVIRONMENT = LocalEnvironment(processes=4)

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
