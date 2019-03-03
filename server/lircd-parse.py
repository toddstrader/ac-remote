#!/usr/bin/env python3

# Parses LIRC config files such as the one found at https://github.com/mattjm/Fujitsu_IR/blob/master/lircd.conf

import argparse
import re
import json

default_out_file = 'ac-codes.json'

parser = argparse.ArgumentParser(description="Parse LIRC file and write out JSON")
parser.add_argument('--out', default=default_out_file, help='JSON file (default is ' + default_out_file + ')')
parser.add_argument('lircd', help='LIRC file')

args = parser.parse_args()

reading_codes = False
codes = {}
current = {}

with open(args.lircd, 'r') as f:
    for line in f:
        if 'end raw_codes' in line:
            reading_codes = False
        if (reading_codes):
            m = re.search('name\s+(\S+)\s*(?:#(.*))?$', line)
            if (m):
                name = m.group(1)
                comment = m.group(2)
                current = {'comment': comment, 'data': []}
                codes[name] = current
            elif (current):
                try:
                    values = list(map(int, line.split()))
                    current['data'].extend(values)
                except ValueError:
                    pass
        if 'begin raw_codes' in line:
            reading_codes = True

with open(args.out, 'w') as f:
    json.dump(codes, f)
