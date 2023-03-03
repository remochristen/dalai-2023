#! /usr/bin/env python3

import itertools
import os

from lab.reports import Attribute

import common_setup
from common_setup import IssueExperiment

ARCHIVE_PATH = "buechner/ipc23-landmarks/dalm/"
DIR = os.path.dirname(os.path.abspath(__file__))

exp = IssueExperiment(
    revisions=[],
    configs=[],
)

def remove_domains_with_derived(run):
    return run['domain'] not in ['optical-telegraphs', 'philosophers', 'psr-large', 'psr-middle']

exp.add_fetcher("../../../../buecle01/ipc-landmarks/experiments/progressions/data/progressions-v3-eval/", name='fetch-facts', filter=remove_domains_with_derived)
exp.add_fetcher("data/dalm-v6-eval/", name='fetch-dalms', merge=True)

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

def filter_algos(run):
    if f"goals=True-gn=True-reas=True" in run["algorithm"]:
        return "dcaf45c03" in run["algorithm"]
    return "rhw-first" in run["algorithm"] or "rhw-possible" in run["algorithm"]

exp.add_absolute_report_step(
    attributes=ATTRIBUTES,
    filter=filter_algos,
)

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
