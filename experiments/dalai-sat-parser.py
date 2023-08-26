#! /usr/bin/env python

import re

from lab.parser import Parser


def first_cost(content, props):
    if props["cost:all"] and len(props["cost:all"]) > 0:
        props["first-cost"] = props["cost:all"][0]

def separate_lms(content, props):
    matches = re.findall(r"Landmark graph contains (\d+) landmarks", content)
    if not "dalai" in content:
        return
    if len(matches) == 3:
        props["landmarks"] = int(matches[1])
        props["landmarks-uaa"] = int(matches[0]) - props["landmarks"]
    elif len(matches) == 2:
        props["landmarks"] = int(matches[0])
        props["landmarks-uaa"] = int(matches[1]) - props["landmarks"]

def find_all_matches(attribute, regex, type=int):
    """
    Look for all occurrences of *regex*, cast what is found in brackets to
    *type* and store the list of found items in the properties dictionary
    under *attribute*. *regex* must contain exactly one bracket group.
    """

    def store_all_occurences(content, props):
        matches = re.findall(regex, content)
        props[attribute] = [type(m) for m in matches]

    return store_all_occurences

parser = Parser()
parser.add_pattern(
    "too_large_uaa_dalms",
    r"Number of too large uaa dalms: (\d+)",
    type=int)
parser.add_function(first_cost)
parser.add_function(find_all_matches("all-expansions", r"Expanded (\d+) state\(s\)", type=int))
parser.add_function(find_all_matches("all-evaluations", r"Evaluated (\d+) state\(s\)", type=int))
parser.add_function(find_all_matches("all-search_time", r"Actual search time: (.+)s", float))
parser.add_function(separate_lms)



parser.parse()
