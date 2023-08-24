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
BENCHMARKS_DIR = os.environ["IPC2023_SAT_BENCHMARKS"]
REVISIONS = [
    ("35c7de4b85bc0a9dff9038848891fe92ba25e2ac", ""),
]
BUILD = "release"
DRIVER_OPTIONS = [
    "--build", BUILD,
    "--overall-memory-limit", "8192M",
]

dalai_sat_lm_factory = "dalm_reasonable_orders_hps(dalm_rhw(max_preconditions=12))"
CONFIGS = [
    IssueConfig(
        f"lama",
        [],
        build_options=[BUILD],
        driver_options=DRIVER_OPTIONS + ["--alias", "lama"],
    ),
    IssueConfig(
        f"lama-minus-ff",
        [
            "--search",
            "--if-unit-cost",
            f"let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
            """iterated([
                lazy_greedy([hlm],preferred=[hlm]),
                lazy_wastar([hlm],preferred=[hlm],w=5),
                lazy_wastar([hlm],preferred=[hlm],w=3),
                lazy_wastar([hlm],preferred=[hlm],w=2),
                lazy_wastar([hlm],preferred=[hlm],w=1)
             ],repeat_last=true,continue_on_fail=true))""",
            "--if-non-unit-cost",
            f"let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=false),"
            f"let(hlm2, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone),pref=false),"
            """iterated([
                lazy_greedy([hlm1],preferred=[hlm1],
                     cost_type=one,reopen_closed=false),
                lazy_greedy([hlm2],preferred=[hlm2],
                     reopen_closed=false),
                lazy_wastar([hlm2],preferred=[hlm2],w=5),
                lazy_wastar([hlm2],preferred=[hlm2],w=3),
                lazy_wastar([hlm2],preferred=[hlm2],w=2),
                lazy_wastar([hlm2],preferred=[hlm2],w=1)
            ],repeat_last=true,continue_on_fail=true)))""",
            # Append --always to be on the safe side if we want to append
            # additional options later.
            "--always"
        ],
        build_options=[BUILD],
        driver_options=DRIVER_OPTIONS,
    ),
    IssueConfig(
        f"dalai-sat",
        [],
        build_options=[BUILD],
        driver_options=DRIVER_OPTIONS + ["--alias", "dalai-sat-2023"],
    ),
    IssueConfig(
        f"dalai-sat-plus-ff",
        [
            "--search",
            "--if-unit-cost",
            f"let(hlm_first,dalm_greedy_hs({dalai_sat_lm_factory},pref=true),"
            f"let(hlm,dalm_greedy_hs(dalm_uaa({dalai_sat_lm_factory}),pref=true),"
            "let(hff,ff(),"
            "iterated(["
                "lazy_greedy([hff,hlm_first],preferred=[hff,hlm_first],boost=1),"
                "lazy_wastar([hff,hlm],preferred=[hff,hlm],boost=1,w=5),"
                "lazy_wastar([hff,hlm],preferred=[hff,hlm],boost=1,w=3),"
                "lazy_wastar([hff,hlm],preferred=[hff,hlm],boost=1,w=2),"
                "lazy_wastar([hff,hlm],preferred=[hff,hlm],boost=1,w=1)"
            "],repeat_last=true,continue_on_fail=true))))",
            "--if-non-unit-cost",
            f"let(hlm_orig,dalm_greedy_hs(dalm_uaa({dalai_sat_lm_factory}),pref=true),"
            f"let(hlm_unit,dalm_greedy_hs({dalai_sat_lm_factory},transform=adapt_costs(one),pref=true),"
            f"let(hlm_plus,dalm_greedy_hs(dalm_uaa({dalai_sat_lm_factory}),transform=adapt_costs(plusone),pref=true),"
            "let(hff_unit, ff(transform=adapt_costs(one)),"
            "let(hff_plus, ff(transform=adapt_costs(plusone)),"
            "iterated(["
                "lazy_greedy([hff_unit,hlm_unit],preferred=[hff_unit,hlm_unit],boost=1,cost_type=one,reopen_closed=false),"
                "lazy_greedy([hff_plus,hlm_plus],preferred=[hff_plus,hlm_plus],boost=1,reopen_closed=false),"
                "lazy_wastar([hff_plus,hlm_plus],preferred=[hff_plus,hlm_plus],boost=1,w=5),"
                "lazy_wastar([hff_plus,hlm_plus],preferred=[hff_plus,hlm_plus],boost=1,w=3),"
                "lazy_wastar([hff_plus,hlm_plus],preferred=[hff_plus,hlm_plus],boost=1,w=2),"
                "lazy_wastar([hff_plus,hlm_plus],preferred=[hff_plus,hlm_plus],boost=1,w=1),"
                "lazy_wastar([hff_plus,hlm_orig],preferred=[hff_plus,hlm_orig],boost=1,w=1)"
            "],repeat_last=true,continue_on_fail=true))))))",
            # Append --always to be on the safe side if we want to append
            # additional options later.
            "--always",
        ],
        build_options=[BUILD],
        driver_options=DRIVER_OPTIONS,
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
    memory_per_cpu="8500M",
    email="remo.christen@unibas.ch",
    export=["PATH", "IPC23_SAT_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = ["rubiks-cube:p02.pddl"]
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
exp.add_parser("../anytime_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("landmarks_disjunctive", min_wins=False),
    Attribute("landmarks_conjunctive", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("lmgraph_generation_time", min_wins=True),
    Attribute("all_costs", absolute=True, min_wins=True),
    Attribute("num_iterations", min_wins=False),
]

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch', merge=True)

exp.add_absolute_report_step(attributes=ATTRIBUTES)
# exp.add_scatter_plot_step(relative=True, attributes=["search_time", "cost"])
# exp.add_scatter_plot_step(relative=False, attributes=["search_time", "cost"])

exp.add_parse_again_step()

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
