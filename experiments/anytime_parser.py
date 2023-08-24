#! /usr/bin/env python

import re

from lab.parser import Parser

def get_num_iterations(content, props):
    props["num_iterations"] = len(re.findall(r"Starting search:", content))

def get_costs(content, props):
    props["all_costs"] = re.findall(r"Plan length: (\d+) step", content)

parser = Parser()
parser.add_function(get_num_iterations)
parser.add_function(get_costs)
parser.parse()
