#!/usr/bin/env python3

"""
SPDX-License-Identifier: LGPL-2.1

Copyright 2019 VMware Inc, Yordan Karadzhov <ykaradzhov@vmware.com>
"""

import os
import sys
import subprocess as sp
import pprint as pr
from collections import Counter
from tabulate import tabulate

import tracecruncher.ftracepy as ft
import tracecruncher.utils as tc

def gdb_decode_address(obj_file, obj_address):
    """ Use gdb to examine the contents of the memory at this
        address.
    """
    result = sp.run(['gdb',
                     '--batch',
                     '-ex',
                     'x/i ' + str(obj_address),
                     obj_file],
                     stdout=sp.PIPE)

    symbol = result.stdout.decode("utf-8").splitlines()

    if symbol:
        func = [symbol[0].split(':')[0], symbol[0].split(':')[1]]
    else:
        func = [obj_address]

    func.append(obj_file)

    return func

# Get the name of the tracing data file.
fname = str(sys.argv[1])

f = tc.open_file(file_name=fname)

data = f.load()
tasks = f.get_tasks()
#pr.pprint(tasks)

# Get the Event Ids of the page_fault_user or page_fault_kernel events.
pf_eid = f.event_id('exceptions/page_fault_user')

# Gey the size of the data.
d_size = tc.size(data)

# Get the name of the user program.
prog_name = str(sys.argv[2])

table_headers = ['N p.f.', 'function', 'value', 'obj. file']

# Loop over all tasks associated with the user program.
for j in range(len(tasks[prog_name])):
    table_list = []
    count = Counter()
    task_pid = tasks[prog_name][j]
    for i in range(0, d_size):
        if data['event'][i] == pf_eid and data['pid'][i] == task_pid:
            address = f.read_event_field(offset=data['offset'][i],
                                         event_id=pf_eid,
                                         field='address')
            ip = f.read_event_field(offset=data['offset'][i],
                                    event_id=pf_eid,
                                    field='ip')
            count[ip] += 1

    pf_list = count.items()

    # Sort the counters of the page fault instruction pointers. The most
    # frequent will be on top.
    pf_list = sorted(pf_list, key=lambda cnt: cnt[1], reverse=True)

    i_max = 25
    if i_max > len(pf_list):
        i_max = len(pf_list)

    #print(pf_list[:25])
    for i in range(0, i_max):
        func_info = []
        address = int(pf_list[i][0])
        func = ft.get_function(stream_id=f.stream_id, address=address)
        if not func :
            # The name of the function cannot be determined. Most probably
            # this is a user-space function.
            instruction = ft.map_instruction_address(stream_id=f.stream_id,
                                                     pid=task_pid,
                                                     proc_addr=address)
            if instruction:
                func_info = gdb_decode_address(instruction['obj_file'],
                                               instruction['address'])
            else:
                func_info = ['addr: ' + hex(address), 'UNKNOWN']

        table_list.append([pf_list[i][1]] + func_info)

    print('\n{}-{}\n'.format(prog_name, task_pid),
          tabulate(table_list,
                   headers=table_headers,
                   tablefmt='simple'))
