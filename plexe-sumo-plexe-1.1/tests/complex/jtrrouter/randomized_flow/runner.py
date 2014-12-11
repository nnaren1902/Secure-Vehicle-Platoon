#!/usr/bin/env python
# -*- coding: utf-8 -*-

# call jtrrouter twice and check that the output differs
import sys,os,subprocess,random
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), '..', '..', '..', '..', "tools"))
from sumolib import checkBinary

def get_depart_lines(route_file):
    return [l for l in open(route_file) if 'depart' in l]

output_file1 = 'output1.rou.xml'
output_file2 = 'output2.rou.xml'

jtrrouter = checkBinary('jtrrouter')
assert(jtrrouter)

args = [jtrrouter,
        '--net-file', 'input_net.net.xml',
        '--flow-files', 'input_flows.flows.xml',
        '--turn-ratio-files', 'input_turns.turns.xml',
        '--output-file', output_file1,
        '--sinks=end',
        '--seed', None,
        '--no-step-log',
        '--randomize-flows']

args[11] = str(random.randint(0, 2 ** 31))
subprocess.call(args)
route_lines1 = get_depart_lines(output_file1)

args[8] = output_file2
args[11] = str(random.randint(0, 2 ** 31))
subprocess.call(args)
route_lines2 = get_depart_lines(output_file2)

if route_lines1 != route_lines2:
    print('test passed. output is random')
else:
    print('test failed. output is deterministic')
