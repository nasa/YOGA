#!/usr/bin/env python

import json
import sys
from pprint import pprint


def fix_trace_file(name):
    s = ''
    with open(name) as f:
        s = f.read().rstrip().rstrip(',')
        if s[-1] != ']':
            print(name+": adding missing ']'")
            s += ']'
    with open(name, 'w') as f:
        f.write(s)


if len(sys.argv) < 3:
    print("usage: %s <trace files to merge>" % sys.argv[0])
    exit(1)

outfile_name = "combined.trace"
data = []

for filename in sys.argv[1:]:
    if filename == outfile_name:
        continue
    fix_trace_file(filename)
    with open(filename) as json_data:
        d = json.load(json_data)
        for event in d:
            event["tid"] = filename + '.' + str(event["tid"])
        data = data + d

with open(outfile_name, 'w') as outfile:
        json.dump(data, outfile, sort_keys=True, indent=4)
