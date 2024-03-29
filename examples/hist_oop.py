#!/usr/bin/env python3

"""
SPDX-License-Identifier: CC-BY-4.0

Copyright 2021 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
"""

import sys
import time

import tracecruncher.ft_utils as tc

name = 'khist_example_oop'
cmds = ['start', 'stop', 'show', 'continue', 'clear', 'close']

# From the event 'kmalloc' in system 'kmem', create a two-dimensional
# histogram, using the event fields 'call_site' and 'bytes_req'.
# The field 'call_site' will be displayed as a kernel symbol.
# The field 'bytes_req' will be displayed as normal field (wothout
# modifying the type).
evt = tc.tc_event('kmem', 'kmalloc')
axes={'call_site': 'sym',
      'bytes_req': 'n'}

# Instead of just recording the "hitcount" in each bin of the histogram,
# we will use the 'value' of 'bytes_alloc' as a weight of the individual
# histogram entries (events).
weights=['bytes_alloc']

# The results will be ordered using 'bytes_req' as a primary and
# 'bytes_alloc' as a secondary sorting criteria. For 'bytes_req' we will
# use descending order.
sort_keys=['bytes_req', 'bytes_alloc']
sort_dir={'bytes_req': 'desc'}

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)

    if not sys.argv[1].isdigit() and not sys.argv[1] in cmds:
        sys.exit(1)

    arg1 = sys.argv[1]
    if arg1.isdigit() or arg1 == 'start':
        # Create the kernel tracing histogram.
        hist = tc.create_hist(name=name, event=evt, axes=axes, weights=weights,
                              sort_keys=sort_keys, sort_dir=sort_dir)
        # Start taking data.
        hist.start()

        if arg1.isdigit():
            # Take data for a while, then stop, print the result and exit. The
            # trace-cruncher module will take care for clearing and destroying
            # the histogram in the kernel.
            time.sleep(int(arg1))
            hist.stop()
            print(hist.data())

        else:
            # Detach the 'hist' object from the trace-cruncher module. This will
            # prevent the kernel histogram from being destroyed when the module
            # is closed (at exit).
            hist.detach()

    else:
        # Try to find an existing histogram with the same definition.
        # The returned histogram is detached from the trace-cruncher module.
        hist = tc.find_hist(name=name, event=evt, axes=axes, weights=weights,
                            sort_keys=sort_keys, sort_dir=sort_dir)

        if arg1 == 'stop':
            # Stop taking data.
            hist.stop()
        elif arg1 == 'show':
            # Print the collected data.
            print(hist.data())
        elif arg1 == 'continue':
            # Continue taking data.
            hist.resume()
        elif arg1 == 'clear':
            # Reset the histogram.
            hist.clear()

        if arg1 == 'close':
            # Attach the 'hist' object to the trace-cruncher module. This will
            # ensure that the kernel histogram will be destroyed when the
            # module is closed (at exit).
            hist.attach()
