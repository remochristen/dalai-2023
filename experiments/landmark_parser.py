#! /usr/bin/env python

import re

from lab.parser import Parser

parser = Parser()
parser.add_pattern(
    "lmgraph_generation_time",
    r"Landmark graph generation time: (.+)s",
    type=float)
parser.add_pattern(
    "landmarks",
    r"Landmark graph contains (\d+) landmarks",
    type=int)
parser.add_pattern(
    "landmarks_disjunctive",
    r"Landmark graph contains \d+ landmarks, of which (\d+) are disjunctive and \d+ are conjunctive.",
    type=int)
parser.add_pattern(
    "landmarks_conjunctive",
    r"Landmark graph contains \d+ landmarks, of which \d+ are disjunctive and (\d+) are conjunctive.",
    type=int)
parser.add_pattern(
    "orderings",
    r"Landmark graph contains (\d+) orderings.",
    type=int)

parser.add_pattern(
    "initial_h_lm",
    r"Initial heuristic value for (?:dalm_sum|landmark_sum).*: (\d+)",
    type=int)

parser.parse()
