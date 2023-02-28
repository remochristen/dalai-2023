#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/dalm/"
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    "2806a4d76",
    "0c9a3b253",
]
BUILDS = ["ipc23"]
CONFIG_NICKS = [
    (f"rhw-first", [
        "--search",
        "let(hlm,"
        "dalm_sum(fact_translator(lm_reasonable_orders_hps(lm_rhw())),"
        f"transform=adapt_costs(one), prog_goal=true,prog_gn=true,"
        f"prog_w=true), lazy_greedy([hlm], cost_type=one,"
        "reopen_closed=false))"
    ]),
    (f"rhw-possible", [
        "--search",
        "let(hlm,"
        "dalm_sum(fact_translator_possible(lm_reasonable_orders_hps(lm_rhw())),"
        f"transform=adapt_costs(one), prog_goal=true,prog_gn=true,"
        f"prog_w=true), lazy_greedy([hlm], cost_type=one,"
        "reopen_closed=false))"
    ]),
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=[
            "--search-time-limit", "30m",
            "--overall-memory-limit", "7600M",
            "--build", build])
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    memory_per_cpu="7744M",
    email="clemens.buechner@unibas.ch",
    setup="export PATH=/scicore/soft/apps/Python/3.10.4-GCCcore-11.3.0/bin:/scicore/soft/apps/SQLite/3.38.3-GCCcore-11.3.0/bin:/scicore/soft/apps/Tcl/8.6.12-GCCcore-11.3.0/bin:/scicore/soft/apps/CMake/3.23.1-GCCcore-11.3.0/bin:/scicore/soft/apps/libarchive/3.6.1-GCCcore-11.3.0/bin:/scicore/soft/apps/XZ/5.2.5-GCCcore-11.3.0/bin:/scicore/soft/apps/cURL/7.83.0-GCCcore-11.3.0/bin:/scicore/soft/apps/OpenSSL/1.1/bin:/scicore/soft/apps/bzip2/1.0.8-GCCcore-11.3.0/bin:/scicore/soft/apps/ncurses/6.3-GCCcore-11.3.0/bin:/scicore/soft/apps/binutils/2.38-GCCcore-11.3.0/bin:/scicore/soft/apps/GCCcore/11.3.0/bin:/export/soft/lua_lmod/centos7/lmod/lmod/libexec:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/infai/buecle01/bin:/infai/buecle01/bin"
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
exp.add_parser("landmark_parser.py")

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

# exp.add_absolute_report_step(attributes=ATTRIBUTES)
exp.add_comparison_table_step(attributes=ATTRIBUTES)
#exp.add_scatter_plot_step(relative=True, attributes=["total_time", "memory"])

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
